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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>


extern int test_ti (int argc, char *argv[]);
extern int test_breakpoint (int argc, char *argv[]);
extern int test_fn_address (int argc, char *argv[]);
extern int test_circular_buffer (int argc, char *argv[]);
extern int test_dwarf2 (int argc, char *argv[]);
extern int test_ld_brk (int argc, char *argv[]);
extern int test_opcode (int argc, char *argv[]);
extern int test_performance (int argc, char *argv[]);
extern int test_profiler (int argc, char *argv[]);
extern int test_symbol (int argc, char *argv[]);
extern int test_thread_db (int argc, char *argv[]);
extern int test_dw2_bb (int argc, char *argv[]);



struct test_list {
        struct test_list *next;
        char const *name;
        char const *params;
        int (*run_test) (int argc, char *argv[]);
} * g_test_list = NULL;

static void 
prepend_test (char const *test_name,
              char const *param_string,
              int (*run_test) (int argc, char *argv[]))
{
        assert (test_name != NULL);
        assert (run_test != NULL);
        struct test_list *item;
        item = malloc (sizeof (struct test_list));
        assert (item != NULL);
        item->next = g_test_list;
        item->name = strdup (test_name);
        if (param_string != NULL) {
                item->params = strdup (param_string);
        } else {
                item->params = NULL;
        }
        item->run_test = run_test;
        g_test_list = item;
}

static void
print_tests (void)
{
        struct test_list *tmp;
        for (tmp = g_test_list; tmp != NULL; tmp = tmp->next) {
                if (tmp->params != NULL) {
                        printf ("\t%s %s\n", tmp->name, tmp->params);
                } else {
                        printf ("\t%s\n", tmp->name);
                }
        }
}

static int
run_test (char const *name, int argc, char *argv[])
{
        struct test_list *tmp;
        for (tmp = g_test_list; tmp != NULL; tmp = tmp->next) {
                if (strcmp (name, tmp->name) == 0) {
                        return (*tmp->run_test) (argc, argv);
                }
        }
        return -1;
}

int main (int argc, char *argv[])
{
        char *test_name = argv[1];

        prepend_test ("ti", "[file]", test_ti);
        prepend_test ("breakpoint", NULL, test_breakpoint);
        prepend_test ("fn_address", NULL, test_fn_address);
        prepend_test ("circular_buffer", NULL, test_circular_buffer);
        prepend_test ("dwarf2", "[file]", test_dwarf2);
        prepend_test ("ld_brk", NULL, test_ld_brk);
        prepend_test ("opcode", "[file]", test_opcode);
        prepend_test ("performance", NULL, test_performance);
        prepend_test ("profiler", NULL, test_profiler);
        prepend_test ("symbol", NULL, test_symbol);
        prepend_test ("thread_db", NULL, test_thread_db);
        prepend_test ("dw2_bb", "[file]", test_dw2_bb);

        if (argc <= 1) {
                printf ("usage: %s [test_name] [test_options]\n", argv[0]);
                printf ("available tests:\n");
                print_tests ();
                return 0;
        }
        argc-= 2;
        argv+= 2;

        return run_test (test_name, argc, argv);
}
