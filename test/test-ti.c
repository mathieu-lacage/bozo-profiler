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
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "memory-reader.h"

#define VERIFY(x,expected) \
{ \
        uint8_t got = x->read_u8 (x); \
        if (got != expected) { \
                printf ("expected: %u, got: %u\n", expected, got); \
        } \
}


static void
dump_data (char const *name, 
           struct reader *reader, 
           uint32_t start, 
           uint32_t length)
{
        uint32_t i;
        FILE *file;
        file = fopen (name, "w");
        reader->seek (reader, start);
        for (i = 0; i < length; i++) {
                uint8_t c;
                c = reader->read_u8 (reader);
                fwrite (&c, 1, 1, file);
        }
        fclose (file);
}


static uint32_t
read_flash_header (struct reader *reader)
{
        int i;
        uint16_t version, revision_year;
        uint8_t flags, object_type, revision_day, revision_month;
        uint8_t app_name_length, device_type, data_type;
        uint32_t data_length;

        printf ("flash header\n");

        reader->set_msb (reader);

        VERIFY (reader, '*');
        VERIFY (reader, '*');
        VERIFY (reader, 'T');
        VERIFY (reader, 'I');
        VERIFY (reader, 'F');
        VERIFY (reader, 'L');
        VERIFY (reader, '*');
        VERIFY (reader, '*');

        version = reader->read_u16bcd (reader);
        flags = reader->read_u8 (reader);
        object_type = reader->read_u8 (reader);
        revision_day = reader->read_u8bcd (reader);
        revision_month = reader->read_u8bcd (reader);
        revision_year = reader->read_u16bcd (reader);

        printf ("version: %u, flags: 0x%x, object_type: 0x%x, DD:MM:YY: %02u:%02u:%02u\n", 
                version, flags, object_type, revision_day, revision_month, revision_year);
        app_name_length = reader->read_u8 (reader);
        printf ("application name: (%u) \"", app_name_length);
        for (i = 0; i < 8; i++) {
                uint8_t c = reader->read_u8 (reader);
                if (c > 0) {
                        printf ("%c", c);
                }
        }
        printf ("\"\n");
        for (i = 0; i < 23; i++) {
                VERIFY (reader, 0);
        }
        device_type = reader->read_u8 (reader);
        if (device_type == (uint8_t)0x98) {
                printf ("TI-89\n");
        } else if (device_type == (uint8_t)0x88) {
                printf ("TI-92\n");
        } else {
                printf ("unknown device: 0x%x\n", device_type);
        }
        data_type = reader->read_u8 (reader);
        printf ("data type: 0x%x\n", data_type);
        for (i = 0; i < 24; i++) {
                VERIFY (reader, 0);
        }
        reader->set_lsb (reader);
        data_length = reader->read_u32 (reader);
        reader->set_msb (reader);
        printf ("data length: 0x%x\n", data_length);
        return data_length;
}


static uint32_t
read_tag (struct reader *reader, uint8_t tag)
{
        uint8_t c, length;
        uint64_t retval;
        c = reader->read_u8 (reader);
        if (c != 0x81) {
                printf ("xx%x\n", c);
                goto error;
        }
        c = reader->read_u8 (reader);
        if ((c >> 4) != tag) {
                printf ("x%x\n", c);
                goto error;
        }
        length = c & 0x0f;
        if (length == 0xf) {
                length = 4;
        } else if (length == 0xe) {
                length = 2;
        } else if (length == 0xd) {
                length = 2;
        }
        if (length > 4) {
                goto error;
        }

        retval = reader->read_u (reader, length);
        return (uint32_t) retval;
 error:
        return 0xdeadbeaf;
}

static uint32_t
read_license_header (struct reader *reader)
{
        uint8_t c;
        uint32_t tag_value;
        uint32_t data_size;

        printf ("licence header\n");

        reader->set_msb (reader);

        // no idea what this tag represents
        read_tag (reader, 0);
        tag_value = read_tag (reader, 1);
        if (tag_value == 0x0101) {
                printf ("ti-89\n");
        } else if (tag_value == 0x0103) {
                printf ("ti-92\n");
        }
        tag_value = read_tag (reader, 2);
        printf ("revision %u\n", tag_value);
        tag_value = read_tag (reader, 3);
        printf ("build %u\n", tag_value);
        VERIFY (reader, 0x81);
        c = reader->read_u8 (reader);
        if ((c>>4) == 4) {
                // app name
                uint8_t n = c & 0x0f;
                uint8_t i;
                for (i = 0; i < n; i++) {
                        c = reader->read_u8 (reader);
                        if (c != 0) {
                                printf ("%c", c);
                        }
                }
                printf ("\n");
        }
        VERIFY (reader, 0x03);
        VERIFY (reader, 0x26);
        VERIFY (reader, 0x09);
        VERIFY (reader, 0x04);
        // no idea what these bytes represent
        reader->skip (reader, 4);
        VERIFY (reader, 0x02);
        VERIFY (reader, 0x0d);
        VERIFY (reader, 0x40);
        // no idea what these bytes represent
        dump_data ("dump.id", reader, reader->get_offset (reader), 0x40);
        c = reader->read_u8 (reader);
        if (c == 0x00) {
                VERIFY (reader, 0x01);
                c = reader->read_u8 (reader);
                if (c == 0x00 || c == 0xff) {
                        // ok
                } else {
                        printf ("invalid data 0\n");
                }
                data_size = read_tag (reader, 7);
                printf ("data size: 0x%x\n", data_size);
        } else if (c == 0x81) {
                c = reader->read_u8 (reader);
                if (c == 0x7f) {
                        data_size = reader->read_u32 (reader);
                        printf ("data size: 0x%x\n", data_size);
                } else if (c == 0x7e) {
                        data_size = reader->read_u16 (reader);
                        printf ("data size: 0x%x\n", data_size);
                } else {
                        /* quiet compiler */
                        data_size = 0;
                        printf ("invalid data 1\n");
                }
        } else {
                /* quiet compiler */
                data_size = 0;
                printf ("invalid data 2\n");
        }
        reader->set_lsb (reader);
        return data_size;
}

static void
read_application_header (struct reader *reader)
{
        uint8_t j;
        uint16_t flags;
        uint32_t data_segment_length, code_segment_offset;
        uint32_t data_table_offset, data_table_length;
        uint32_t optional_header_length;

        printf ("application header\n");
        
        VERIFY (reader, 0x16);
        VERIFY (reader, 0x7b);
        VERIFY (reader, 0x53);
        VERIFY (reader, 0x3d);
        for (j = 0; j < 8; j++) {
                uint8_t c = reader->read_u8 (reader);
                if (c != 0) {
                        printf ("%c", c);
                }
        }
        printf ("\n");
        reader->skip (reader, 24);
        flags = reader->read_u16 (reader);
        reader->set_msb (reader);
        data_segment_length = reader->read_u32 (reader);
        code_segment_offset = reader->read_u32 (reader);
        data_table_offset = reader->read_u32 (reader);
        data_table_length = reader->read_u32 (reader);
        optional_header_length = reader->read_u32 (reader);
        printf ("flags: 0x%x\n", flags);
        printf ("data segment length: 0x%x, code segment offset: 0x%x\n",
                data_segment_length, code_segment_offset);
        printf ("data table offset: 0x%x, length: 0x%x\n",
                data_table_offset, data_table_length);
        printf ("optional header length: 0x%x\n", optional_header_length);
        reader->skip (reader, optional_header_length);
}




static void
read_ti (struct reader *reader, uint32_t file_size)
{
        uint32_t data_length, application_length;
        uint32_t saved_offset;

        data_length = read_flash_header (reader);
        application_length = read_license_header (reader);
        saved_offset = reader->get_offset (reader);
        read_application_header (reader);
        reader->seek (reader, saved_offset);
        reader->skip (reader, application_length);
        VERIFY (reader, 0x02);
        VERIFY (reader, 0x0d);
        VERIFY (reader, 0x40);
        saved_offset = reader->get_offset (reader);
        printf ("off: 0x%x, 0x%x, 0x%x, 0x%x\n", 
                saved_offset, application_length, 
                data_length, file_size);
        dump_data ("dump.signature", reader, saved_offset, 0x40);
        reader->seek (reader, 0);
        data_length = read_flash_header (reader);
        saved_offset = reader->get_offset (reader);
        dump_data ("dump.data", reader, saved_offset, data_length);
}

int main (int argc, char *argv[])
{
        uint8_t *data;
        int fd;
        struct stat stat_buf;
        uint32_t size;
        struct memory_reader reader;


        fd = open (argv[1], O_RDONLY);
        if (fd == -1) {
                printf ("could not open %s\n", argv[1]);
                exit (1);
        }
        if (fstat (fd, &stat_buf) == -1) {
                printf ("unable to stat %s\n", argv[1]);
        }
        data = mmap (0, stat_buf.st_size, PROT_READ, MAP_SHARED, fd, 0);
        size = stat_buf.st_size;

        memory_reader_initialize (&reader, data, size);

	read_ti (READER (&reader), size);

        return 0;
}
