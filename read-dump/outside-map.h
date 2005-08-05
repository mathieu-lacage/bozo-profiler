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

#ifndef OUTSIDE_MAP_H
#define OUTSIDE_MAP_H

#include <stdint.h>
#include "load-map.h"

/* It is important to call the append 
 * function in the right order. Typically,
 * the entry corresponding to the main binary
 * should be appended first such that we 
 * can parse the the load map in the right
 * order later.
 */
void outside_map_append (struct load_map *map, 
                         uint32_t base_address, 
                         char const *filename);

void outside_map_reset (struct load_map *map);

void outside_map_initialize (struct load_map *map);


#endif /* OUTSIDE_MAP_H */
