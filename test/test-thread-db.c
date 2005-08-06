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

#include <pthread.h>


#include <stdint.h>
#include <stdio.h>
#include "mbool.h"
#include "memory-reader.h"
#include "elf32-parser.h"
#include "breakpoint.h"
#include "user-ctx.h"


#include <thread_db.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>


static struct breakpoint g_th_create_bp;
static struct breakpoint g_new_th_create_bp;
static struct breakpoint g_th_death_bp;
static td_thragent_t *g_ta;

static char const *td_err_to_string (td_err_e td_err)
{
#define FOO(x) \
  case x: \
    return #x; \
    break;

        switch (td_err) {
                FOO (TD_OK);          /* No error.  */
                FOO (TD_ERR);         /* No further specified error.  */
                FOO (TD_NOTHR);       /* No matching thread found.  */
                FOO (TD_NOSV);        /* No matching synchronization handle found.  */
                FOO (TD_NOLWP);       /* No matching light-weighted process found.  */
                FOO (TD_BADPH);       /* Invalid process handle.  */
                FOO (TD_BADTH);       /* Invalid thread handle.  */
                FOO (TD_BADSH);       /* Invalid synchronization handle.  */
                FOO (TD_BADTA);       /* Invalid thread agent.  */
                FOO (TD_BADKEY);      /* Invalid key.  */
                FOO (TD_NOMSG);       /* No event available.  */
                FOO (TD_NOFPREGS);    /* No floating-point register content available.  */
                FOO (TD_NOLIBTHREAD); /* Application not linked with thread library.  */
                FOO (TD_NOEVENT);  /* Requested event is not supported.  */
                FOO (TD_NOCAPAB);     /* Capability not available.  */
                FOO (TD_DBERR);       /* Internal debug library error.  */
                FOO (TD_NOAPLIC);     /* Operation is not applicable.  */
                FOO (TD_NOTSD);       /* No thread-specific data available.  */
                FOO (TD_MALLOC);      /* Out of memory.  */
                FOO (TD_PARTIALREG);  /* Not entire register set was read or written.  */
                FOO (TD_NOXREGS);     /* X register set not available for given thread.  */
                FOO (TD_TLSDEFER);    /* Thread has not yet allocated TLS for given module.  */
                FOO (TD_VERSION);     /* Version if libpthread and libthread_db do not match.  */
                FOO (TD_NOTLS);        /* There is no TLS segment in the given module.  */
        }
        return "DEADBEAF";
#undef FOO
}

static void
output_error (char const *str, td_err_e td_err)
{
        printf ("%s %s\n", str, td_err_to_string (td_err));
}

typedef enum {
        PS_OK,                /* Generic "call succeeded".  */
        PS_ERR,               /* Generic error. */
        PS_BADPID,            /* Bad process handle.  */
        PS_BADLID,            /* Bad LWP identifier.  */
        PS_BADADDR,           /* Bad address.  */
        PS_NOSYM,             /* Could not find given symbol.  */
        PS_NOFREGS            /* FPU register set not available for given LWP.  */
} ps_err_e;

#define PS_DEBUG(x) \
  printf ("PS DEBUG %s\n", x);

struct ps_prochandle {
        int test;
};

/**
 * Callback functions to access the target process.
 * Here, the target process is ourselves so these
 * functions are trivially implemented.
 */
ps_err_e 
ps_pdread (struct ps_prochandle *ph,
	   psaddr_t addr, void *buf, size_t size)
{
        //PS_DEBUG ("ps_pdread");
        memcpy (buf, addr, size);
        return PS_OK;
}

ps_err_e 
ps_pdwrite (struct ps_prochandle *ph,
	    psaddr_t addr, const void *buf, size_t size)
{
        //PS_DEBUG ("ps_pdwrite");
        memcpy (addr, buf, size);
        return PS_OK;
}

pid_t 
ps_getpid (struct ps_prochandle *ph)
{
        PS_DEBUG ("ps_getpid");
        return getpid ();
}
ps_err_e 
ps_lgetregs (struct ps_prochandle *ph,
	     lwpid_t lid, prgregset_t gregset)
{
        PS_DEBUG ("ps_lgetregs");
        return PS_ERR;
}
ps_err_e 
ps_lsetregs (struct ps_prochandle *ph,
	     lwpid_t lid, const prgregset_t gregset)
{
        PS_DEBUG ("ps_lsetregs");
        return PS_ERR;
}
ps_err_e 
ps_lgetfpregs (struct ps_prochandle *ph,
	       lwpid_t lid, prfpregset_t *gregset)
{
        PS_DEBUG ("ps_lgetfpregs");
        return PS_ERR;
}
ps_err_e 
ps_lsetfpregs (struct ps_prochandle *ph,
	       lwpid_t lid, const prfpregset_t *gregset)
{
        PS_DEBUG ("ps_lsetfpregs");
        return PS_ERR;
}

#define __USE_GNU
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <link.h>

ps_err_e 
ps_pglobal_lookup (struct ps_prochandle *ph,
		   const char *object_name,
		   const char *sym_name,
		   psaddr_t *sym_addr)
{
        int fd, status;
        uint8_t *data;
        uint32_t size;
        void * handle;
        struct stat stat_buf;
        struct link_map *map;
        struct memory_reader reader;
        struct elf32_header elf32;
        struct elf32_symbol symbol;
        char *filename;

        dlerror ();
        handle = dlopen (object_name, RTLD_LAZY);
        if (handle == NULL) {
                printf ("error getting handle for main binary: %s\n", dlerror ());
                goto error;
        }
        if (dlinfo (handle, RTLD_DI_LINKMAP, &map) == -1) {
                printf ("error getting link map: %s\n", dlerror ());
                dlclose (handle);
                goto error;
        }
        dlclose (handle);

        if (strcmp (map->l_name, "") == 0) {
                /* this is the main binary. I do not know why the loader does not
                 * copy here its name so we need to open it directly with /proc/self/exe
                 */
                filename = "/proc/self/exe";
        } else {
                filename = map->l_name;
        }
        fd = open (filename, O_RDONLY);
        if (fstat (fd, &stat_buf) == -1) {
                printf ("unable to stat %s\n", filename);
                goto error;
        }
        data = mmap (0, stat_buf.st_size, PROT_READ, MAP_SHARED, fd, 0);
        size = stat_buf.st_size;

        memory_reader_initialize (&reader, data, size);
        status = elf32_parser_initialize (&elf32, READER (&reader));
        if (status == -1) {
                /*printf ("unable to initialize parser for %s\n", filename); */
                goto error;
        }
                
        status = elf32_parser_find_symbol_slow (&elf32, sym_name, &symbol, READER (&reader));
        if (status == -1) {
                goto error;
        }

        *sym_addr = (psaddr_t) (map->l_addr + symbol.st_value);

        printf ("found symbol \"%s\": %p in %s\n", sym_name, *sym_addr, filename);
        
        return PS_OK;
 error:
        /* we probably should try to lookup the symbol in the whole link map
         * if it fails for the requested object name.
         */
        return PS_NOSYM;
}

static void
trap_handler (int signo, siginfo_t *info, ucontext_t *user_ctx)
{
        long pc = user_ctx_get_pc (user_ctx) - 1;
        if (pc == g_th_create_bp.address) {
                td_event_msg_t msg;
                breakpoint_continue_from_handler (&g_th_create_bp, user_ctx);
                if (td_ta_event_getmsg (g_ta, &msg) == TD_OK) {
                        td_thrinfo_t info;
                        long thread_start;
                        td_thr_events_t events;
                        if (msg.event != TD_CREATE) {
                                printf ("hrm, a small error\n");
                                goto error;
                        }
                        td_event_emptyset (&events);
                        td_event_addset (&events, TD_CREATE);
                        td_event_addset (&events, TD_DEATH);
                        if (td_thr_set_event (msg.th_p, &events) != TD_OK) {
                                goto error;
                        }
                        if (td_thr_event_enable (msg.th_p, TRUE) != TD_OK) {
                                goto error;
                        }

                        if (td_thr_get_info (msg.th_p, &info) != TD_OK) {
                                goto error;
                        }
                        thread_start = (long) info.ti_startfunc;
                        g_new_th_create_bp.address = thread_start;
                        breakpoint_enable (&g_new_th_create_bp);
                        printf ("thread starting.\n");
                } else {
                        printf ("could not get creation event\n");
                        goto error;
                }
        } else if (pc == g_th_death_bp.address) {
                breakpoint_continue_from_handler (&g_th_death_bp, user_ctx);
                printf ("death thread\n");
        } else if (pc == g_new_th_create_bp.address) {
                breakpoint_continue_from_handler (&g_new_th_create_bp, user_ctx);
                printf ("thread started.\n");
        } else {
                printf ("AAARGGGHHHH!\n");
        }
 error:
        return;
}

typedef void (*action_callback_t) (int, siginfo_t *, void *);


static td_thragent_t *
start_db (void)
{
        td_err_e status;
        struct ps_prochandle ph;
        td_thragent_t *ta;
        td_notify_t callback;
        td_thr_events_t events;
        pthread_t self;
        td_thrhandle_t th;
        struct sigaction action;

        action.sa_sigaction = (action_callback_t)trap_handler;
        action.sa_flags = SA_SIGINFO;
        if (sigaction (SIGTRAP, &action, NULL) == -1) {
                printf ("could not install trap signal handler\n");
                exit (1);
        }

        status = td_init ();
        if (status != TD_OK) {
                output_error ("init failure", status);
                goto error;
        }
        
        status = td_ta_new (&ph, &ta);
        if (status != TD_OK) {
                output_error ("agent creation failure", status);
                goto error;
        }
        td_event_emptyset (&events);
        td_event_addset (&events, TD_CREATE);
        td_event_addset (&events, TD_DEATH);
        status = td_ta_set_event (ta, &events);
        if (status != TD_OK) {
                output_error ("could not set events", status);
                goto error;
        }
        status = td_ta_event_addr (ta, TD_CREATE, &callback);
        if (status != TD_OK) {
                output_error ("could not get event address", status);
                goto error;
        }
        g_th_create_bp.address = (long)callback.u.bptaddr;
        breakpoint_enable (&g_th_create_bp);

        status = td_ta_event_addr (ta, TD_DEATH, &callback);
        if (status != TD_OK) {
                output_error ("could not get event address", status);
                goto error;
        }
        g_th_death_bp.address = (long)callback.u.bptaddr;
        breakpoint_enable (&g_th_death_bp);


        self = pthread_self ();
        status = td_ta_map_id2thr (ta, self, &th);
        if (status != TD_OK) {
                output_error ("could not map pthread_t to thread debug handle", status);
                goto error;
        }
        status = td_thr_set_event (&th, &events);
        if (status != TD_OK) {
                output_error ("cannot set events", status);
                goto error;
        }
        status = td_thr_event_enable (&th, TRUE);
        if (status != TD_OK) {
                output_error ("cannot enable events", status);
                goto error;
        }

        return ta;
 error:
        return NULL;
}

static void *thread_cb (void *arg)
{
        int i = 0;
        while (i < 10) {
                sleep (1);
                printf ("cb\n");
                i++;
        }
        printf ("out\n");
        return NULL;
}

int 
test_thread_db (int argc, char *argv[])
{
        pthread_t thread;
        int result;

        g_ta = start_db ();

        result = pthread_create (&thread, NULL, thread_cb, NULL);
        if (result < 0) {
                printf ("thread creation error\n");
        }

        while (TRUE) {sleep(1);}
        //loop_db (ta);
        return 0;
}
