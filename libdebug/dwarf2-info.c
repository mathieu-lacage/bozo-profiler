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

#include "dwarf2-info.h"
#include "dwarf2-abbrev.h"
#include "reader.h"
#include "dwarf2-constants.h"
#include "dwarf2-utils.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#define DEBUG(x) do {}while (0)


/***********************************************************
 * A bunch of functions to parse the attribute values from
 * their forms.
 ***********************************************************/

static uint64_t 
read_constant_form (struct dwarf2_info_cuh *header,
                    uint32_t form, 
                    struct reader *reader)
{
        uint64_t retval;
        switch (form) {
        case DW_FORM_DATA1:
                retval = reader->read_u8 (reader);
                break;
        case DW_FORM_DATA2:
                retval = reader->read_u16 (reader);
                break;
        case DW_FORM_DATA4:
                retval = reader->read_u32 (reader);
                break;
        case DW_FORM_DATA8:
                retval = reader->read_u64 (reader);
                break;
        case DW_FORM_SDATA:
                retval = reader->read_sleb128 (reader);
                break;
        case DW_FORM_UDATA:
                retval = reader->read_uleb128 (reader);
                break;
        default:
                /* quiet the compiler */
                retval = 0;
                DEBUG ("error, invalid form");
                break;
        }
        return retval;
}

static uint64_t 
read_ref_form (struct dwarf2_info_cuh *header,
               uint32_t form, struct reader *reader)
{
        uint64_t retval;
        if (form == DW_FORM_REF_ADDR) {
                retval = header->info_start;
                retval += reader->read_u (reader, header->address_size);
        } else {
                retval = header->start;
                switch (form) {
                case DW_FORM_REF1:
                        retval += reader->read_u8 (reader);
                        break;
                case DW_FORM_REF2:
                        retval += reader->read_u16 (reader);
                        break;
                case DW_FORM_REF4:
                        retval += reader->read_u32 (reader);
                        break;
                case DW_FORM_REF8:
                        retval += reader->read_u64 (reader);
                        break;
                case DW_FORM_REF_UDATA:
                        retval += reader->read_uleb128 (reader);
                        break;
                }
        }
        return retval;
}

static uint64_t 
read_address_form (struct dwarf2_info_cuh *header,
                   uint32_t form, struct reader *reader)
{
        uint64_t retval;
        retval = reader->read_u (reader, header->address_size);
        return retval;
}

static uint32_t
read_string_form (struct dwarf2_info_cuh *header,
                  uint32_t form, struct reader *reader)
{
        uint32_t offset;
        if (form == DW_FORM_STRING) {
                uint8_t c;
                offset = reader->get_offset (reader);
                do {
                        c = reader->read_u8 (reader);
                } while (c != 0);
        } else if (form == DW_FORM_STRP) {
                offset = header->str_start;
                offset += reader->read_u32 (reader);
        } else {
                offset = 0;
                assert (false);
        }
        return offset;
}
        

static void
skip_form (struct dwarf2_info_cuh *header,
           uint32_t form, struct reader *reader)
{
        switch (form) {
        case DW_FORM_ADDR:
                reader->skip (reader, header->address_size);
                break;
        case DW_FORM_BLOCK: {
                uint64_t length;
                length = reader->read_uleb128 (reader);
                reader->skip64 (reader, length);
        } break;
        case DW_FORM_BLOCK1: {
                uint8_t length;
                length = reader->read_u8 (reader);
                reader->skip (reader, length);
        } break;
        case DW_FORM_BLOCK2:{
                uint16_t length;
                length = reader->read_u16 (reader);
                reader->skip (reader, length);
        } break;
        case DW_FORM_BLOCK4:{
                uint32_t length;
                length = reader->read_u32 (reader);
                reader->skip (reader, length);
        } break;
        case DW_FORM_DATA1:
                reader->skip (reader, 1);
                break;
        case DW_FORM_DATA2:
                reader->skip (reader, 2);
                break;
        case DW_FORM_DATA4:
                reader->skip (reader, 4);
                break;
        case DW_FORM_DATA8:
                reader->skip (reader, 8);
                break;
        case DW_FORM_SDATA: {
                int64_t ref;
                ref = reader->read_sleb128 (reader);
        } break;
        case DW_FORM_UDATA: {
                uint64_t ref;
                ref = reader->read_uleb128 (reader);
        } break;
        case DW_FORM_STRP:
                reader->skip (reader, 4);
                break;
        case DW_FORM_STRING: {
                uint8_t c;
                do {
                        c = reader->read_u8 (reader);
                } while (c != 0);
        } break;
        case DW_FORM_FLAG:
                reader->skip (reader, 1);
                break;
        case DW_FORM_REF_ADDR:
                reader->skip (reader, header->address_size);
                break;
        case DW_FORM_REF1:
                reader->skip (reader, 1);
                break;
        case DW_FORM_REF2:
                reader->skip (reader, 2);
                break;
        case DW_FORM_REF4:
                reader->skip (reader, 4);
                break;
        case DW_FORM_REF8:
                reader->skip (reader, 8);
                break;
        case DW_FORM_REF_UDATA: {
                uint64_t ref;
                ref = reader->read_uleb128 (reader);
        } break;
        case DW_FORM_INDIRECT:
                assert (false);
                break;
        }
}

static void
attr_read_entry (struct dwarf2_info_cuh *cuh,
                 struct dwarf2_abbrev_attr *attr,
                 struct dwarf2_info_entry *entry,
                 struct reader *reader)
{
        uint64_t form = attr->form;
        uint64_t name = attr->name;

        if (form == DW_FORM_INDIRECT) {
                form = reader->read_uleb128 (reader);
        }
        switch (name) {
        case DW_AT_STMT_LIST:
                /* see section 3.1 dwarf 2.0.0 p23 */
                entry->used |= DW2_INFO_ATTR_STMT_LIST;
                entry->stmt_list = read_constant_form (cuh, form, reader);
                break;
        case DW_AT_COMP_DIR:
                /* see section 3.1 dwarf 2.0.0 p24 */
                entry->used |= DW2_INFO_ATTR_COMP_DIRNAME_OFFSET;
                entry->comp_dirname_offset = read_string_form (cuh, form, reader);
                break;
        case DW_AT_HIGH_PC:
                entry->used |= DW2_INFO_ATTR_HIGH_PC;
                entry->high_pc = read_address_form (cuh, form, reader);
                break;
        case DW_AT_LOW_PC:
                entry->used |= DW2_INFO_ATTR_LOW_PC;
                entry->low_pc = read_address_form (cuh, form, reader);
                break;
        case DW_AT_NAME:
                entry->used |= DW2_INFO_ATTR_NAME_OFFSET;
                entry->name_offset = read_string_form (cuh, form, reader);
                break;
        case DW_AT_SPECIFICATION:
                entry->used |= DW2_INFO_ATTR_SPECIFICATION;
                entry->specification = read_ref_form (cuh, form, reader);
                break;
        case DW_AT_ABSTRACT_ORIGIN:
                entry->used |= DW2_INFO_ATTR_ABSTRACT_ORIGIN;
                entry->abstract_origin = read_ref_form (cuh, form, reader);
                break;
        default:
                /* uninteresting for now. */
                assert (form != DW_FORM_INDIRECT);
                skip_form (cuh, form, reader);
                break;
        }
}


void 
dwarf2_info_cuh_read_entry_first (struct dwarf2_info_cuh *cuh,
                                  struct dwarf2_info_entry *entry,
                                  uint32_t *end_offset,
                                  struct reader *abbrev_reader,
                                  struct reader *reader)
{
        entry->child_level = 0;
        dwarf2_info_cuh_read_entry (cuh, entry, 
                                    cuh->start + 4 + 2 + 4 + 1, end_offset,
                                    abbrev_reader, reader);
}

void 
dwarf2_info_cuh_read_entry (struct dwarf2_info_cuh *cuh,
                            struct dwarf2_info_entry *entry,
                            uint32_t start_offset,
                            uint32_t *end_offset,
                            struct reader *abbrev_reader,
                            struct reader *reader)
{
        uint64_t abbr_code;
        struct dwarf2_abbrev_decl decl;
        struct dwarf2_abbrev_attr attr;
        uint32_t attr_offset;
        struct dwarf2_abbrev_cu *abbrev_cu;

        entry->used = 0;
        abbrev_cu = &cuh->abbrev_cu;
        reader->seek (reader, start_offset);
        while (true) {
                abbr_code = reader->read_uleb128 (reader);
                if (abbr_code != 0) {
                        break;
                }
                entry->child_level--;
        }
        dwarf2_abbrev_cu_read_decl (abbrev_cu, &decl, abbr_code, abbrev_reader);
        entry->tag = decl.tag;
        entry->children = decl.children;
        if (entry->children) {
                entry->child_level++;
        }
        dwarf2_abbrev_decl_read_attr_first (&decl, &attr, &attr_offset, abbrev_reader);
        while (!dwarf2_abbrev_attr_is_last (&attr)) {
                attr_read_entry (cuh, &attr, entry, reader);
                dwarf2_abbrev_read_attr (attr_offset, &attr, &attr_offset, abbrev_reader);
        }

        *end_offset = reader->get_offset (reader);
}



void 
dwarf2_info_read_cuh (struct dwarf2_info *info,
                      struct dwarf2_info_cuh *cuh,
                      uint32_t start /* offset to start of cuh from start of file */,
                      struct reader *reader)
{
        /* see section 7.5.1, dw2 */
        reader->seek (reader, start);
        cuh->start = start;
        cuh->info_start = info->info_start;
        cuh->str_start = info->str_start;
        cuh->length = reader->read_u32 (reader);
        cuh->version = reader->read_u16 (reader);
        cuh->offset = info->abbrev_start;
        cuh->offset += reader->read_u32 (reader);
        cuh->address_size = reader->read_u8 (reader);

        dwarf2_abbrev_initialize_cu (&info->abbrev, 
                                     &cuh->abbrev_cu,
                                     cuh->offset);
}

void dwarf2_info_initialize (struct dwarf2_info *info,
                             uint32_t info_start,
                             uint32_t info_end,
                             uint32_t str_start,
                             uint32_t abbrev_start,
                             uint32_t abbrev_end)
{
        info->info_start = info_start;
        info->info_end = info_end;
        info->str_start = str_start;
        info->abbrev_start = abbrev_start;
        dwarf2_abbrev_initialize (&info->abbrev, abbrev_start, abbrev_end);
}


bool
dwarf2_info_cuh_entry_is_last (struct dwarf2_info_entry const*entry,
                               uint32_t offset,
                               struct reader *reader)
{
        uint32_t abbr_code;
        reader->seek (reader, offset);
        abbr_code = reader->read_uleb128 (reader);
        if (abbr_code == 0 && entry->child_level == 0) {
                return true;
        }
        return false;
}


void 
dwarf2_info_cuh_print_entry (struct dwarf2_info_entry const*entry, struct reader *reader)
{
        printf ("TAG=%s\n", dwarf2_utils_tag_to_string (entry->tag));
        printf ("children=%u\n", entry->children);
        if (entry->used & DW2_INFO_ATTR_STMT_LIST) {
                printf ("stmt list=%x\n", entry->stmt_list);
        }
        if (entry->used & DW2_INFO_ATTR_NAME_OFFSET) {
                printf ("name=");
                dwarf2_utils_print_string (reader, entry->name_offset);
                printf ("\n");
        }
        if (entry->used & DW2_INFO_ATTR_COMP_DIRNAME_OFFSET) {
                printf ("comp dirname=");
                dwarf2_utils_print_string (reader, entry->comp_dirname_offset);
                printf ("\n");
        }
        if (entry->used & DW2_INFO_ATTR_HIGH_PC) {
                printf ("high pc=%llx\n", entry->high_pc);
        }
        if (entry->used & DW2_INFO_ATTR_LOW_PC) {
                printf ("low pc=%llx\n", entry->low_pc);
        }
        if (entry->used & DW2_INFO_ATTR_ABSTRACT_ORIGIN) {
                printf ("abstract origin=%llx\n", entry->abstract_origin);
        }
        if (entry->used & DW2_INFO_ATTR_SPECIFICATION) {
                printf ("specification=%llx\n", entry->specification);
        }
}
