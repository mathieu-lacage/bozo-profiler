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

#include "memory-reader.h"
#include "reader.h"
#include "dwarf2-parser.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>

static void
print_string (struct reader *reader, uint32_t offset)
{
        uint8_t b;
        reader->seek (reader, offset);
        do {
                b = reader->read_u8 (reader);
                if (b != 0) {
                        printf ("%c", b);
                }
        } while (b != 0);
}


static void 
print_symbol (struct dwarf2_symbol_information *symbol, struct reader *reader)
{
        if (symbol->valid_fields & DWARF2_SYMBOL_NAME_OFFSET) {
                printf ("name: ");
                print_string (reader, symbol->name_offset);
                printf (" ");
        }
        if (symbol->valid_fields & DWARF2_SYMBOL_COMP_DIRNAME_OFFSET) {
                printf ("(");
                print_string (reader, symbol->comp_dirname_offset);
                printf (") ");
        }
        if (symbol->valid_fields & DWARF2_SYMBOL_DIRNAME_OFFSET) {
                printf ("(");
                print_string (reader, symbol->dirname_offset);
                printf (") ");
        }
        if (symbol->valid_fields & DWARF2_SYMBOL_FILENAME_OFFSET) {
                printf ("(");
                print_string (reader, symbol->filename_offset);
                printf (") ");
        }
        if (symbol->valid_fields & DWARF2_SYMBOL_LINE) {
                printf ("L%llu", symbol->line);
        }
        printf ("\n");
}

/* Of course, this code is completely gcc/32-bit x86 specific */
static uint64_t
get_caller_pc (void)
{
        uint32_t v;
        uint32_t *p;
        p = &v;
        return p[2];
}

int foo (void) {return 9;}

int main (int argc, char *argv[])
{
        uint32_t target_address;
        struct memory_reader reader;
        struct memory_reader abbrev_reader;
        struct stat stat_buf;
        int fd;
        uint8_t *data;
        uint32_t size;
        //char const *filename = "/proc/self/exe";
        char const *filename = "bin/test/test-dwarf2";
        struct dwarf2_symbol_information symbol;

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
        memory_reader_initialize (&abbrev_reader, data, size);

        target_address = (uint32_t) foo;
        if (dwarf2_lookup (target_address, &symbol, READER (&reader), READER (&abbrev_reader)) == -1) {
                printf ("error foo\n");
                goto error;
        }
        print_symbol (&symbol, READER (&reader));

        memory_reader_initialize (&reader, data, size);
        memory_reader_initialize (&abbrev_reader, data, size);

        target_address = (uint32_t) dwarf2_lookup;
        if (dwarf2_lookup (target_address, &symbol, READER (&reader), READER (&abbrev_reader)) == -1) {
                printf ("error dwarf2\n");
                goto error;
        }
        print_symbol (&symbol, READER (&reader));

        target_address = get_caller_pc ();
        printf ("target %x\n", target_address);
        if (dwarf2_lookup (target_address, &symbol, READER (&reader), READER (&abbrev_reader)) == -1) {
                printf ("error dwarf2\n");
                goto error;
        }
        print_symbol (&symbol, READER (&reader));
#if 0
        memory_reader_initialize (&reader, data, size);
        memory_reader_initialize (&abbrev_reader, data, size);

        target_address = (uint32_t) errno;
        if (dwarf2_lookup (target_address, &symbol, READER (&reader), READER (&abbrev_reader)) == -1) {
                printf ("error dwarf2\n");
                goto error;
        }
        print_symbol (&symbol, READER (&reader));
#endif
        return 0;
 error:
        return -1;
}
