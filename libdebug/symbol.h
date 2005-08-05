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

#ifndef SYMBOL_H
#define SYMBOL_H

#include <stdint.h>

#include "elf32-parser.h"
#include "dwarf2-parser.h"
#include "load-map.h"

/*
 * Return != 0 if you want to stop the iteration.
 */
typedef int (*symbol_iter_cb) (struct elf32_symbol const *symbol, uint64_t address, char const *real_name, void *context);

/**
 * This function performs a symbol lookup to find where
 * this symbol is defined. The symbol lookup is implemented
 * by parsing the link map and trying to find a non-UNDEF
 * entry in the symbol table for this symbol name.
 * It should be pointed out that there can be more than one
 * address for a given symbol because of symbol versionning.
 * Returns -1 on error.
 */
int symbol_iterate_definitions (struct load_map *map, char const *name, symbol_iter_cb cb, void *context);

/**
 * This function performs a reverse symbol lookup and returns the list 
 * of symbols which contain the input pointer.
 */
int symbol_iterate_names (struct load_map *map, uint64_t address, symbol_iter_cb cb, 
                          void *context);




#endif /* SYMBOL_H */
