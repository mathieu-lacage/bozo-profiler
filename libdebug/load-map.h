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

#ifndef LOAD_MAP_H
#define LOAD_MAP_H

#include <stdint.h>
#include "mbool.h"

/* The first item in the load map is
 * expected to represent the main 
 * binary. 
 */

struct load_map {
        char const *(*get_filename) (struct load_map *map);
        uint32_t (*get_base_address) (struct load_map *map);
        mbool (*has_next) (struct load_map *map);
        void (*next) (struct load_map *map);
        void (*first) (struct load_map *map);
        void *priv_current;
        void *priv_first;
};

void load_map_linux_initialize (struct load_map *map);

#endif /* LOAD_MAP_H */
