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

#include "load-map.h"
#include "null.h"
#include "utils.h"

#define _GNU_SOURCE
#include <link.h>
#include <string.h>


static char const *
get_filename (struct load_map *map)
{
        struct link_map *linux_map;
        linux_map = (struct link_map *)map->priv_current;
        if (strcmp (linux_map->l_name, "") == 0) {
                return "/proc/self/exe";
        } else {
                return linux_map->l_name;
        }
}


static uint32_t 
get_base_address (struct load_map *map)
{
        struct link_map *linux_map;
        linux_map = (struct link_map *)map->priv_current;
        return linux_map->l_addr;
}

static mbool 
has_next (struct load_map *map)
{
        struct link_map *linux_map;
        linux_map = (struct link_map *)map->priv_current;
        if (linux_map == NULL) {
                return FALSE;
        } else {
                return TRUE;
        }
}

static void
next (struct load_map *map)
{
        struct link_map *linux_map;
        linux_map = (struct link_map *)map->priv_current;
        map->priv_current = linux_map->l_next;
}

static void
first (struct load_map *map)
{
        map->priv_current = map->priv_first;
}


void 
load_map_linux_initialize (struct load_map *map)
{
        struct link_map *linux_map;
        linux_map = utils_get_link_map ();
        map->get_filename = get_filename;
        map->get_base_address = get_base_address;
        map->has_next = has_next;
        map->next = next;
        map->first = first;
        map->priv_current = linux_map;
        map->priv_first = linux_map;
}
