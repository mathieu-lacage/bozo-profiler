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

#define _GNU_SOURCE

#include "utils.h"
#include "null.h"
#include "memory-reader.h"
#include "mmap-cache.h"

#include <dlfcn.h>
#include <link.h>
#include <stdio.h>
#include <string.h>

struct link_map *
utils_get_link_map (void)
{
        void * handle;
        struct link_map *map;
        dlerror ();
        handle = dlopen (NULL, RTLD_LAZY);
        if (handle == NULL) {
                printf ("error getting handle for main binary: %s\n", dlerror ());
                goto error;
        }
        if (dlinfo (handle, RTLD_DI_LINKMAP, &map) == -1) {
                printf ("error getting link map: %s\n", dlerror ());
                dlclose (handle);
                goto error;
        }
        dlclose (handle);
        return map;
 error:
        return NULL;
}


struct r_debug *
utils_get_ld_debug(void)
{
        ElfW(Dyn) *dyn;
        for (dyn = _DYNAMIC; dyn->d_tag != DT_NULL; dyn++) {
                if (dyn->d_tag == DT_DEBUG) {
                        struct r_debug *r_debug;
                        r_debug = (struct r_debug *) dyn->d_un.d_ptr;
                        return r_debug;
                }
        }
        return NULL;
}

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int
utils_copy_process_name (char *str, uint32_t size)
{
        int fd;
        fd = open ("/proc/self/cmdline", O_RDONLY);
        while (size > 0) {
                ssize_t just_read;
                just_read = read (fd, str, 1);
                if (*str == 0) {
                        close (fd);
                        return 0;
                }
                size -= just_read;
                str++;
        }
        *(str-1) = 0;
        close (fd);
        return -1;
}

static struct mmap_cache *g_cache = NULL;

int
utils_filename_to_memory_reader (char const *filename, struct memory_reader *reader)
{
        struct mmap_cache *entry;
        if (strcmp (filename, "") == 0) {
                filename = "/proc/self/exe";
        }
        entry = mmap_cache_lookup (g_cache, filename);
        if (entry == NULL) {
                g_cache = mmap_cache_add_entry (g_cache, filename);
                if (g_cache == NULL) {
                        // memory error.
                        goto error;
                }
                entry = mmap_cache_lookup (g_cache, filename);
        }

        memory_reader_initialize (reader, entry->address, entry->size);
        return 0;
 error:
        return -1;
}


int
utils_get_iterator_for_filename (char const *filename, 
                                 struct elf32_symbol_iterator *i,
                                 struct elf32_header *elf32, 
                                 struct memory_reader *reader)
{
        if (utils_filename_to_memory_reader (filename, reader) == -1) {
                goto error;
        }
        if (elf32_parser_initialize (elf32, READER (reader)) == -1) {
                goto error;
        }
        if (elf32_symbol_iterator_initialize (elf32, i, READER (reader)) == -1) {
                goto error;
        }

        return 0;
 error:
        return -1;
}
