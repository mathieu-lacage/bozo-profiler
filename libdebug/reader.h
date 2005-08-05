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

#ifndef READER_H
#define READER_H

#include <stdint.h>

struct reader {
        /* status < 0 in case of error. */
        int status;
        void     (*set_msb)      (struct reader *reader);
        void     (*set_lsb)      (struct reader *reader);
        uint8_t  (*read_u8)      (struct reader *reader);
        uint16_t (*read_u16)     (struct reader *reader);
        uint32_t (*read_u32)     (struct reader *reader);
        uint64_t (*read_u64)     (struct reader *reader);
        int8_t   (*read_s8)      (struct reader *reader);
        int16_t  (*read_s16)     (struct reader *reader);
        int32_t  (*read_s32)     (struct reader *reader);
        uint32_t (*read_u)       (struct reader *reader, uint8_t length);
        uint64_t (*read_uleb128) (struct reader *reader);
        int64_t  (*read_sleb128) (struct reader *reader);
        uint8_t  (*read_u8bcd)   (struct reader *reader);
        uint16_t (*read_u16bcd)  (struct reader *reader);

        uint32_t (*get_offset)   (struct reader *reader);
        /* absolute offset. */
        void     (*seek)         (struct reader *reader, uint32_t offset);
        /* relative offset. */
        void     (*skip)         (struct reader *reader, uint32_t offset);
        void     (*skip64)       (struct reader *reader, uint64_t offset);
};

#define READER(x) ((struct reader *)x)

#endif /* READER_H */
