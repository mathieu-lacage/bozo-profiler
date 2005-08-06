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

/* Dwarf2 encoding is expected to be in a byte order similar to the
 * one used in the container file. For elf files, this means that the
 * byte order specified in the elf header should be used to read
 * dwarf-2 data structures.
 */

/* As far as target pointer size is concerned, the current supports
 * only targets whose pointer size is < 32 bits. If you feel the need
 * to change this, look for the places marked with a TARGET_PTR comment.
 * These mark the places where a 32 bit integer must be replaced by a
 * 64 bit integer to handler larger address spaces.
 * Of course, the elf32 support should also probably be replaced by some 
 * elf64 code...
 */

#include "dwarf2-parser.h"
#include "elf32-parser.h"
#include "mbool.h"
#include "dwarf2-constants.h"
#include "dwarf2-line.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

#define ABBREV_CACHE 1


#define nopeABBREV_DEBUG 1
#ifdef ABBREV_DEBUG
#define ABBREV_DEBUG_PRINTF(str, ...) \
     printf (str, ## __VA_ARGS__);
#else
#define ABBREV_DEBUG_PRINTF(str, ...)
#endif

#define nopeCACHE_DEBUG 1
#ifdef CACHE_DEBUG
#define CACHE_DEBUG_PRINTF(str, ...) \
     printf (str, ## __VA_ARGS__);
#else
#define CACHE_DEBUG_PRINTF(str, ...)
#endif


/********************************************************************
 *******************************************************************/

#define ADDR_CUH_SIZE (4+2+4+1+1)
#define ADDR_CU_SIZE(cuh) (4+cuh.length)
#define ADDR_TUPLE_SIZE(cuh) (cuh.address_size*2)

#ifdef ABBREV_CACHE

#define CACHE_SIZE (16)
struct cache {
        uint8_t keys[CACHE_SIZE];
        uint32_t values[CACHE_SIZE];
        uint8_t last_used[CACHE_SIZE];
        uint8_t time;
};

#endif /* ABBREV_CACHE */


#define CUH_SIZE (4+2+4+1)

/* The compilation unit header described in the 
 * .debug_info section.
 * see section 7.5.1 dwarf 2.0.0
 */
struct compilation_unit_header {
        uint32_t length;       /* of cu not including this field. */
        uint16_t version;      /* 2 */
        uint32_t offset;       /* into .debug_abbrev */
        uint8_t address_size;  /* of target */
};


/* The compilation unit header described in the 
 * .debug_aranges section.
 */
struct addr_compilation_unit_header {
        uint32_t length;
        uint16_t version;
        uint32_t offset;
        uint8_t address_size;
        uint8_t segment_descriptor_size;
};


enum abbrev_attribute_e {
        ABBREV_ATTR_NAME_OFFSET = (1<<0),
        ABBREV_ATTR_HIGH_PC = (1<<1),
        ABBREV_ATTR_LOW_PC = (1<<2),
        ABBREV_ATTR_STMT_LIST = (1<<3),
        ABBREV_ATTR_ABSTRACT_ORIGIN = (1<<4),
        ABBREV_ATTR_SPECIFICATION = (1<<5),
        ABBREV_ATTR_COMP_DIRNAME_OFFSET = (1<<6),
};

struct abbrev_attributes {
        int used;
        uint32_t stmt_list;
        uint32_t name_offset;
        uint32_t comp_dirname_offset;
        uint64_t high_pc;
        uint64_t low_pc;
        uint64_t abstract_origin;
        uint64_t specification;
};


struct dwarf2_lookup_data {
        uint32_t debug_info_start;
        uint32_t debug_str_start;
        uint32_t debug_abbrev_start;
        uint32_t debug_abbrev_end;
        uint32_t cu_offset;
        struct elf32_header elf32;
        struct compilation_unit_header cuh;
#ifdef ABBREV_CACHE
        struct cache cache;
#endif
};


static uint32_t 
abbrev_table_start (struct dwarf2_lookup_data const *data)
{
        uint32_t start;
        start = data->debug_abbrev_start + data->cuh.offset;
        return start;
}

static uint32_t 
abbrev_table_end (struct dwarf2_lookup_data const *data)
{
        uint32_t end;
        end = data->debug_abbrev_end;
        return end;
}
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


int
lookup_cu (uint32_t target_address, uint32_t *const cu_offset,
           struct elf32_header *elf32, struct reader *reader)
{
        struct elf32_section_header debug_aranges_header;
        struct addr_compilation_unit_header cuh;
        uint32_t end, start;

        if (elf32_parser_read_section_by_name (elf32, ".debug_aranges", 
                                               &debug_aranges_header,
                                               reader) == -1) {
                goto error;
        }

        start = debug_aranges_header.sh_offset;
        end = start + debug_aranges_header.sh_size;
        reader->seek (reader, start);
        while (reader->get_offset (reader) < end) {
                uint32_t tuple_offset, end_offset;
                uint32_t cu_current;
                cuh.length = reader->read_u32 (reader);
                cuh.version = reader->read_u16 (reader);
                cuh.offset = reader->read_u32 (reader);
                cuh.address_size = reader->read_u8 (reader);
                cuh.segment_descriptor_size = reader->read_u8 (reader);
                if (cuh.address_size > 4 || cuh.segment_descriptor_size > 4) {
                        /* TARGET_PTR */
                        /* We do not want to handle targets whose address
                         * space is larger than 32 bits (i.e., 4 bytes).
                         */
                        goto error;
                }
                tuple_offset = ADDR_TUPLE_SIZE (cuh) - (ADDR_CUH_SIZE % ADDR_TUPLE_SIZE (cuh));
                reader->skip (reader, tuple_offset);
                cu_current = ADDR_CUH_SIZE + tuple_offset;
                while (cu_current < cuh.length + 4) {
                        uint32_t tu_address; /* TARGET_PTR */
                        uint32_t tu_delta; /* TARGET_PTR */
                        tu_address = reader->read_u (reader, cuh.address_size);
                        tu_delta = reader->read_u (reader, cuh.address_size);
                        if (tu_address <= target_address && 
                            tu_address + tu_delta > target_address) {
                                *cu_offset = cuh.offset;
                                goto ok;
                        }
                        cu_current += ADDR_TUPLE_SIZE (cuh);
                }
                /* adjust to the end of the compilation unit to be able to
                 * read the following compilation unit header.
                 */
                end_offset = ADDR_CU_SIZE (cuh) - cu_current;
                reader->skip (reader, end_offset);
        }

        if (reader->status == -1) {
                goto error;
        }

 ok:
        return 0;
 error:
        return -1;
}

enum search_abbrev_e {
        SEARCH_BY_TAG,
        SEARCH_BY_CODE
};

/* see section 7.5.3 dwarf2 p67 */
static int
search_abbrev_start_to_end (uint64_t searched_id, enum search_abbrev_e search_type,
                            uint32_t start, uint32_t end, uint32_t*retval,
                            struct reader *reader)
{
        reader->seek (reader, start);

        while (reader->get_offset (reader) < end) {
                uint64_t abbr_code, tag, name, form;
                uint8_t children;
                uint32_t offset;
                offset = reader->get_offset (reader);
                abbr_code = reader->read_uleb128 (reader);
                ABBREV_DEBUG_PRINTF ("%llu ", abbr_code);
                if (abbr_code == 0) {
                        /* last entry for the compilation entry we were 
                         * looking into. We did not find what we were
                         * looking for.
                         */
                        break;
                }
                tag = reader->read_uleb128 (reader);
                children = reader->read_u8 (reader);

                if ((search_type == SEARCH_BY_TAG  && searched_id == tag) || 
                    (search_type == SEARCH_BY_CODE && searched_id == abbr_code)) {
                        *retval = offset;
                        goto ok;
                }

                /* skip the attribute specification */
                do {
                        name = reader->read_uleb128 (reader);
                        form = reader->read_uleb128 (reader);
                } while (name != 0 && form != 0);
        }
 error:
        ABBREV_DEBUG_PRINTF ("\n");
        return -1;
 ok:
        if (reader->status == -1) {
                goto error;
        }
        ABBREV_DEBUG_PRINTF ("\n");
        ABBREV_DEBUG_PRINTF ("input: %llu, output: %u\n", searched_id, *retval);
        return 0;
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



#ifdef ABBREV_CACHE

#define get_occupied(x) (x&0x80)
#define get_value(x) ((uint8_t)x&0x7f)

static int
cache_try_lookup (uint64_t key, uint32_t *value, struct cache *cache)
{
        uint8_t start_slot, i, backup_slot, backup_key;
        cache->time++;
        start_slot = (uint8_t) (key - 1) % CACHE_SIZE;
        i = start_slot;
        backup_key = 0;
        backup_slot = 0;
        do {
                if (get_occupied (cache->keys[i])) {
                        if (get_value (cache->keys[i]) == get_value (key)) {
                                /* found slot */
                                CACHE_DEBUG_PRINTF ("found %d at %d\n", get_value (key), i);
                                cache->last_used[i] = cache->time;
                                *value = cache->values[i];
                                return 0;
                        } else if (get_value (cache->keys[i]) < get_value (key) &&
                                   get_value (cache->keys[i]) > backup_key) {
                                backup_key = get_value (cache->keys[i]);
                                backup_slot = i;
                        }
                }
                i++;
                i %= CACHE_SIZE;
        } while (i != start_slot);
        
        /* did not find the key in the cache after looking 
         * in every entry. Try to return the highest
         * key which is smaller than the requested key.
         * It is stored in backup_slot.
         */
        if (backup_key > 0) {
                cache->last_used[backup_slot] = cache->time;
                *value = cache->values[backup_slot];
                CACHE_DEBUG_PRINTF ("found backup %d at %d for %llu\n", backup_key, backup_slot, key);
                return 0;
        } else {
                CACHE_DEBUG_PRINTF ("did not find %llu\n", key);
                return -1;
        }
}

static uint8_t
cache_search_lru_location (struct cache *cache)
{
        uint8_t lru, location, i;
        location = 0;
        lru = cache->time;
        for (i = 0; i < CACHE_SIZE; i++) {
                if (cache->last_used[i] < lru) {
                        lru = cache->last_used[i];
                        location = i;
                }
        }
        return location;
}

static void 
cache_try_add (uint64_t key, uint32_t value, struct cache *cache)
{
        uint8_t start_slot, i, lru;
        cache->time++;
        start_slot = (uint8_t) (key - 1) % CACHE_SIZE;
        i = start_slot;
        do {
                if (!get_occupied (cache->keys[i])) {
                        /* reached empty slot 
                         * add item.
                         */
                        CACHE_DEBUG_PRINTF ("add %d at %d, start: %d\n", get_value (key), i, start_slot);
                        cache->keys[i] = 0x80 | get_value (key);
                        cache->values[i] = value;
                        cache->last_used[i] = cache->time;
                        goto ok;
                }
                i++;
                i %= CACHE_SIZE;
        } while (i != start_slot);

        /* no empty slot. replace the Least Recently Used 
         * with this entry.
         */
        lru = cache_search_lru_location (cache);
        cache->keys[lru] = 0x80 | get_value (key);
        cache->values[lru] = value;
        cache->last_used[lru] = cache->time;
        CACHE_DEBUG_PRINTF ("add full %d at %d\n", get_value (key), i);
 ok:
        return;
}

static void
cache_initialize (struct cache *cache)
{
        memset (cache, 0, sizeof (*cache));
}

static int
abbrev_code_to_abbrev_offset (struct dwarf2_lookup_data *data, 
                              uint64_t abbr_code, 
                              uint32_t *abbrev_offset,
                              struct reader *abbrev_reader)
{
        uint32_t abbrev_start;
        uint32_t abbrev_end;

        abbrev_start = abbrev_table_start (data);
        abbrev_end = abbrev_table_end (data);

        if (cache_try_lookup (abbr_code, abbrev_offset, &data->cache) == -1) {
                *abbrev_offset = abbrev_start;
        }
        abbrev_reader->seek (abbrev_reader, *abbrev_offset);
        if (abbrev_reader->read_uleb128 (abbrev_reader) != abbr_code) {
                if (search_abbrev_start_to_end (abbr_code, SEARCH_BY_CODE, 
                                                *abbrev_offset, abbrev_end, 
                                                abbrev_offset,
                                                abbrev_reader) == 0) {
                        /* found abbr_code from abbrev_offset to abbrev_end */
                        abbrev_reader->seek (abbrev_reader, *abbrev_offset);
                        abbrev_reader->read_uleb128 (abbrev_reader);
                        cache_try_add (abbr_code, *abbrev_offset, &data->cache);
                } else if (*abbrev_offset == abbrev_start) {
                        /* did not find abbr_code from abbrev_start to abbrev_end */
                        goto error;
                } else if (search_abbrev_start_to_end (abbr_code, SEARCH_BY_CODE, 
                                                       abbrev_start, *abbrev_offset, 
                                                       abbrev_offset,
                                                       abbrev_reader) == -1) {
                        /* Did not find abbr_code from 
                         *   - abbrev_offset to abbrev_end
                         *   - abbrev_start to abbrev_offset
                         * where abbrev_start <= abbrev_offset <= abbrev_end
                         */
                        goto error;
                } else {
                        /* found abbr_code from abbrev_start to abbrev_offset */
                        abbrev_reader->seek (abbrev_reader, *abbrev_offset);
                        abbrev_reader->read_uleb128 (abbrev_reader);
                        cache_try_add (abbr_code, *abbrev_offset, &data->cache);
                }
        }
        return 0;
 error:
        //printf ("could not find abbrev code %llu.\n", abbr_code);
        return -1;
}
#else /* ABBREV_CACHE */

static int
abbrev_code_to_abbrev_offset (struct dwarf2_lookup_data const *data, 
                              uint64_t abbr_code, 
                              uint32_t *abbrev_offset,
                              struct reader *abbrev_reader)
{
        uint32_t abbrev_start;
        uint32_t abbrev_end;
        int retval;

        abbrev_start = abbrev_table_start (&data);
        abbrev_end = abbrev_table_end (&data);

        retval = search_abbrev_start_to_end (abbr_code, SEARCH_BY_CODE, 
                                             abbrev_start, abbrev_end, 
                                             abbrev_offset,
                                             abbrev_reader);
        return retval;
}

        
#endif /* ABBREV_CACHE */



/***********************************************************
 * A bunch of functions to parse the attribute values from
 * their forms.
 ***********************************************************/

static uint64_t 
read_constant_form (struct dwarf2_lookup_data const *data,
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
                printf ("error, invalid form\n");
                break;
        }
        return retval;
}

static uint64_t 
read_ref_form (struct dwarf2_lookup_data const *data,
               uint32_t form, struct reader *reader)
{
        uint64_t retval;
        if (form == DW_FORM_REF_ADDR) {
                retval = reader->read_u (reader, data->cuh.address_size);
        } else {
                retval = cuh_start (data);
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
read_address_form (struct dwarf2_lookup_data const *data,
                   uint32_t form, struct reader *reader)
{
        uint64_t retval;
        retval = reader->read_u (reader, data->cuh.address_size);
        return retval;
}

static uint32_t
read_string_form (struct dwarf2_lookup_data const *data,
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
                offset = data->debug_str_start;
                offset += reader->read_u32 (reader);
        } else {
                offset = 0;
                assert (FALSE);
        }
        return offset;
}
        

static void
skip_form (struct dwarf2_lookup_data const *data,
           uint32_t form, struct reader *reader)
{
        switch (form) {
        case DW_FORM_ADDR:
                reader->skip (reader, data->cuh.address_size);
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
                reader->skip (reader, data->cuh.address_size);
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
                printf ("should not happen\n");
                break;
        }
}

/**
 * Read the abbrev attributes to which reader points.
 * abbrev_reader should point to the form description of
 * the abbrev entry we are parsing.
 */
static void
read_attributes (struct dwarf2_lookup_data const *data,
                 struct abbrev_attributes *abbrev_attributes,
                 struct reader *abbrev_reader,
                 struct reader *reader)
{
        uint64_t name, form;

        abbrev_attributes->used = 0;

        name = abbrev_reader->read_uleb128 (abbrev_reader);
        form = abbrev_reader->read_uleb128 (abbrev_reader);
        while (name != 0 && form != 0) {
                //printf ("%llu/%llu\n", name, form);
                if (form == DW_FORM_INDIRECT) {
                        form = reader->read_uleb128 (reader);
                        continue;
                } else {
                        if (name == DW_AT_STMT_LIST) {
                                /* see section 3.1 dwarf 2.0.0 p23 */
                                abbrev_attributes->used |= ABBREV_ATTR_STMT_LIST;
                                abbrev_attributes->stmt_list = read_constant_form (data, form, reader);
                        } else if (name == DW_AT_COMP_DIR) {
                                /* see section 3.1 dwarf 2.0.0 p24 */
                                abbrev_attributes->used |= ABBREV_ATTR_COMP_DIRNAME_OFFSET;
                                abbrev_attributes->comp_dirname_offset = read_string_form (data, form, reader);
                        } else if (name == DW_AT_HIGH_PC) {
                                abbrev_attributes->used |= ABBREV_ATTR_HIGH_PC;
                                abbrev_attributes->high_pc = read_address_form (data, form, reader);
                        } else if (name == DW_AT_LOW_PC) {
                                abbrev_attributes->used |= ABBREV_ATTR_LOW_PC;
                                abbrev_attributes->low_pc = read_address_form (data, form, reader);
                        } else if (name == DW_AT_NAME) {
                                abbrev_attributes->used |= ABBREV_ATTR_NAME_OFFSET;
                                abbrev_attributes->name_offset = read_string_form (data, form, reader);
                        } else if (name == DW_AT_SPECIFICATION) {
                                abbrev_attributes->used |= ABBREV_ATTR_SPECIFICATION;
                                abbrev_attributes->specification = read_ref_form (data, form, reader);
                        } else if (name == DW_AT_ABSTRACT_ORIGIN) {
                                abbrev_attributes->used |= ABBREV_ATTR_ABSTRACT_ORIGIN;
                                abbrev_attributes->abstract_origin = read_ref_form (data, form, reader);
                        } else {
                                /* uninteresting for now. */
                                skip_form (data, form, reader);
                        }
                }
                name = abbrev_reader->read_uleb128 (abbrev_reader);
                form = abbrev_reader->read_uleb128 (abbrev_reader);
        }


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



static int
dwarf2_lookup_data_initialize (struct dwarf2_lookup_data *data, 
                               uint32_t target_address,
                               struct reader *abbrev_reader, 
                               struct reader *reader)
{
        struct elf32_section_header section_header;

        if (elf32_parser_initialize (&data->elf32, reader) == -1) {
                printf ("elf32 init error\n");
                goto error;
        }
        
        if (lookup_cu (target_address, &data->cu_offset, &data->elf32, reader) == -1) {
                goto error;
        }

        if (elf32_parser_read_section_by_name (&data->elf32, ".debug_info", 
                                               &section_header,
                                               reader) == -1) {
                goto error;
        }
        data->debug_info_start = section_header.sh_offset;
        if (elf32_parser_read_section_by_name (&data->elf32, ".debug_str", 
                                               &section_header,
                                               reader) == -1) {
                goto error;
        }
        data->debug_str_start = section_header.sh_offset;

        reader->seek (reader, cuh_start (data));

        data->cuh.length = reader->read_u32 (reader);
        data->cuh.version = reader->read_u16 (reader);
        data->cuh.offset = reader->read_u32 (reader);
        data->cuh.address_size = reader->read_u8 (reader);

        if (elf32_parser_read_section_by_name (&data->elf32, ".debug_abbrev", 
                                               &section_header,
                                               reader) == -1) {
                goto error;
        }
        data->debug_abbrev_start = section_header.sh_offset;
        data->debug_abbrev_end = data->debug_abbrev_start + section_header.sh_size;

#ifdef ABBREV_CACHE
        cache_initialize (&data->cache);
#endif /* ABBREV_CACHE */

        return 0;
 error:
        return -1;
}
        
int 
dwarf2_lookup (uint64_t target_address, 
               struct dwarf2_symbol_information *symbol,
               struct reader *abbrev_reader,
               struct reader *reader)
{
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
}
