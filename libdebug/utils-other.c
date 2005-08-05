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

#include <stdint.h>
#include <stdio.h>

#include "utils.h"
#include "elf32-parser.h"
#include "load-map.h"
#include "memory-reader.h"

mbool 
utils_goto_map_entry (struct load_map *map, uint64_t address)
{
        struct memory_reader reader;
        struct elf32_header elf32;

        for (map->first (map); map->has_next (map); map->next (map)) {
                uint32_t start, end;
                
                if (utils_filename_to_memory_reader (map->get_filename (map), &reader) == -1) {
                        printf ("cannot get parser.\n");
                        goto error;
                }
                if (elf32_parser_initialize (&elf32, READER (&reader)) == -1) {
                        goto error;
                }
                if (elf32_parser_get_bounds (&elf32, &start, &end, READER (&reader)) == -1) {
                        printf ("unable to get bounds.\n");
                        goto error;
                }
                start += map->get_base_address (map);
                end += map->get_base_address (map);
                if (address >= start && address <= end) {
                        //printf ("\nfound 0x%llx in %s\n", address, map->get_filename (map));
                        return TRUE;
                }
        }

 error:
        return FALSE;
}
