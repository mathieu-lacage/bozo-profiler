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

#include "dwarf2-abbrev.h"
#include "reader.h"
#include <string.h>
#include <assert.h>


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



enum search_abbrev_e {
        SEARCH_BY_TAG,
        SEARCH_BY_CODE
};

/* see section 7.5.3 dwarf2 p67 */
static bool
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
                        ABBREV_DEBUG_PRINTF ("\n");
                        ABBREV_DEBUG_PRINTF ("input: %llu, output: %u\n", searched_id, *retval);
                        return true;
                }

                /* skip the attribute specification */
                do {
                        name = reader->read_uleb128 (reader);
                        form = reader->read_uleb128 (reader);
                } while (name != 0 && form != 0);
        }
        ABBREV_DEBUG_PRINTF ("\n");
        return false;
}



void 
dwarf2_abbrev_initialize (struct dwarf2_abbrev *abbrev,
                          uint32_t abbrev_start,
                          uint32_t abbrev_end)
{
        abbrev->start = abbrev_start;
        abbrev->end = abbrev_end;
}

void 
dwarf2_abbrev_initialize_cu (struct dwarf2_abbrev *abbrev,
                             struct dwarf2_abbrev_cu *abbrev_cu,
                             uint32_t start)
{
        abbrev_cu->start = start;
        abbrev_cu->end = abbrev->end;
        cache_initialize (&abbrev_cu->cache);
}

void
dwarf2_abbrev_cu_read_decl (struct dwarf2_abbrev_cu *abbrev_cu,
                            struct dwarf2_abbrev_decl *decl,
                            uint64_t abbr_code,
                            struct reader *reader)
{
        uint32_t abbrev_start;
        uint32_t abbrev_end;
        uint32_t abbrev_offset;

        abbrev_start = abbrev_cu->start;
        abbrev_end = abbrev_cu->end;

        if (cache_try_lookup (abbr_code, &abbrev_offset, &abbrev_cu->cache) == -1) {
                abbrev_offset = abbrev_start;
        }
        reader->seek (reader, abbrev_offset);
        if (reader->read_uleb128 (reader) != abbr_code) {
                if (search_abbrev_start_to_end (abbr_code, SEARCH_BY_CODE, 
                                                abbrev_offset, abbrev_end,
                                                &abbrev_offset,
                                                reader)) {
                        /* found abbr_code from abbrev_offset to abbrev_end */
                        cache_try_add (abbr_code, abbrev_offset, &abbrev_cu->cache);
                } else if (abbrev_offset == abbrev_start) {
                        /* did not find abbr_code from abbrev_start to abbrev_end */
                        goto error;
                } else if (search_abbrev_start_to_end (abbr_code, SEARCH_BY_CODE, 
                                                        abbrev_start, abbrev_offset, 
                                                        &abbrev_offset,
                                                        reader)) {
                        /* found abbr_code from abbrev_start to abbrev_offset */
                        cache_try_add (abbr_code, abbrev_offset, &abbrev_cu->cache);
                } else {
                        /* Did not find abbr_code from 
                         *   - abbrev_offset to abbrev_end
                         *   - abbrev_start to abbrev_offset
                         * where abbrev_start <= abbrev_offset <= abbrev_end
                         */
                        goto error;
                }
        }
        decl->offset = abbrev_offset;
        reader->seek (reader, abbrev_offset);
        decl->abbr_code = reader->read_uleb128 (reader);
        decl->tag = reader->read_uleb128 (reader);
        decl->children = reader->read_u8 (reader);

        assert (decl->abbr_code == abbr_code);

        return;
 error:
        reader->status = -1;
}

void 
dwarf2_abbrev_decl_read_attr_first (struct dwarf2_abbrev_decl *decl,
                                    struct dwarf2_abbrev_attr *attr,
                                    uint32_t *new_offset,
                                    struct reader *reader)
{
        uint64_t code, tag;
        uint8_t children;
        reader->seek (reader, decl->offset);

        code = reader->read_uleb128 (reader);
        tag = reader->read_uleb128 (reader);
        children = reader->read_u8 (reader);
        assert (decl->abbr_code == code);
        assert (decl->tag == tag);
        assert (decl->children == children);

        attr->name = reader->read_uleb128 (reader);
        attr->form = reader->read_uleb128 (reader);
        *new_offset = reader->get_offset (reader);
}

void 
dwarf2_abbrev_read_attr (uint32_t cur_offset,
                         struct dwarf2_abbrev_attr *attr,
                         uint32_t *new_offset,
                         struct reader *reader)
{
        reader->seek (reader, cur_offset);
        attr->name = reader->read_uleb128 (reader);
        attr->form = reader->read_uleb128 (reader);
        *new_offset = reader->get_offset (reader);
}

bool 
dwarf2_abbrev_attr_is_last (struct dwarf2_abbrev_attr *attr)
{
        if (attr->name == 0 && attr->form == 0) {
                return true;
        }
        return false;
}

