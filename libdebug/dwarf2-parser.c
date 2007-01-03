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
#include "dwarf2-info.h"
#include "dwarf2-aranges.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#define nopeABBREV_DEBUG 1
#ifdef ABBREV_DEBUG
#define ABBREV_DEBUG_PRINTF(str, ...) \
     printf (str, ## __VA_ARGS__);
#else
#define ABBREV_DEBUG_PRINTF(str, ...)
#endif


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

struct aranges_lookup_data {
        uint32_t target_address;
        uint32_t cuh_offset;
        bool found;
};

static int 
aranges_cb (uint32_t cuh_offset, uint32_t start, uint32_t size, void *context)
{
        struct aranges_lookup_data *data = (struct aranges_lookup_data *)context;
        if (data->target_address > start &&
            data->target_address <= start + size) {
                data->cuh_offset = cuh_offset;
                data->found = true;
                return 1;
        }
        return 0;
}
        
int 
dwarf2_lookup (uint64_t target_address, 
               struct dwarf2_symbol_information *symbol,
               struct reader *abbrev_reader,
               struct reader *reader)
{
        struct elf32_section_header section_header;
        struct elf32_header elf32;
        uint32_t aranges_start, aranges_end;
        struct aranges_lookup_data aranges_cb_context;
        uint32_t cuh_start;
        uint32_t info_start, info_end;
        uint32_t str_start;
        uint32_t abbrev_start, abbrev_end;
        struct dwarf2_info info;
        struct dwarf2_info_cuh cuh;
        struct dwarf2_info_entry cuh_entry;
        uint32_t current;
        bool found_stmt_list = false;
        uint32_t stmt_list = 0;
        
        if (elf32_parser_initialize (&elf32, reader) == -1) {
                printf ("elf32 init error\n");
                goto error;
        }

        
        if (elf32_parser_read_section_by_name (&elf32, ".debug_aranges", 
                                               &section_header,
                                               reader) == -1) {
                goto error;
        }
        aranges_start = section_header.sh_offset;
        aranges_end = aranges_start + section_header.sh_size;
        aranges_cb_context.found = false;
        aranges_cb_context.target_address = target_address;
        if (dwarf2_aranges_read_all (aranges_start,
                                     aranges_end,
                                     aranges_cb,
                                     &aranges_cb_context,
                                     reader) == -1) {
                goto error;
        }
        if (!aranges_cb_context.found) {
                goto error;
        }
        cuh_start = aranges_cb_context.cuh_offset;


        if (elf32_parser_read_section_by_name (&elf32, ".debug_info", 
                                               &section_header,
                                               reader) == -1) {
                goto error;
        }
        info_start = section_header.sh_offset;
        info_end = section_header.sh_offset + section_header.sh_size;

        if (elf32_parser_read_section_by_name (&elf32, ".debug_str", 
                                               &section_header,
                                               reader) == -1) {
                goto error;
        }
        str_start = section_header.sh_offset;

        if (elf32_parser_read_section_by_name (&elf32, ".debug_abbrev", 
                                               &section_header,
                                               reader) == -1) {
                goto error;
        }
        abbrev_start = section_header.sh_offset;
        abbrev_end = section_header.sh_offset + section_header.sh_size;
        
        dwarf2_info_initialize (&info, info_start, info_end,
                                str_start, abbrev_start, abbrev_end);
        dwarf2_info_read_cuh (&info, &cuh, cuh_start, reader);
        dwarf2_info_cuh_read_entry_first (&cuh, &cuh_entry, &current, abbrev_reader, reader);
        do {
                dwarf2_info_cuh_read_entry (&cuh, &cuh_entry, current, &current, 
                                            abbrev_reader, reader);
                if (cuh_entry.tag == DW_TAG_COMPILE_UNIT) {
                        if (cuh_entry.used & DW2_INFO_ATTR_STMT_LIST) {
                                found_stmt_list = true;
                                stmt_list = cuh_entry.stmt_list;
                        } 
                        if (cuh_entry.used & DW2_INFO_ATTR_COMP_DIRNAME_OFFSET) {
                                symbol->valid_fields |= DWARF2_SYMBOL_COMP_DIRNAME_OFFSET;
                                symbol->comp_dirname_offset = cuh_entry.comp_dirname_offset;
                        }
                } else if (cuh_entry.used & DW2_INFO_ATTR_HIGH_PC &&
                           cuh_entry.used & DW2_INFO_ATTR_LOW_PC &&
                           target_address >= cuh_entry.low_pc &&
                           target_address < cuh_entry.high_pc) {
                        symbol->valid_fields |= DWARF2_SYMBOL_HIGH_PC;
                        symbol->valid_fields |= DWARF2_SYMBOL_LOW_PC;
                        symbol->high_pc = cuh_entry.high_pc;
                        symbol->low_pc = cuh_entry.low_pc;
                        goto ok;
                }
        } while (!dwarf2_info_cuh_entry_is_last (current, reader));

        return -1;
 ok:
        if (cuh_entry.used & DW2_INFO_ATTR_NAME_OFFSET) {
                symbol->valid_fields |= DWARF2_SYMBOL_NAME_OFFSET;
                symbol->name_offset = cuh_entry.name_offset;
        } else {
                if (cuh_entry.used & DW2_INFO_ATTR_ABSTRACT_ORIGIN) {
                        dwarf2_info_cuh_read_entry (&cuh, &cuh_entry, 
                                                    cuh_entry.abstract_origin,
                                                    &current, abbrev_reader, reader);
                } 
                if (cuh_entry.used & DW2_INFO_ATTR_SPECIFICATION) {
                        dwarf2_info_cuh_read_entry (&cuh, &cuh_entry, 
                                                    cuh_entry.specification,
                                                    &current, abbrev_reader, reader);
                }
                if (cuh_entry.used & DW2_INFO_ATTR_NAME_OFFSET) {
                        symbol->valid_fields |= DWARF2_SYMBOL_NAME_OFFSET;
                        symbol->name_offset = cuh_entry.name_offset;
                }
        }

        if (found_stmt_list) {
                struct dwarf2_line_cuh line_cuh;
                struct dwarf2_line_machine_state state;
                if (elf32_parser_read_section_by_name (&elf32, ".debug_line", 
                                                       &section_header,
                                                       reader) == -1) {
                        goto end;
                }
                uint64_t stmt_list_start = section_header.sh_offset + stmt_list;

                dwarf2_line_read_cuh (stmt_list_start, &line_cuh, reader);
                if (dwarf2_line_state_for_address (&line_cuh, &state, 
                                                   target_address,
                                                   reader) == -1) {
                        goto end;
                }
                symbol->valid_fields |= DWARF2_SYMBOL_LINE;
                symbol->line = state.line;
                if (update_file_and_directory_information (&line_cuh, 
                                                           state.file, 
                                                           symbol, 
                                                           reader) == -1) {
                        goto end;
                }
        }
        
 end:
        return 0;
 error:
        return -1;
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


