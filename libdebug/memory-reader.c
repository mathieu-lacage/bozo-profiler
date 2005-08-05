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

#include "reader.h"
#include "memory-reader.h"
#include <stdint.h>
#include "mbool.h"

#define no_DEBUG_REPORT

#ifdef yes_DEBUG_REPORT
  #define DEBUG_REPORT(str, ...) \
    printf ("DEBUG "  __FILE__  ":%d (%s) " str "\n", __LINE__, __func__, ## __VA_ARGS__);
#else
  #define DEBUG_REPORT(str, ...)
#endif


#define ENSURE_BYTES_LEFT(reader, n) \
((reader->size - reader->current) >= n)


static void
memory_reader_set_msb  (struct memory_reader *reader)
{
        reader->lsb = FALSE;
}

static void 
memory_reader_set_lsb  (struct memory_reader *reader)
{
        reader->lsb = TRUE;
}

static uint8_t 
memory_reader_read_u8  (struct memory_reader *reader)
{
        uint8_t retval;
        if (!ENSURE_BYTES_LEFT (reader, 1)) {
                DEBUG_REPORT ("not a single byte left");
                goto error;
        }
        retval = reader->buffer[reader->current];
        reader->current++;
        return retval;
 error:
        reader->reader.status = -1;
        return 0xff;
}

static uint16_t 
memory_reader_read_u16 (struct memory_reader *reader)
{
        uint16_t retval;
        uint8_t byte0;
        uint8_t byte1;
        if (!ENSURE_BYTES_LEFT (reader, 2)) {
                DEBUG_REPORT ("2 bytes not left");
                goto error;
        }
        byte0 = reader->buffer[reader->current];
        reader->current++;
        byte1 = reader->buffer[reader->current];
        reader->current++;

        if (reader->lsb) {
                retval = byte0 + (byte1<<8);
        } else {
                retval = (byte0<<8) + byte1;
        }

        return retval;
 error:
        reader->reader.status = -1;
        return 0xffff;
}

static uint32_t 
memory_reader_read_u32 (struct memory_reader *reader)
{
        uint32_t retval;
        uint8_t byte0;
        uint8_t byte1;
        uint8_t byte2;
        uint8_t byte3;
        if (!ENSURE_BYTES_LEFT (reader, 4)) {
                DEBUG_REPORT ("4 bytes not left");
                goto error;
        }
        byte0 = reader->buffer[reader->current];
        reader->current++;
        byte1 = reader->buffer[reader->current];
        reader->current++;
        byte2 = reader->buffer[reader->current];
        reader->current++;
        byte3 = reader->buffer[reader->current];
        reader->current++;

        if (reader->lsb) {
                retval = byte0 + (byte1<<8) + (byte2<<16) + (byte3<<24);
        } else {
                retval = (byte0<<24) + (byte1<<16) + (byte2<<8) + byte3;
        }

        return retval;
 error:
        reader->reader.status = -1;
        return 0xffffffff;
}

static uint64_t 
memory_reader_read_u64 (struct memory_reader *reader)
{
        uint64_t retval;
        uint64_t byte0;
        uint64_t byte1;
        uint64_t byte2;
        uint64_t byte3;
        uint64_t byte4;
        uint64_t byte5;
        uint64_t byte6;
        uint64_t byte7;

        if (!ENSURE_BYTES_LEFT (reader, 8)) {
                DEBUG_REPORT ("8 bytes not left");
                goto error;
        }
        byte0 = reader->buffer[reader->current];
        reader->current++;
        byte1 = reader->buffer[reader->current];
        reader->current++;
        byte2 = reader->buffer[reader->current];
        reader->current++;
        byte3 = reader->buffer[reader->current];
        reader->current++;
        byte4 = reader->buffer[reader->current];
        reader->current++;
        byte5 = reader->buffer[reader->current];
        reader->current++;
        byte6 = reader->buffer[reader->current];
        reader->current++;
        byte7 = reader->buffer[reader->current];
        reader->current++;

        if (reader->lsb) {
                retval = byte0 + (byte1<<8) + (byte2<<16) + (byte3<<24) 
                        + (byte4<<32) + (byte5<<40) + (byte6<<48) + (byte7<<56);
        } else {
                retval = (byte0<<56) + (byte1<<48) + (byte2<<40) + (byte3<<32) 
                        + (byte4<<24) + (byte5<<16) + (byte6<<8) + (byte7<<0);
        }

        return retval;
 error:
        reader->reader.status = -1;
        return 0xffffffffffffffffLL;
        
}


static uint32_t 
memory_reader_read_u (struct memory_reader *reader, uint8_t length)
{
        uint32_t retval;
        if (length == 1) {
                retval = memory_reader_read_u8 (reader);
        } else if (length == 2) {
                retval = memory_reader_read_u16 (reader);
        } else if (length == 4) {
                retval = memory_reader_read_u32 (reader);
        } else {
                retval = memory_reader_read_u64 (reader);
        }
        return retval;

}

static uint64_t 
memory_reader_read_uleb128 (struct memory_reader *reader)
{
        uint64_t result;
        uint8_t byte;
        uint8_t shift;
        result = 0;
        shift = 0;
        do {
                if (!ENSURE_BYTES_LEFT (reader, 1)) {
                        DEBUG_REPORT ("not a single byte left");
                        goto error;
                }
                byte = reader->buffer[reader->current];
                reader->current++;
                result |= (byte & (~0x80))<<shift;
                shift += 7;
        } while (byte & 0x80 && 
                 /* a LEB128 unsigned number is at most 9 bytes long. */
                 shift < (7*9)); 
        if (byte & 0x80) {
                /* This means the LEB128 number was not valid.
                 * ie: the last (9th) byte did not have the high-order bit zeroed.
                 */
                reader->reader.status = -1;
        }
        return result;
 error:
        reader->reader.status = -1;
        return (uint64_t)-1;
}

static int64_t 
memory_reader_read_sleb128 (struct reader *reader)
{
        int64_t result; 
        uint8_t shift;
        uint8_t byte;
        result = 0;
        shift = 0;
        while(TRUE) { 
                byte = reader->read_u8 (reader);
                result |= (((uint64_t)(byte & 0x7f)) << shift); 
                shift += 7; /* sign bit of byte is 2nd high order bit (0x40) */ 
                if ((byte & 0x80) == 0) {
                        break; 
                }
        } 
        if ((shift < 64) && (byte & 0x40)) {
                /* sign extend */ 
                result |= - (1 << shift);
        }
        return result;
}


static int8_t 
memory_reader_read_s8  (struct memory_reader *reader)
{
        return (int8_t) memory_reader_read_u8 (reader);
}

static int16_t 
memory_reader_read_s16 (struct memory_reader *reader)
{
        return (int16_t) memory_reader_read_u16 (reader);
}

static int32_t 
memory_reader_read_s32 (struct memory_reader *reader)
{
        return (int32_t) memory_reader_read_u32 (reader);
}


static uint8_t
memory_reader_read_u8bcd (struct memory_reader *reader)
{
        uint8_t retval;
        uint8_t byte, bcd0, bcd1;
        if (!ENSURE_BYTES_LEFT (reader, 1)) {
                DEBUG_REPORT ("1 byte not left");
                goto error;
        }
        byte = reader->buffer[reader->current];
        bcd0 = byte & 0xf;
        bcd1 = byte >> 4;
        reader->current++;

        // XXX Probably wrong. I have no idea what I am doing here.
        if (reader->lsb) {
                retval = bcd0 + bcd1*10;
        } else {
                retval = bcd0 + bcd1*10;
        }

        return retval;
 error:
        reader->reader.status = -1;
        return 0xff;
}

static uint16_t
memory_reader_read_u16bcd (struct memory_reader *reader)
{
        uint16_t retval;
        uint8_t byte, bcd0, bcd1, bcd2, bcd3;
        if (!ENSURE_BYTES_LEFT (reader, 2)) {
                DEBUG_REPORT ("2 bytes not left");
                goto error;
        }
        byte = reader->buffer[reader->current];
        bcd0 = byte & 0xf;
        bcd1 = byte >> 4;
        reader->current++;
        byte = reader->buffer[reader->current];
        bcd2 = byte & 0xf;
        bcd3 = byte >> 4;
        reader->current++;

        if (reader->lsb) {
                // XXX untested. Probably wrong.
                retval = bcd0 + bcd1*10 + bcd2*100 + bcd3*1000;
        } else {
                retval = bcd1*1000 + bcd0*100 + bcd3*10 + bcd2;
        }

        return retval;
 error:
        reader->reader.status = -1;
        return 0xffff;
}




static uint32_t
memory_reader_get_offset (struct memory_reader *reader)
{
        return reader->current;
}

static void
memory_reader_seek (struct memory_reader *reader, uint32_t offset)
{
        if (reader->size < offset) {
                DEBUG_REPORT ("seek is too big for file size: %d < %d", reader->size, offset);
                goto error;
        }
        reader->current = offset;
        return;
 error:
        reader->reader.status = -1;
}

static void
memory_reader_skip (struct memory_reader *reader, uint32_t offset)
{
        if (!ENSURE_BYTES_LEFT (reader, offset)) {
                DEBUG_REPORT ("skip: %d bytes not left", offset);
                goto error;
        }
        reader->current += offset;
        return;
 error:
        reader->reader.status = -1;
}
static void
memory_reader_skip64 (struct memory_reader *reader, uint64_t offset)
{
        if (!ENSURE_BYTES_LEFT (reader, offset)) {
                DEBUG_REPORT ("skip: %llu bytes not left", offset);
                goto error;
        }
        /* XXX Of course, this code will work only if ->current is 
         * 64 bits large which is not the case for now and is 
         * unlikely to ever be the case.
         */
        reader->current += offset;
        return;
 error:
        reader->reader.status = -1;
}


void 
memory_reader_sub_initialize (struct memory_reader const *reader, 
                              struct memory_reader *sub_memory_reader, 
                              uint32_t size)
{
        if (!ENSURE_BYTES_LEFT (reader, size)) {
                DEBUG_REPORT ("sub init: %d bytes not left", size);
                goto error;
        }
        *sub_memory_reader = *reader;
        sub_memory_reader->buffer = reader->buffer + reader->current;
        sub_memory_reader->size = size;
        return;
 error:
        sub_memory_reader->reader.status = -1;
}


void
memory_reader_initialize (struct memory_reader *reader, 
                          uint8_t *buffer, uint32_t size)
{
        reader->reader.set_msb = (void (*) (struct reader *))
                memory_reader_set_msb;
        reader->reader.set_lsb = (void (*) (struct reader *))
                memory_reader_set_lsb;
        reader->reader.read_u8 = (uint8_t (*) (struct reader *))
                memory_reader_read_u8;
        reader->reader.read_u16 = (uint16_t (*) (struct reader *))
                memory_reader_read_u16;
        reader->reader.read_u32 = (uint32_t (*) (struct reader *))
                memory_reader_read_u32;
        reader->reader.read_u64 = (uint64_t (*) (struct reader *))
                memory_reader_read_u64;
        reader->reader.read_u = (uint32_t (*) (struct reader *, uint8_t))
                memory_reader_read_u;
        reader->reader.read_uleb128 = (uint64_t (*) (struct reader *))
                memory_reader_read_uleb128;
        reader->reader.read_sleb128 = (int64_t (*) (struct reader *))
                memory_reader_read_sleb128;

        reader->reader.read_s8 = (int8_t (*) (struct reader *))
                memory_reader_read_s8;
        reader->reader.read_s16 = (int16_t (*) (struct reader *))
                memory_reader_read_s16;
        reader->reader.read_s32 = (int32_t (*) (struct reader *))
                memory_reader_read_s32;

        reader->reader.read_u8bcd = (uint8_t (*) (struct reader *))
                memory_reader_read_u8bcd;
        reader->reader.read_u16bcd = (uint16_t (*) (struct reader *))
                memory_reader_read_u16bcd;

        reader->reader.get_offset = (uint32_t (*) (struct reader *))
                memory_reader_get_offset;
        reader->reader.seek = (void (*) (struct reader *, uint32_t))
                memory_reader_seek;
        reader->reader.skip = (void (*) (struct reader *, uint32_t))
                memory_reader_skip;
        reader->reader.skip64 = (void (*) (struct reader *, uint64_t))
                memory_reader_skip64;
        reader->reader.status = 0;
        reader->buffer = buffer;
        reader->size = size;
        reader->current = 0;
}
