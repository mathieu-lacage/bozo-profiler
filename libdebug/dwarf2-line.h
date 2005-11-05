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

#ifndef DWARF2_LINE_H
#define DWARF2_LINE_H

#include <stdint.h>
#include "elf32-parser.h"
#include "dwarf2-parser.h"
#include "reader.h"

/* section 6.2.2 dwarf 2.0.0 p51 */
struct dwarf2_line_machine_state {
        uint64_t address;
        uint64_t file;
        uint64_t line;
        uint64_t column;
        mbool is_stmt;
        mbool basic_block;
        mbool end_sequence;
};

/* see section 6.4.2 dwarf 2.0.0 p52 */
struct dwarf2_line_cuh {
        uint32_t offset; /* from the begining of the file to here.*/
        uint32_t length; /* not including this field */
        uint16_t version; /* 2 */
        uint32_t prologue_length; 
        uint8_t  minimum_instruction_length;
        uint8_t  default_is_stmt;
        int8_t   line_base;
        uint8_t  line_range;
        uint8_t  opcode_base;
        /* the length of the array "standard_opcode_lengths" 
         * is equal to the value of opcode_base.
         */
        /* length of the array "include_directories" */
        uint32_t include_directories_length;
        /* length of the array "file_names" */
        uint32_t file_names_length;
        /* number of entries in file_names */
        uint64_t nfiles;
        /* number of entries in include_directories */
        uint64_t ndirs;
};

struct dwarf2_line_file_information {
        /* 0: refers to compilation directory.
         * 1 -- cuh->ndirs:  refers to directory you 
         *   can read with read_directory_name.
         */
        uint64_t directory_index;
        /* bytes */
        uint64_t size;
        uint64_t last_modif_time;
        uint32_t name_offset;
};


int dwarf2_line_read_cuh (uint64_t stmt_list, 
                          struct dwarf2_line_cuh *cuh,
                          struct elf32_header const *elf32, 
                          struct reader *reader);

int dwarf2_line_state_for_address (struct dwarf2_line_cuh const *cuh,
                                   struct dwarf2_line_machine_state *state,
                                   uint32_t target_address, 
                                   struct elf32_header const *elf32, 
                                   struct reader *reader);

int dwarf2_line_get_bb_start (struct dwarf2_line_cuh const *cuh,
                              struct dwarf2_line_machine_state *state,
                              uint32_t ad_start, 
                              uint32_t ad_end,
                              void (*report_bb_start) (uint32_t, void *),
                              void *user_data,
                              struct elf32_header const *elf32, 
                              struct reader *reader);


/* 1 <= nfile <= cuh->nfile */
int dwarf2_line_read_file_information (struct dwarf2_line_cuh const *cuh, 
                                       uint64_t file_index,
                                       struct dwarf2_line_file_information *file,
                                       struct reader *reader);

/* 1 <= ndirectory <= cuh->ndirs */
int dwarf2_line_read_directory_name (struct dwarf2_line_cuh const *cuh, 
                                     uint64_t directory_index,
                                     uint32_t *dirname_offset, 
                                     struct reader *reader);

#endif /* DWARF2_LINE_H */
