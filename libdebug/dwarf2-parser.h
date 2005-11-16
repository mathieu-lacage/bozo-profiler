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

#ifndef DWARF2_PARSER_H
#define DWARF2_PARSER_H

#include "reader.h"
#include "dwarf2-line.h"
#include <stdint.h>

enum dwarf2_symbol_information_e {
        DWARF2_SYMBOL_NAME_OFFSET      = (1<<0),
        DWARF2_SYMBOL_LINE             = (1<<1),
        DWARF2_SYMBOL_FILENAME_OFFSET  = (1<<2),
        DWARF2_SYMBOL_DIRNAME_OFFSET   = (1<<3),
        DWARF2_SYMBOL_COMP_DIRNAME_OFFSET   = (1<<4),
        DWARF2_SYMBOL_HIGH_PC          = (1<<5),
        DWARF2_SYMBOL_LOW_PC           = (1<<6)
};

struct dwarf2_symbol_information {
        /* ORed flags which indicate which fields in this
         * structure are valid and can be used.
         */
        int valid_fields;
        /* offset in reader where the symbol name can be read */
        uint32_t name_offset; 
        /* line number where the symbol is defined. */
        uint64_t line;
        /* offset in reader where the filename (where the symbol
         * is defined) can be read.
         */
        uint32_t filename_offset;
        /* offset in reader where the directory name
         * (which contains the filname where the symbol is 
         * defined) can be read.
         * This name is either an absolute path or a path
         * relative to the compilation directory (defined
         * below)
         * If this field is not valid and the filename_offset 
         * field is valid, the directory in which the 
         * filename is located is the compilation directory.
         * shown below.
         */
        uint32_t dirname_offset;
        /* offset in the reader where the directory name
         * of the compilation directory can be read.
         */
        uint32_t comp_dirname_offset;
        uint64_t high_pc;
        uint64_t low_pc;
};

/* This function performs a thoroughful lookup within the 
 * dwarf2 sections contained in the elf32 file accessed
 * through both the reader and the abbrev_reader readers.
 * Both of the readers must access the elf32 file
 * from offset zero in independent ways.
 * If symbol information is found, it is returned in the
 * symbol structure. If an error occurs during parsing of
 * these structures, -1 is returned. 0 is returned otherwise.
 */
int dwarf2_lookup (uint64_t address, struct dwarf2_symbol_information *symbol, 
                   struct reader *reader, struct reader *abbrev_reader);

int dwarf2_parser_get_all_rows (void (*)(struct dwarf2_line_machine_state *, void *),
                                void *data,
                                struct reader *reader);

#endif /* DWARF2_PARSER_H */
