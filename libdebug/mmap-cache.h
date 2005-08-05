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

#ifndef MMAP_CACHE_H
#define MMAP_CACHE_H

#include <stdint.h>

struct mmap_cache {
        char const *filename;
        uint8_t *address;
        uint32_t size;
        struct mmap_cache *next;
};

/* Return the cache entry which matches the input filename. 
 * If there is none, it returns NULL.
 */
struct mmap_cache *mmap_cache_lookup (struct mmap_cache *cache, char const *filename);
/* Add an entry to cache. This function returns the new cache 
 * which contains the new cache entry.
 */
struct mmap_cache *mmap_cache_add_entry (struct mmap_cache *cache, char const *filename);
/* Kill all cache entries.
 */
void mmap_cache_flush (struct mmap_cache *cache);

#endif /* MMAP_CACHE_H */
