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

#include "outside-map.h"
#include "load-map.h"
#include "null.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct outside_load_map {
        uint32_t base_address;
        char *filename;
        struct outside_load_map *next;
};

static char const *
get_filename (struct load_map *map)
{
        struct outside_load_map *outside;
        outside = (struct outside_load_map *)map->priv_current;
        return outside->filename;
}


static uint32_t 
get_base_address (struct load_map *map)
{
        struct outside_load_map *outside;
        outside = (struct outside_load_map *)map->priv_current;
        return outside->base_address;
}

static mbool 
has_next (struct load_map *map)
{
        struct outside_load_map *outside;
        outside = (struct outside_load_map *)map->priv_current;
        if (outside == NULL) {
                return FALSE;
        } else {
                return TRUE;
        }
}

static void
next (struct load_map *map)
{
        struct outside_load_map *outside;
        outside = (struct outside_load_map *)map->priv_current;
        map->priv_current = outside->next;
}

static void
first (struct load_map *map)
{
        map->priv_current = map->priv_first;
}


void 
outside_map_append (struct load_map *map, 
                         uint32_t base_address, 
                         char const *filename)
{
        struct outside_load_map *new_map, *end;
        new_map = malloc (sizeof (*new_map));
        if (new_map == NULL) {
                printf ("malloc failed\n");
                return;
        }
        new_map->base_address = base_address;
        new_map->filename = strdup (filename);
        new_map->next = NULL;
        if (map->priv_first == NULL) {
                map->priv_first = new_map;
                return;
        }
        end = map->priv_first;
        while (end->next != NULL) {
                end = end->next;
        }
        end->next = new_map;
}

void 
outside_map_reset (struct load_map *map)
{
        struct outside_load_map *tmp, *prev;
        for (tmp = (struct outside_load_map *)map->priv_first, prev = NULL; 
             tmp != NULL; prev = tmp, tmp = tmp->next) {
                if (prev != NULL) {
                        free (prev->filename);
                        free (prev);
                }
        }
        map->priv_first = NULL;
        map->priv_current = NULL;
}

void 
outside_map_initialize (struct load_map *map)
{
        map->get_filename = get_filename;
        map->get_base_address = get_base_address;
        map->next = next;
        map->first = first;
        map->has_next = has_next;
        map->priv_first = NULL;
        map->priv_current = NULL;
}
