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

#include "null.h"
#include "breakpoint.h"
#include "utils.h"
#include "foo.h"

#include <link.h>
#include <pthread.h>
#include <signal.h>
#include <sys/ucontext.h>
#include <stdio.h>

static struct breakpoint g_bp;
static int *g_ld_brk_state = NULL;

static void
trap_handler (int signo, siginfo_t *info, ucontext_t *user_ctx)
{
        int rstate;
        rstate = *g_ld_brk_state;
        breakpoint_continue_from_handler (&g_bp, user_ctx);
        if (rstate == RT_CONSISTENT) {
                printf ("consistent\n");
        } else if (rstate == RT_ADD) {
                printf ("adding\n");
	} else if (rstate == RT_DELETE) {
                printf ("removing\n");
	} else {
                printf ("unknown state\n");
        }
}


int main (int argc, char *argv[])
{
        struct sigaction action;
        struct r_debug *r_debug;
        void *h;

        action.sa_sigaction = (void(*)(int,siginfo_t*,void*))trap_handler;
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
        g_ld_brk_state = (int*)&r_debug->r_state;
        g_bp.address = (long)r_debug->r_brk;
        if (breakpoint_enable (&g_bp) == -1) {
                printf ("could not set breakpoint\n");
                goto error;
        }

        printf ("open foo\n");
        h = dlopen ("test/libfoo.so", RTLD_LAZY);
        printf ("close foo\n");
        dlclose (h);
        printf ("call foo\n");
        foo ();
        printf ("called foo\n");
 error:
        return 0;
}
