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

#include "breakpoint.h"
#include "fn-address.h"
#include "symbol.h"
#include "load-map.h"

#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef NULL
  #define NULL (0)
#endif

struct breakpoint bp;

static void
trap_handler (int signo, siginfo_t *info, ucontext_t *user_ctx)
{
         breakpoint_continue_from_handler (&bp, user_ctx);
         printf ("trap\n");
}

static int foo (void)
{
        printf ("foo\n");
        return 7;
}

typedef void (*action_callback_t) (int, siginfo_t *, void *);

static int 
definitions_cb (struct elf32_symbol const *symbol, uint64_t address, 
                char const *real_name, void *context)
{
        bp.address = address;
        if (breakpoint_enable (&bp) == -1) {
                printf ("could not set breakpoint\n");
        }
        return 1;
}

int 
main (int argc, char *argv[])
{
        struct load_map map;
        struct sigaction action;

        load_map_linux_initialize (&map);

        action.sa_sigaction = (action_callback_t)trap_handler;
        action.sa_flags = SA_SIGINFO;
        if (sigaction (SIGTRAP, &action, NULL) == -1) {
                printf ("could not install trap signal handler\n");
                exit (1);
        }

        printf ("try foo\n");
        bp.address = (long)foo;
        if (breakpoint_enable (&bp) == -1) {
                printf ("could not set breakpoint\n");
        }

        foo ();
        foo ();

        printf ("try getpid\n");
        if (symbol_iterate_definitions (&map, "getpid", definitions_cb, NULL) == -1) {
                printf ("error looking up getpid\n");
        }
        getpid ();
        getpid ();

        return 0;
}
