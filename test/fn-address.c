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

#include "fn-address.h"

#include <stdio.h>

static void my_const (void) __attribute__ ((constructor));

static void my_const (void)
{
        printf ("constructor\n");
}

void fn_address_foo (void)
{}

/* return -1 if the address is not what is expected. */
int fn_address_test (void(*fn)(void))
{
        if (fn != fn_address_foo) {
                printf ("not equal: %p -- %p\n", fn, fn_address_foo);
                return -1;
        }
        //fn_address_foo ();
        printf ("%p\n", fn_address_foo);
        return 0;
}
