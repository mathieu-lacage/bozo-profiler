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

#include "record.h"
#include "circular-buffer.h"
#include "manager.h"
#include "mbool.h"
#include "minmax.h"
#include "utils.h"
#include "null.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <link.h>

static __thread struct managed_thread *g_thread;
static __thread mbool g_initialized = FALSE;

static void record_init (void)
{
        g_thread = malloc (sizeof (struct managed_thread));
        if (g_thread == NULL) {
                printf ("this is so so bad\n");
                exit (1);
        }
        g_thread->pending_map_change = TRUE;
        circular_buffer_initialize (&g_thread->buffer);
        manager_register_thread (g_thread);
        g_initialized = TRUE;
}

static struct managed_thread *
get_thread (void)
{
        if (!g_initialized) {
                record_init ();
        }
        return g_thread;
}

static void 
write_data (struct managed_thread *thread, uint8_t *src, uint32_t size)
{
        uint8_t *end;
        end = src + size;
        while (src < end) {
                uint32_t left, to_write;
                uint8_t *dest;
                left = circular_buffer_get_write_size (&thread->buffer);
                if (left == 0) {
                        sleep (0);
                        continue;
                }
                to_write = min (left, size);
                dest = circular_buffer_peek_write_ptr (&thread->buffer);
                memcpy (dest, src, to_write);
                circular_buffer_commit_write (&thread->buffer, to_write);
                size -= to_write;
                src += to_write;
        }
}

/* Layout: 
 * int type
 * int n
 * {
 *    void *base_address
 *    int len
 *    char * len
 * } * n
 */
static void
write_map (struct managed_thread *thread)
{
        char buffer[1024];
        struct link_map *tmp, *map;
        int type;
        int n;
        //printf ("write map\n");
        type = RECORD_TYPE_MAP;
        write_data (thread, (uint8_t *)&type, sizeof (type));
        map = utils_get_link_map ();
        n = 0;
        for (tmp = map; tmp != NULL; tmp = tmp->l_next) {
                n++;
        }
        write_data (thread, (uint8_t *)&n, sizeof (n));
        for (tmp = map; tmp != NULL; tmp = tmp->l_next) {
                int len;
                char *filename;
                write_data (thread, (uint8_t *)&tmp->l_addr, sizeof (tmp->l_addr));
                if (strcmp (tmp->l_name, "") == 0) {
                        if (utils_copy_process_name (buffer, 1024) == -1) {
                                printf ("process name too large\n");
                                return;
                        }
                        filename = buffer;
                } else {
                        filename = tmp->l_name;
                }
                len = strlen (filename) + 1;
                write_data (thread, (uint8_t *)&len, sizeof (len));
                write_data (thread, (uint8_t *)filename, len);
        }
}

void 
record_map_change_pending (struct managed_thread *thread)
{
        thread->pending_map_change = TRUE;
}


void record_reset (void)
{
        record_init ();
}

static void 
record_enter (void *this_function, void *call_site)
{
        struct managed_thread *thread;
        struct record_t entry;

        thread = get_thread ();
        if (thread->pending_map_change) {
                write_map (thread);
                thread->pending_map_change = FALSE;
        }

        entry.type = RECORD_TYPE_ENTER;
        entry.this_function = this_function;
        entry.call_site = call_site;

        write_data (thread, (uint8_t *)&entry, sizeof (entry));
}


static void 
record_leave (void *this_function, void *call_site)
{
        struct managed_thread *thread;
        struct record_t entry;

        thread = get_thread ();

        entry.type = RECORD_TYPE_LEAVE;
        entry.this_function = this_function;
        entry.call_site = call_site;

        write_data (thread, (uint8_t *)&entry, sizeof (entry));
}


void __cyg_profile_func_enter (void *this_fn,
			       void *call_site)
{
        record_enter (this_fn, call_site);
}

void __cyg_profile_func_exit  (void *this_fn,
			       void *call_site)
{
        record_leave (this_fn, call_site);
}
