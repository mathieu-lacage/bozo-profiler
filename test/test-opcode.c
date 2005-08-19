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

  Copyright (C) 2005  Mathieu Lacage
  Author: Mathieu Lacage <mathieu@gnu.org>
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>


#include "elf32-parser.h"
#include "memory-reader.h"
#include "x86-opcode.h"

int 
test_opcode (int argc, char *argv[])
{
        struct memory_reader reader;
        struct elf32_header elf32;
        struct elf32_section_header header;
        struct stat stat_buf;
        int fd;
        uint8_t *text_buffer;
        uint32_t text_size;
        uint8_t *data;
        uint32_t size, total_read;
        char const *filename = argv[0];
        struct x86_opcode_parser parser;

        if (x86_opcode_run_self_tests ()) {
                goto error;
        }

        fd = open (filename, O_RDONLY);
        if (fd == -1) {
                printf ("error opening \"%s\"\n", filename);
                goto error;
        }
        if (fstat (fd, &stat_buf) == -1) {
                printf ("unable to stat \"%s\"\n", filename);
                close (fd);
                goto error;
        }
        size = stat_buf.st_size;
        data = mmap (0, stat_buf.st_size, PROT_READ, MAP_SHARED, fd, 0);
        memory_reader_initialize (&reader, data, size);

        if (elf32_parser_initialize (&elf32, READER (&reader)) == -1) {
                printf ("unable to parse elf32 header for \"%s\"\n", filename);
                close (fd);
                goto error;
        }

        if (elf32_parser_read_section_by_name (&elf32, ".text", 
                                               &header, READER (&reader)) == -1) {
                printf ("unable to parse elf32 section header .text for \"%s\"\n", filename);
                close (fd);
                goto error;
        }

        text_buffer = data + header.sh_offset;
        text_size = header.sh_size;

        x86_opcode_initialize (&parser, X86_MODE_32);

        total_read = 0;
        while (total_read < text_size && !x86_opcode_error (&parser)) {
                uint32_t tmp_read = x86_opcode_parse (&parser, text_buffer, text_size);
                total_read += tmp_read;
                text_buffer += tmp_read;
                x86_opcode_print (&parser);
        }

        return 0;
 error:
        return -1;
}
