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

#ifndef DWARF2_CACHE_H
#define DWARF2_CACHE_H

#include <stdint.h>
#include "dwarf2-parser.h"

void dwarf2_cache_initialize (void);

/* return an owned symbol_data string. */
int  dwarf2_cache_lookup (uint64_t key, char **symbol_data, uint64_t *line);

/* take ownership of the symbol_data string. */
void dwarf2_cache_add (uint64_t key, char *symbol_data, uint64_t line);

/* flush cache */
void dwarf2_cache_flush (void);


#endif /* DWARF2_CACHE_H */
