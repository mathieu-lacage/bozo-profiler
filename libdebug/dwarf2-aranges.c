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

#include "dwarf2-aranges.h"
#include "reader.h"

#define ADDR_CUH_SIZE (4+2+4+1+1)
#define ADDR_CU_SIZE(length) (4+length)
#define ADDR_TUPLE_SIZE(address_size) (address_size*2)

int 
dwarf2_aranges_read_all (uint32_t start, 
			 uint32_t end,
                         int (*callback) (uint32_t cuh_offset, 
                                          uint32_t start, 
                                          uint32_t size, 
                                          void *context),
                         void *data,
			 struct reader *reader)
{
        reader->seek (reader, start);
        while (reader->get_offset (reader) < end) {
                uint32_t length;
                uint16_t version;
                uint32_t offset;
                uint8_t address_size;
                uint8_t segment_descriptor_size;

                uint32_t tuple_offset, end_offset;
                uint32_t cu_current;

                length = reader->read_u32 (reader);
                version = reader->read_u16 (reader);
                offset = reader->read_u32 (reader);
                address_size = reader->read_u8 (reader);
                segment_descriptor_size = reader->read_u8 (reader);
                if (address_size > 4 || segment_descriptor_size > 4) {
                        /* TARGET_PTR */
                        /* We do not want to handle targets whose address
                         * space is larger than 32 bits (i.e., 4 bytes).
                         */
                        goto error;
                }
                tuple_offset = ADDR_TUPLE_SIZE (address_size) - (ADDR_CUH_SIZE % ADDR_TUPLE_SIZE (address_size));
                reader->skip (reader, tuple_offset);
                cu_current = ADDR_CUH_SIZE + tuple_offset;
                while (cu_current < length + 4) {
                        uint32_t tu_address; /* TARGET_PTR */
                        uint32_t tu_delta; /* TARGET_PTR */
                        tu_address = reader->read_u (reader, address_size);
                        tu_delta = reader->read_u (reader, address_size);
                        if ((*callback) (offset, tu_address, tu_delta, data) == 1) {
                                goto ok;
                        }
                        cu_current += ADDR_TUPLE_SIZE (address_size);
                }
                /* adjust to the end of the compilation unit to be able to
                 * read the following compilation unit header.
                 */
                end_offset = ADDR_CU_SIZE (length) - cu_current;
                reader->skip (reader, end_offset);
        }

        if (reader->status == -1) {
                goto error;
        }

 ok:
        return 0;
 error:
        return -1;

}

