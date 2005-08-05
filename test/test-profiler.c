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

#include "mbool.h"

#include <dlfcn.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>



int foo ()
{
	return 3;
}

int main (int argc, char *argv[])
{
        void *h;
        void (*f) (void);
	int v = foo ();

	h = dlopen ("test/libtest-fn-address.so", RTLD_LAZY);

	printf ("forking\n");
	//fork ();

	foo ();
	
	f = (void (*) (void))dlsym (h, "fn_address_foo");

	(*f) ();


	while (v > 0) {sleep (1);v--;}

	return v;
}
