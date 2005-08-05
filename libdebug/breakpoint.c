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

  Copyright (C) 2004,2005  Mathieu Lacage
  Author: Mathieu Lacage <mathieu@gnu.org>
*/

#include <signal.h>
#include <stdio.h>
#include <string.h>

#include "breakpoint.h"
#include "null.h"
#include "user-ctx.h"

#include <errno.h>
#include <sys/mman.h>
#include <limits.h> /* for PAGESIZE */

#ifndef PAGESIZE
#define PAGESIZE 4096
#endif



typedef void (*action_callback_t) (int, siginfo_t *, void *);


static int
write_enable (long address)
{
        int status;
        long addr_page_aligned;
        addr_page_aligned = address & ~(PAGESIZE-1);
        status = mprotect ((void *)addr_page_aligned, PAGESIZE, PROT_READ | PROT_EXEC | PROT_WRITE);
        if (status == -1) {
                printf ("mprotect failed: %s\n", strerror (errno));
                goto error;
        }
        return 0;
 error:
        return -1;
}

static int
write_disable (long address)
{
        int status;
        long addr_page_aligned;
        addr_page_aligned = address & ~(PAGESIZE-1);
        status = mprotect ((void *)addr_page_aligned, PAGESIZE, PROT_READ | PROT_EXEC);
        if (status == -1) {
                printf ("mprotect failed: %s\n", strerror (errno));
                goto error;
        }
        return 0;
 error:
        return -1;
}


int 
breakpoint_enable (struct breakpoint *bp)
{
        uint8_t *p = (uint8_t *)bp->address;
        /* save what is written there */
        bp->data = p[0];
        if (write_enable (bp->address) == -1) {
                goto error;
        }
        /* write "int3" asm instruction at this address. */
        p[0] = 0xcc;
        if (write_disable (bp->address) == -1) {
                goto error;
        }
        //printf ("enable bp %p\n", bp->address);
        return 0;
 error:
        return -1;
}

int 
breakpoint_disable (struct breakpoint const *bp)
{
        uint8_t *p = (uint8_t *)bp->address;
        if (write_enable (bp->address) == -1) {
                goto error;
        }
        /* restore previous instruction. */
        p[0] = bp->data;
        if (write_disable (bp->address) == -1) {
                goto error;
        }
        return 0;
 error:
        return -1;        
}

int 
breakpoint_disable_from_handler (struct breakpoint const *bp, 
                                 ucontext_t *user_ctx)
{
        /* Restore the instruction previously there.
         */
        if (breakpoint_disable (bp) == -1) {
                goto error;
        }
        /* Make sure to re-execute the instruction we just restored 
         * upon return to the user stack.
         */
        user_ctx_set_pc (user_ctx, user_ctx_get_pc (user_ctx)-1);

        return 0;
 error:
        return -1;
}

static struct sigaction g_old_action;
static struct breakpoint const *g_bp;

/**
 * We are here thanks to the steping mode so 
 * We re-enable the breakpoint from this handler,
 * disable the stepping mode and re-enable the
 * original handler.
 */
static void 
reenable_trap_handler (int signo, siginfo_t *info, ucontext_t *user_ctx)
{
        /* Disable step by step. */
        user_ctx_disable_step (user_ctx);
        /* re-enable old action handler. */
        if (sigaction (SIGTRAP, &g_old_action, NULL) == -1) {
                goto error;
        }
        /* re-enable breakpoint. */
        breakpoint_enable ((struct breakpoint *)g_bp);
 error:
        return;
}

int 
breakpoint_continue_from_handler (struct breakpoint const *bp, 
                                  ucontext_t *user_ctx)
{
        if (breakpoint_disable_from_handler (bp, user_ctx) == -1) {
                goto error;
        }
        
        /* save old action handler and install the re-enable action handler. */
        {
                struct sigaction action;
                action.sa_sigaction = (action_callback_t)reenable_trap_handler;
                action.sa_flags = SA_SIGINFO;
                g_bp = bp;
                if (sigaction (SIGTRAP, &action, &g_old_action) == -1) {
                        printf ("could not install trap signal handler\n");
                        goto error;
                }
        }

        /* schedule a task breakpoint on the next instruction
         * to re-enable the breakpoint at that point.
         */
        user_ctx_enable_step (user_ctx);

        return 0;
 error:
        return -1;
}

