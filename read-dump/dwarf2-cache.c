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

  Copyright (C) 2005  INRIA
  Author: Mathieu Lacage <lacage@sophia.inria.fr>
*/

#include "dwarf2-cache.h"

#include <glib.h>

struct cache_entry {
        char *symbol;
        uint64_t line;
};

static GHashTable *g_hash = NULL;
static int n_hits = 0;
static int n_misses = 0;

static void
key_destroy (gpointer data)
{}
static void
value_destroy (gpointer data)
{
        struct cache_entry *entry;
        entry = (struct cache_entry *) data;
        g_free (entry->symbol);
        g_free (data);
}
static void
renew_hash (void)
{
        if (g_hash_table_size (g_hash) >= 10000) {
                dwarf2_cache_flush ();
        }
}

#include <stdio.h>
void 
dwarf2_cache_flush (void)
{
        fprintf (stderr, "hits: %d, misses: %d\n", n_hits, n_misses);
        g_hash_table_destroy (g_hash);
        dwarf2_cache_initialize ();
}

void 
dwarf2_cache_initialize (void)
{
        g_hash = g_hash_table_new_full (NULL, NULL, key_destroy, value_destroy);
        n_hits = 0;
        n_misses = 0;
}


int 
dwarf2_cache_lookup (uint64_t key, char **symbol_data, uint64_t *line)
{
        struct cache_entry *entry;
        unsigned long small_key = key;
        entry = (struct cache_entry *)g_hash_table_lookup (g_hash, (gconstpointer)small_key);
        if (entry == NULL) {
                n_misses++;
                return -1;
        } else {
                *symbol_data = entry->symbol;
                *line = entry->line;
                n_hits++;
                return 0;
        }
        
}

void
dwarf2_cache_add (uint64_t key, char *symbol_data, uint64_t line)
{
        struct cache_entry *entry;
        unsigned long small_key = key;
        entry = g_new0 (struct cache_entry, 1);
        entry->symbol = symbol_data;
        entry->line = line;
        renew_hash ();
        g_hash_table_insert (g_hash, (gpointer)small_key, entry);
}
