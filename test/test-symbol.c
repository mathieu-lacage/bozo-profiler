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

#include "symbol.h"
#include "fn-address.h"
#include "null.h"
#include "load-map.h"
#include <pthread.h>
#include <stdio.h>


static int 
definitions_cb (struct elf32_symbol const *symbol, uint64_t address, char const *real_name, void *context)
{
        uint64_t *ptr = (uint64_t *)context;
        printf ("def %s at %llu\n", real_name, address);
        *ptr = address;
        return 0;
}

static int
names_cb (struct elf32_symbol const *symbol, uint64_t address, char const *real_name, void *context)
{
        printf ("name %s at %llu\n", real_name, address);
        return 0;
}


void test_symbol_foo (void) {}

void test_symbol_bar (void) {}

#include <sys/types.h>
#include <unistd.h>


int
test_symbol (int argc, char *argv[])
{
        struct load_map map;
        uint64_t address;

        load_map_linux_initialize (&map);

        if (symbol_iterate_definitions (&map, "test_symbol_foo", definitions_cb, &address) == -1) {
                goto error;
        }
        if (symbol_iterate_names (&map, address, names_cb, NULL) == -1) {
                goto error;
        }


        if (symbol_iterate_definitions (&map, "fn_address_foo", definitions_cb, &address) == -1) {
                goto error;
        }
        if (symbol_iterate_names (&map, address, names_cb, NULL) == -1) {
                goto error;
        }


        if (symbol_iterate_definitions (&map, "getpid", definitions_cb, &address) == -1) {
                goto error;
        }
        if (symbol_iterate_names (&map, address, names_cb, NULL) == -1) {
                goto error;
        }


        if (symbol_iterate_definitions (&map, "pthread_create", definitions_cb, &address) == -1) {
                goto error;
        }
        if (symbol_iterate_names (&map, address, names_cb, NULL) == -1) {
                goto error;
        }

        return 0;
 error:
        return -1;
}
