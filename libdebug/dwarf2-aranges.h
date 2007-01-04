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

  Copyright (C) 2004,2005  Mathieu Lacage
  Author: Mathieu Lacage <mathieu@gnu.org>
*/
#ifndef DWARF2_ARANGES_H
#define DWARF2_ARANGES_H

#include <stdint.h>

struct reader;

/**
 * @start offset from start of file of .debug_aranges section
 * @end offset from start of file of .debug_aranges section
 * @callback callback to invoke on each entry. 
 *           cuh_offset: offset from start of .debug_info section to
 *                       cuh header
 *           start: address in virtual address space.
 *           end: address in virtual address space.
 *           Return 1 to stop iteration.
 * @data context to pass to callback
 * @reader reader to read the data
 */
int dwarf2_aranges_read_all (uint32_t start, 
                             uint32_t end, 
                             int (*callback) (uint32_t cuh_offset,
                                              uint32_t start, 
                                              uint32_t size, 
                                              void *context),
                             void *data,
                             struct reader *reader);


#endif /* DWARF2_ARANGES_H */
