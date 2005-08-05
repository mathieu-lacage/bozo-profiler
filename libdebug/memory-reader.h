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

#ifndef MEMORY_READER_H
#define MEMORY_READER_H

#include "reader.h"
#include <stdint.h>

struct memory_reader {
        struct reader reader;
        uint8_t *buffer;
        uint32_t size;
        uint32_t current;
        int lsb;
};

void memory_reader_initialize (struct memory_reader *reader, uint8_t *buffer, uint32_t size);

/* sub_memory_reader is initialized to read a subset of the input reader.
 * If an inconsistancy is detected during initialization, the sub_memory_reader
 * is initialized in a bad status state.
 */
void memory_reader_sub_initialize (struct memory_reader const *reader, struct memory_reader *sub_memory_reader, uint32_t size);



#endif /* MEMORY_READER_H */
