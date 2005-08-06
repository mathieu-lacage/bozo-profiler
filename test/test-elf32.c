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

#include "elf32-parser.h"
#include "memory-reader.h"


int
test_elf32 (int argc, char *argv[])
{
        uint8_t *data;
        int fd, status;
        struct stat stat_buf;
        uint32_t size;
        struct memory_reader reader;
        struct elf32_header elf32_header;
        char *symbol_name = argv[2];
        struct elf32_symbol symbol;
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
        status = elf32_parser_initialize (&elf32_header, READER (&reader));
        if (status == -1) {
                printf ("unable to initialize parser\n");
                exit (1);
        }

        status = elf32_parser_find_symbol_slow (&elf32_header, symbol_name, &symbol, READER (&reader));
        if (status == -1) {
                printf ("unable to lookup symbol \"%s\"\n", symbol_name);
                exit (2);
        }
        printf ("found symbol \"%s\": 0x%x\n", symbol_name, symbol.st_value);

        return 0;
}
