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

  Copyright (C) 2005 Mathieu Lacage
  Author: Mathieu Lacage <mathieu@gnu.org>
*/

#include "timestamp.h"

#include <stdint.h>

uint64_t 
timestamp_read (void)
{
        uint64_t time;
        __asm__ volatile (".byte 0x0f, 0x31" : "=A"(time));
        return time;
}
