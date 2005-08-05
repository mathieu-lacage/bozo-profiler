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

  Copyright (C) 2004,2005 Mathieu Lacage
  Author: Mathieu Lacage <mathieu@gnu.org>
*/

#define _GNU_SOURCE  /* for REG_EIP */

#include <sys/ucontext.h>

#include "user-ctx.h"

#define TF_N 8

unsigned long 
user_ctx_get_pc (ucontext_t *user_ctx)
{
        return user_ctx->uc_mcontext.gregs[REG_EIP];
}

void 
user_ctx_set_pc (ucontext_t *user_ctx, unsigned long pc)
{
        user_ctx->uc_mcontext.gregs[REG_EIP] = pc;
}

void user_ctx_enable_step (ucontext_t *user_ctx)
{
        user_ctx->uc_mcontext.gregs[REG_EFL] |= (1<<TF_N);
}

void user_ctx_disable_step (ucontext_t *user_ctx)
{
        user_ctx->uc_mcontext.gregs[REG_EFL] &= ~(1<<TF_N);
}
