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

#ifndef BREAKPOINT_H
#define BREAKPOINT_H

#include <stdint.h>
#include <sys/ucontext.h>

struct breakpoint {
        long address;
        uint8_t data;
};

/* returns -1 on error */
int breakpoint_enable (struct breakpoint *bp);
int breakpoint_disable (struct breakpoint const *bp);
int breakpoint_disable_from_handler (struct breakpoint const *bp, ucontext_t *user_ctx);
int breakpoint_continue_from_handler (struct breakpoint const *bp, ucontext_t *user_ctx);

#endif /* BREAKPOINT_H */
