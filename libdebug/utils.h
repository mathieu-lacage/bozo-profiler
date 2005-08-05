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

#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include "memory-reader.h"
#include "load-map.h"
#include "elf32-parser.h"

struct link_map;

struct link_map *utils_get_link_map (void);
struct r_debug *utils_get_ld_debug(void);
int utils_copy_process_name (char *str, uint32_t size);
int utils_filename_to_memory_reader (char const *filename, struct memory_reader *reader);
mbool utils_goto_map_entry (struct load_map *map, uint64_t address);
int utils_get_iterator_for_filename (char const *filename, 
                                     struct elf32_symbol_iterator *i,
                                     struct elf32_header *elf32, 
                                     struct memory_reader *reader);


#endif /* UTILS_H */
