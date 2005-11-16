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

#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <assert.h>

#include "memory-reader.h"
#include "dwarf2-line.h"
#include "dwarf2-info.h"
#include "dwarf2-abbrev.h"
#include "elf32-parser.h"

#if 0

struct functions {
        char const *name;
        uint32_t low;
        uint32_t high;
        struct functions *next;
};

static struct functions *g_functions = NULL;

static bool g_found_stmt_list;
static uint32_t g_stmt_list;
static uint32_t g_debug_str_start = 0;


static int 
info_callback (struct dwarf2_info_entry *entry, void *data)
{
       
        assert (data == NULL);
        switch (entry->tag) {
        case DW_TAG_COMPILE_UNIT:
                if (entry->used & DW2_INFO_ATTR_STMT_LIST) {
                        g_found_stmt_list = true;
                        g_stmt_list = entry->stmt_list;
                }
                break;
        case DW_TAG_SUBPROGRAM: {
                if ((entry->used & DW2_INFO_ATTR_HIGH_PC) &&
                    (entry->used & DW2_INFO_ATTR_LOW_PC)) {
                        struct funtions *other = malloc (sizeof (*other));
                        assert (other != NULL);
                        assert ();
                        other->low = entry->low_pc;
                        other->high = entry->high_pc;
                        other->next = g_functions;

                        if (entry->used & DW2_INFO_ATTR_NAME_OFFSET) {
                                other->name = strdup (g_debug_str_start + entry->.name_offset);
                        } else {
                                if (entry->used & DW2_INFO_ATTR_ABSTRACT_ORIGIN) {
                                        if (read_abbrev_entry (&data,
                                                               abbrev_attributes.abstract_origin,
                                                               &abbrev_attributes,
                                                               abbrev_reader, reader) == -1) {
                                                goto error;
                                        }
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

                }
                
        } break;
        }
}
#endif

static int
dump_from_readers (struct reader *reader, struct reader *abbrev_reader)
{
        struct elf32_header elf32;
        struct elf32_section_header debug_line_section;
        struct elf32_section_header debug_str_section;
        struct elf32_section_header debug_info_section;
        struct elf32_section_header debug_abbrev_section;
        struct dwarf2_info info;
        struct dwarf2_info_cuh cuh;
        struct dwarf2_info_entry entry;
        uint32_t info_offset;

        if (elf32_parser_initialize (&elf32, reader) == -1) {
                goto error;
        }

        if (elf32_parser_read_section_by_name (&elf32, ".debug_line",
                                               &debug_line_section,
                                               reader) == -1) {
                goto error;
        }
        if (elf32_parser_read_section_by_name (&elf32, ".debug_str",
                                               &debug_str_section,
                                               reader) == -1) {
                goto error;
        }
        if (elf32_parser_read_section_by_name (&elf32, ".debug_info",
                                               &debug_info_section,
                                               reader) == -1) {
                goto error;
        }
        if (elf32_parser_read_section_by_name (&elf32, ".debug_abbrev",
                                               &debug_abbrev_section,
                                               reader) == -1) {
                goto error;
        }

        uint32_t info_start = debug_info_section.sh_offset;
        uint32_t info_end = info_start + debug_info_section.sh_size;
        uint32_t abbrev_start = debug_abbrev_section.sh_offset;
        uint32_t abbrev_end = abbrev_start + debug_abbrev_section.sh_size;
        uint32_t str_start = debug_str_section.sh_offset;

        dwarf2_info_initialize (&info, info_start, info_end, str_start, abbrev_start, abbrev_end);

        reader->seek (reader, info_start);
        while (reader->get_offset (reader) < info_end) {
                dwarf2_info_read_cuh (&info, &cuh, 
                                      reader->get_offset (reader),
                                      reader);
                dwarf2_info_cuh_read_entry_first (&cuh, &entry,
                                                  &info_offset,
                                                  abbrev_reader,
                                                  reader);
                while (dwarf2_info_entry_is_last (info_offset, reader)) {
                        switch (entry.tag) {
                                
                        }
                        dwarf2_info_cuh_read_entry (&cuh, 
                                                    info_offset,
                                                    &entry,
                                                    &info_offset,
                                                    abbrev_reader,
                                                    reader);
                }
        }
        
        


        return 0;
 error:
        return -1;
}

static int
dump_from_file (char const *filename)
{
	int fd;
        struct stat stat_buf;
        uint32_t size;
        uint8_t *data;
        struct memory_reader reader;
        struct memory_reader abbrev_reader;

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

        return dump_from_readers (READER (&reader), READER (&abbrev_reader));
 error:
        return -1;
}

#include <string.h>


int main (int argc, char *argv[])
{
        char *file = NULL;
        argc--;
        argv++;
        while (argc > 0) {
                if (strncmp (argv[0], "--file=", strlen ("--file=")) == 0) {
                        file = argv[0];
                        file += strlen ("--file=");
                } else if (strcmp (argv[0], "--print-bb") == 0) {
                } else if (strcmp (argv[0], "") == 0) {
                }
        }
        if (file != 0) {
                return dump_from_file (file);
        }

        return 0;
}
