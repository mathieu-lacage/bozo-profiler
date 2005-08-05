/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  version 2 as published by the Free Software Foundation.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

  Copyright (C) 2004,2005  INRIA
  Author: Mathieu Lacage <lacage@sophia.inria.fr>
*/

#define _LARGEFILE64_SOURCE 1

#include "manager.h"
#include "circular-buffer.h"
#include "mbool.h"
#include "sllist.h"
#include "breakpoint.h"
#include "utils.h"
#include "pthread-utils.h"

#include <pthread.h>
#include <signal.h>
#include <sys/ucontext.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <link.h>


struct buffer_list {
        struct buffer_list *next;
        struct managed_thread *thread;
        int fd;
};

struct manager {
        struct breakpoint ld_bp;
        int *ld_state;
        struct buffer_list *l;
        pthread_mutex_t l_mutex;
        char process_name[1024];
};

static mbool g_initialized = FALSE;
static struct manager g_manager;

static void ensure_manager_started (void);
static int  open_dump_buffer (struct manager *manager, int tid);
static void write_to_fd (int fd, uint8_t *data, uint32_t to_write);
static void manager_destructor (void)  __attribute__ ((destructor));
static void trap_handler (int signo, siginfo_t *info, ucontext_t *user_ctx);
static void at_fork_prepare_callback (void);
static void at_fork_parent_callback (void);
static void at_fork_child_callback (void);



static void flush_thread (struct managed_thread *thread, int fd)
{
        int i;
        for (i = 0; i < 2; i++) {
                uint8_t *data;
                uint32_t to_write;
                to_write = circular_buffer_get_read_size (&thread->buffer);
                if (to_write == 0) {
                        continue;
                }
                data = circular_buffer_peek_read_ptr (&thread->buffer);
                write_to_fd (fd, data, to_write);
                circular_buffer_commit_read (&thread->buffer, to_write);
        }       
}

static void 
flush_manager (struct manager *manager)
{
        struct buffer_list *tmp;
        pthread_mutex_lock (&manager->l_mutex);
        for (tmp = manager->l; tmp != NULL; tmp = (struct buffer_list *)tmp->next) {
                flush_thread (tmp->thread, tmp->fd);
        }
        pthread_mutex_unlock (&manager->l_mutex);
}

static void 
write_to_fd (int fd, uint8_t *data, uint32_t to_write)
{
        uint8_t *end;
        end = data + to_write;
        while (data < end) {
                ssize_t written;
                errno = 0;
                written = write (fd, data, to_write);
                if (written == -1) {
                        printf ("error %s\n", strerror (errno));
                        continue;
                }
                //printf ("written %d\n", written);
                to_write -= written;
                data += written;
        }

}

static void *
manager_thread (void *arg)
{
        struct manager *manager = (struct manager *)arg;
        while (TRUE) {
                flush_manager (manager);
                sleep (0);
        }
        return NULL;
}

/* start a manager. This function assumes it needs 
 * to allocate all ressources.
 */
static void 
ensure_manager_started (void)
{
        pthread_t th;
        struct sigaction action; /* XXX need to initialize ? */
        struct r_debug *r_debug;

        if (g_initialized) {
                return;
        }

        if (utils_copy_process_name (g_manager.process_name, 1024) == -1) {
                g_manager.process_name[1023] = 0;
        }
        g_manager.l = NULL;
        /* always returns zero. */
        pthread_mutex_init (&g_manager.l_mutex, NULL);

        action.sa_sigaction = (void (*)(int,siginfo_t *,void*))trap_handler;
        action.sa_flags = SA_SIGINFO;
        if (sigaction (SIGTRAP, &action, NULL) == -1) {
                printf ("could not install trap signal handler\n");
                goto error;
        }

        r_debug = utils_get_ld_debug ();
        if (r_debug == NULL) {
                printf ("could not get debug struct\n");
                goto error;
        }
        g_manager.ld_state = (int*)&r_debug->r_state;
        g_manager.ld_bp.address = r_debug->r_brk;
        if (breakpoint_enable (&g_manager.ld_bp) == -1) {
                printf ("could not set breakpoint\n");
                goto error;
        }

        if (pthread_atfork (at_fork_prepare_callback, 
                            at_fork_parent_callback, 
                            at_fork_child_callback) == -1) {
                printf ("could not set child fork callback\n");
                goto error;
        }
       
        if (pthread_create (&th, NULL, manager_thread, &g_manager) != 0) {
                printf ("thread cannot be started\n");
                goto error;
        }

        g_initialized = TRUE;
 error:
        return;
}

static int
open_dump_buffer (struct manager *manager, int tid)
{
        int fd;
        char buf[1024];
        sprintf (buf, "%s-%d.func-dump", manager->process_name, tid);
        fd = open (buf, O_LARGEFILE | O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
        return fd;
}

/*****************************************************************
 * The functions below are invoked by different types of event.
 * They are the most important functions in this file.
 ****************************************************************/

/* To record a map change, we push the map data into the 
 * buffer of each of the managed threads. This function is 
 * invoked from a signal handler which was triggered by a 
 * call to dlopen or dlclose. We are thus most likely running
 * within the context of one of the managed threads. We thus
 * merely mark each of these threads that a map change is pending.
 * The threads are then responsible for writing the new map
 * in their buffer.
 */
static void
trap_handler (int signo, siginfo_t *info, ucontext_t *user_ctx)
{
        struct manager *manager = &g_manager;
        int rstate;
        rstate = *manager->ld_state;
        breakpoint_continue_from_handler (&manager->ld_bp, user_ctx);
        if (rstate == RT_CONSISTENT) {
                struct buffer_list *tmp;
                pthread_mutex_lock (&manager->l_mutex);
                for (tmp = manager->l; tmp != NULL; tmp = (struct buffer_list *)tmp->next) {
                        record_map_change_pending (tmp->thread);
                }
                pthread_mutex_unlock (&manager->l_mutex);
                //printf ("recording map change\n");
        }
}

static void
at_fork_prepare_callback (void) 
{
        if (!g_initialized) {
                printf ("oh, something really really bad happened\n");
        }
        pthread_mutex_lock (&g_manager.l_mutex);
}

static void
at_fork_parent_callback (void)
{
        if (!g_initialized) {
                printf ("oh, something really really bad happened\n");
        }
        pthread_mutex_unlock (&g_manager.l_mutex);
}

/* The parent process triggered a fork. This callback
 * is invoked from the child before the child is given
 * a chance to run. The other threads which might have 
 * been running in the parent are not alive here (this
 * is what the posix standard says should happen when
 * a threaded application is forked) so we don't have
 * a manager thread running.
 *
 * Here, we need to ensure to get rid of the fds used 
 * in the parent (we must not flush their data on disk 
 * because that would mean the data is written twice, 
 * once by the manager thread in the parent, once by us).
 * 
 * We also need to re-create at least a dump fd for our
 * single thread in this new process as well as start 
 * a new thread manager.
 */
static void
at_fork_child_callback (void)
{
        /* close all open dump fds. Do not flush them. */
        struct buffer_list *tmp, *prev;

        if (!g_initialized) {
                printf ("oh, something really really bad happened\n");
        }

        pthread_mutex_unlock (&g_manager.l_mutex);
        pthread_mutex_destroy (&g_manager.l_mutex);

        g_initialized = FALSE;
        breakpoint_disable (&g_manager.ld_bp);
        for (tmp = g_manager.l, prev = NULL; tmp != NULL; prev = tmp, tmp = tmp->next) {
                if (prev != NULL) {
                        prev->thread = NULL;
                        close (prev->fd);
                        free (prev);
                }
        }
        prev->thread = NULL;
        close (prev->fd);
        free (prev);
        
        g_manager.l = NULL;

        /* restart a new manager thread and allocate manager ressources. */
        ensure_manager_started ();
        /* reset the buffer for the current thread and 
         * re-register it in the new manager. */
        record_reset ();
}

/**
 * This destructor is invoked whenever the process finished through
 * either the end of main () or a call to exit (). This function
 * does its best to write any pending data to the output files.
 * and complete its destruction.
 * Of course, this destructor will not be invoked in case:
 *   - _exit () is called which means that the file descriptors 
 *     are closed so any data already written to the fds is written
 *     on disk. Of course, in this case, we might miss some data
 *     which was pending a write to the fds but there is not much
 *     we can do about this.
 *   - a bad signal comes which means the process is terminated. 
 *     We could try to hook into these signals but that would not
 *     really help since the real problem is that we did get a bad
 *     signal so the application is broken.
 *
 * This function is probably invoked within the context of the main 
 * managed thread.
 */
static void
manager_destructor (void)
{
        struct manager *manager = &g_manager;
        //printf ("destructor\n");
        flush_manager (manager);
}

/**
 * The single function exported to the client threads
 * to register their event buffers. It is invoked within 
 * the context of a managed thread which is why we perform
 * proper locking to access the list of managed threads.
 */
void manager_register_thread (struct managed_thread *thread)
{
        struct buffer_list *l;
        int tid;
        
        ensure_manager_started ();
        l = malloc (sizeof (struct buffer_list));
        if (l == NULL) {
                printf ("no memory left\n");
                return;
        }

        l->thread = thread;
        tid = pthread_utils_to_id (pthread_self ());
        l->fd = open_dump_buffer (&g_manager, tid);
        pthread_mutex_lock (&g_manager.l_mutex);
        l->next = g_manager.l;
        g_manager.l = l;
        pthread_mutex_unlock (&g_manager.l_mutex);
}

