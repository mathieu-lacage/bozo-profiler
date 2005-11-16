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

#include "dwarf2-parser.h"
#include "elf32-parser.h"
#include "dwarf2-constants.h"
#include "dwarf2-line.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

#define nopeABBREV_DEBUG 1
#ifdef ABBREV_DEBUG
#define ABBREV_DEBUG_PRINTF(str, ...) \
     printf (str, ## __VA_ARGS__);
#else
#define ABBREV_DEBUG_PRINTF(str, ...)
#endif


        
int 
dwarf2_lookup (uint64_t target_address, 
               struct dwarf2_symbol_information *symbol,
               struct reader *abbrev_reader,
               struct reader *reader)
{
        return 0;
}

struct all_states_tmp {
        void (*report_state) (struct dwarf2_line_machine_state *, void *);
        void *user_data;
};
static int
all_states_callback (struct dwarf2_line_machine_state *state,
                     void *user_data)
{
        struct all_states_tmp *tmp = (struct all_states_tmp *)user_data;
        tmp->report_state (state, tmp->user_data);
        return 0;
}


int 
dwarf2_parser_get_all_rows (void (*callback)(struct dwarf2_line_machine_state *, void *),
                            void *data,
                            struct reader *reader)
{
        struct elf32_section_header debug_line_section;
        uint32_t end;
        struct elf32_header elf32;

        if (elf32_parser_initialize (&elf32, reader) == -1) {
                goto error;
        }
        if (elf32_parser_read_section_by_name (&elf32, ".debug_line",
                                               &debug_line_section,
                                               reader) == -1) {
                goto error;
        }
        
        end = debug_line_section.sh_offset + debug_line_section.sh_size;
        reader->seek (reader, debug_line_section.sh_offset);
        while (reader->get_offset (reader) < end) {
                struct dwarf2_line_machine_state state;
                struct dwarf2_line_cuh cuh;
                struct all_states_tmp tmp = {
                        callback,
                        data
                };
                dwarf2_line_read_cuh (reader->get_offset (reader),
                                      &cuh, reader);
                if (dwarf2_line_read_all_rows (&cuh, &state,
                                               all_states_callback,
                                               &tmp,
                                               reader) == -1) {
                        goto error;
                }
                reader->seek (reader, cuh.offset + cuh.length + 4);
        }
        return 0;       
 error:
        return -1;
}


#if 0

        struct dwarf2_lookup_data data;
        struct abbrev_attributes abbrev_attributes;
        mbool found_stmt_list;
        uint64_t stmt_list;

        symbol->valid_fields = 0;
        stmt_list = 0;
        found_stmt_list = FALSE;

        if (dwarf2_lookup_data_initialize (&data, target_address, abbrev_reader, reader) == -1) {
                goto error;
        }

        reader->seek (reader, cuh_end (&data));
        while (reader->get_offset (reader) < cu_end (&data)) {
                uint64_t abbr_code, tag;
                uint8_t children;
                uint32_t abbrev_offset;
                abbr_code = reader->read_uleb128 (reader);
                if (abbr_code == 0) {
                        /* I _believe_ this might be the end of the CU. */
                        continue;
                }

                /* goto this abbr code. */
                if (abbrev_code_to_abbrev_offset (&data, 
                                                  abbr_code, 
                                                  &abbrev_offset,
                                                  abbrev_reader) == -1) {
                        goto error;
                }
                abbrev_reader->seek (abbrev_reader, abbrev_offset);
                abbrev_reader->read_uleb128 (abbrev_reader);

                tag = abbrev_reader->read_uleb128 (abbrev_reader);
                children = abbrev_reader->read_u8 (abbrev_reader);

                read_attributes (&data,
                                 &abbrev_attributes, 
                                 abbrev_reader, 
                                 reader);
                
                if (tag == DW_TAG_COMPILE_UNIT) {
                        if (abbrev_attributes.used & ABBREV_ATTR_STMT_LIST) {
                                found_stmt_list = TRUE;
                                stmt_list = abbrev_attributes.stmt_list;
                        } 
                        if (abbrev_attributes.used & ABBREV_ATTR_COMP_DIRNAME_OFFSET) {
                                symbol->valid_fields |= DWARF2_SYMBOL_COMP_DIRNAME_OFFSET;
                                symbol->comp_dirname_offset = abbrev_attributes.comp_dirname_offset;
                        }
                } else if (abbrev_attributes.used & (ABBREV_ATTR_HIGH_PC | ABBREV_ATTR_LOW_PC) &&
                           target_address >= abbrev_attributes.low_pc &&
                           target_address < abbrev_attributes.high_pc) {
                        goto ok;
                }
        }
 error:
        return -1;

 ok:
#if 0
        printf ("fields: %x, low: %llx, high: %llx -- target: %llx\n", abbrev_attributes.used,
                abbrev_attributes.low_pc, abbrev_attributes.high_pc, target_address);
#endif
        symbol->valid_fields |= DWARF2_SYMBOL_HIGH_PC;
        symbol->high_pc = abbrev_attributes.high_pc;
        symbol->valid_fields |= DWARF2_SYMBOL_LOW_PC;
        symbol->low_pc = abbrev_attributes.low_pc;

        if (abbrev_attributes.used & ABBREV_ATTR_NAME_OFFSET) {
                symbol->valid_fields |= DWARF2_SYMBOL_NAME_OFFSET;
                symbol->name_offset = abbrev_attributes.name_offset;
        } else {
                if (abbrev_attributes.used & ABBREV_ATTR_ABSTRACT_ORIGIN) {
                        if (read_abbrev_entry (&data,
                                               abbrev_attributes.abstract_origin,
                                               &abbrev_attributes,
                                               abbrev_reader, reader) == -1) {
                                goto error;
                        }
                } 
                if (abbrev_attributes.used & ABBREV_ATTR_SPECIFICATION) {
                        if (read_abbrev_entry (&data,
                                               abbrev_attributes.specification,
                                               &abbrev_attributes,
                                               abbrev_reader, reader) == -1) {
                                goto error;
                        }
                        if (abbrev_attributes.used & ABBREV_ATTR_NAME_OFFSET) {
                                symbol->valid_fields |= DWARF2_SYMBOL_NAME_OFFSET;
                                symbol->name_offset = abbrev_attributes.name_offset;
                        }
                }
        }

        if (found_stmt_list) {
                struct dwarf2_line_cuh line_cuh;
                struct dwarf2_line_machine_state state;
                if (dwarf2_line_read_cuh (stmt_list, &line_cuh, &data.elf32, reader) == -1) {
                        goto error;
                }
                if (dwarf2_line_state_for_address (&line_cuh, &state, 
                                                   target_address,
                                                   &data.elf32, reader) == -1) {
                        goto error;
                }
                symbol->valid_fields |= DWARF2_SYMBOL_LINE;
                symbol->line = state.line;
                if (update_file_and_directory_information (&line_cuh, 
                                                           state.file, 
                                                           symbol, 
                                                           reader) == -1) {
                        goto error;
                }
        }

        if (reader->status == -1 ||
            abbrev_reader->status == -1) {
                goto error;
        }
        
        return 0;



static uint32_t 
cu_start (struct dwarf2_lookup_data const *data)
{
        uint32_t start;
        start = data->debug_info_start + data->cu_offset;
        return start;        
}

static uint32_t 
cu_end (struct dwarf2_lookup_data const *data)
{
        uint32_t end;
        end = cu_start (data) + 4 + data->cuh.length;
        return end;
}

static uint32_t 
cuh_start (struct dwarf2_lookup_data const *data)
{
        uint32_t start;
        start = cu_start (data);
        return start;
}

static uint32_t 
cuh_end (struct dwarf2_lookup_data const *data)
{
        uint32_t start;
        start = cuh_start (data) + 4 + 2 + 4 + 1;
        return start;
}


static int
update_file_and_directory_information (struct dwarf2_line_cuh const *line_cuh, 
                                       uint64_t file_index, 
                                       struct dwarf2_symbol_information *symbol,
                                       struct reader *reader)
{
        struct dwarf2_line_file_information file;
        if (dwarf2_line_read_file_information (line_cuh,
                                               file_index, 
                                               &file, reader) == -1) {
                goto error;
        }
        symbol->valid_fields |= DWARF2_SYMBOL_FILENAME_OFFSET;
        symbol->filename_offset = file.name_offset;
        if (file.directory_index != 0) {
                uint32_t dirname_offset;
                if (dwarf2_line_read_directory_name (line_cuh, 
                                                     file.directory_index, 
                                                     &dirname_offset,
                                                     reader) == -1) {
                        goto error;
                }
                symbol->valid_fields |= DWARF2_SYMBOL_DIRNAME_OFFSET;
                symbol->dirname_offset = dirname_offset;
        }

        return 0;
 error:
        return -1;
}






static int
read_abbrev_entry (struct dwarf2_lookup_data *data,
                   uint32_t start,
                   struct abbrev_attributes *abbrev_attributes,
                   struct reader *abbrev_reader, 
                   struct reader *reader)
{
        uint64_t abbr_code, tag;
        uint32_t abbrev_offset;
        reader->seek (reader, start);
        abbr_code = reader->read_uleb128 (reader);
        if (abbr_code == 0) {
                goto error;
        }
        if (abbrev_code_to_abbrev_offset (data, 
                                          abbr_code, 
                                          &abbrev_offset,
                                          abbrev_reader) == -1) {
                goto error;
        }
        abbrev_reader->seek (abbrev_reader, abbrev_offset);
        abbrev_reader->read_uleb128 (abbrev_reader);
        
        tag = abbrev_reader->read_uleb128 (abbrev_reader);
        abbrev_reader->skip (abbrev_reader, 1);
        
        read_attributes (data,
                         abbrev_attributes, 
                         abbrev_reader, reader);
        return 0;
 error:
        return -1;
}


#endif
