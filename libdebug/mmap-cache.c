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

#include "mmap-cache.h"

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>



struct mmap_cache *
mmap_cache_lookup (struct mmap_cache *cache, char const *filename)
{
        struct mmap_cache *tmp;
        if (cache == NULL) {
                return NULL;
        }
        for (tmp = cache; tmp != NULL; tmp = tmp->next) {
                if (strcmp (tmp->filename, filename) == 0) {
                        return tmp;
                }
        }
        return NULL;
}


struct mmap_cache *
mmap_cache_add_entry (struct mmap_cache *cache, char const *filename)
{
        struct mmap_cache *new;
        struct stat stat_buf;
        int len;
        int fd;
        len = strlen (filename)+1;
        new = (struct mmap_cache *) malloc (sizeof (struct mmap_cache) + len);
        if (new == NULL) {
                goto error;
        }
        new->filename = (char const *)new + sizeof (struct mmap_cache);
        memcpy ((char *)new->filename, filename, len);
        fd = open (filename, O_RDONLY);
        if (fd == -1) {
                printf ("error opening \"%s\"\n", filename);
                free (new);
                goto error;
        }
        if (fstat (fd, &stat_buf) == -1) {
                printf ("unable to stat \"%s\"\n", filename);
                close (fd);
                free (new);
                goto error;
        }
        new->address = mmap (0, stat_buf.st_size, PROT_READ, MAP_SHARED, fd, 0);
        new->size = stat_buf.st_size;
        close (fd);

        new->next = cache;
        return new;
 error:
        return NULL;
}


void 
mmap_cache_flush (struct mmap_cache *cache)
{
        struct mmap_cache *tmp;
        for (tmp = cache; tmp->next != NULL; tmp = tmp->next) {
                munmap (tmp->address, tmp->size);
                free (tmp);
        }        
}
