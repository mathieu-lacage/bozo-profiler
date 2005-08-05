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
#include "null.h"
#include "memory-reader.h"
#include "elf32-parser.h"
#include "mmap-cache.h"
#include "utils.h"

#define _GNU_SOURCE
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <string.h>
#include <stdio.h>


#define ASCII_AT (0x40)

/**
 * The idea of symbol name matching is a bit complex because
 * it does not correspond to string equality. This is done
 * to match multiple versions of the same symbol.
 */
static int
symbol_matches (char const *searched, char const *name)
{
        while (*searched != 0 && *name != 0) {
                if (*searched != *name) {
                        return 0;
                }
                searched++;
                name++;
        }
        if (*searched == 0 && *name == 0) {
                return 1;
        } else if (*searched == 0 && *name == ASCII_AT) {
                return 1;
        } else {
                return 0;
        }
}

int 
symbol_iterate_definitions (struct load_map *map,
                            char const *name,
                            symbol_iter_cb cb,
                            void *context)
{
        struct memory_reader reader;
        struct elf32_header elf32;
        struct elf32_symbol_iterator i;
        uint64_t address;

        for (map->first (map); map->has_next (map); map->next (map)) {
                mbool defined;
                if (utils_get_iterator_for_filename (map->get_filename (map), &i, &elf32, &reader) == -1) {
                        goto error;
                }
                while (elf32_symbol_iterator_has_next (&i)) {
                        if (i.symbol.st_shndx == SHN_ABS) {
                                // I don't know what to do here.
                                //printf ("XXX ABS in %s for %s\n", iter->map->l_name, name);
                                address = 0;
                                defined = FALSE;
                        } else if (i.symbol.st_shndx > SHN_UNDEF &&
                                   i.symbol.st_shndx < SHN_LORESERVE) {
                                /* symbol is defined in this binary file in section number st_shndx.
                                   This section is located at address sh_addr+base_address.
                                   So, the symbol is located at address sh_addr+st_value.
                                */
                                address = map->get_base_address (map) + i.symbol.st_value;
                                defined = TRUE;
                        } else if (ELF32_ST_TYPE (i.symbol.st_info) == STT_FUNC &&
                                   i.symbol.st_value != 0) {
                                // this entry is "special". i.e., it is the address
                                // of the PLT entry associated to this function.
                                // The spec says to use it as the symbol address.
                                // see abi i386 elf spec, (draft copy, march 19 1997) page 87.
                                // see also page 76.
                                address = map->get_base_address (map) + i.symbol.st_value;
                                defined = TRUE;
                        } else {
                                // symbol is undefined here.
                                defined = FALSE;
                        }

                        if (defined) {
                                char const *real_name;
                                real_name = reader.buffer + i.name_offset;
                                if (symbol_matches (real_name, name)) {
                                        int retval;
                                        retval = (*cb) (&i.symbol, address, name, context);
                                        if (retval) {
                                                break;
                                        }
                                }
                        }

                        elf32_symbol_iterator_next (&i, READER (&reader));
                }
                
        }

        if (reader.reader.status < 0) {
                goto error;
        }

        return 0;
 error:
        return -1;
}

int 
symbol_iterate_names (struct load_map *map, uint64_t address, 
                      symbol_iter_cb cb, 
                      void *context)
{
        struct memory_reader reader;
        struct elf32_header elf32;
        struct elf32_symbol_iterator i;

        if (!utils_goto_map_entry (map, address)) {
                goto error;
        }

        address -= map->get_base_address (map);

        if (utils_get_iterator_for_filename (map->get_filename (map),
                                             &i, &elf32, &reader) == -1) {
                goto error;
        }
        while (elf32_symbol_iterator_has_next (&i)) {
                if (address >= i.symbol.st_value &&
                    address < i.symbol.st_value + i.symbol.st_size) {
                        char const *real_name;
                        real_name = reader.buffer + i.name_offset;
                        cb (&i.symbol, address, real_name, context);
                }
                elf32_symbol_iterator_next (&i, READER (&reader));
        }

        if (reader.reader.status < 0) {
                goto error;
        }

        return 0;
 error:
        return -1;

}



