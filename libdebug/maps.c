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

#include <stdint.h>
#include <stdio.h>

#include "mbool.h"


static int
inverse_partial_cmp (char const *short_name, char const *long_name)
{
        int short_len;
        int long_len;
        int i;
        int found;
        short_len = strlen (short_name);
        long_len = strlen (long_name);
        found = FALSE;
        i = 0;
        while (short_len >= 0) {
                if (short_name[short_len-i] != long_name[long_len-i]) {
                        found = FALSE;
                        break;
                } else {
                        found = TRUE;
                }
                i++;
        }
        return found;
}

int 
maps_search_start (char const *object_name, unsigned long *map_start, unsigned long*map_end)
{
        char buffer[1024];
        char file[255];
        FILE *in;
        char perms[26];
        unsigned long start, end;
        unsigned int major, minor;
        unsigned int offset;
        unsigned long inode;
        int len;

        len = strlen (object_name);
        
        in = fopen ("/proc/self/maps", "r");
        while (fgets(buffer, 1023, in)) {
                sscanf (buffer, "%lx-%lx %15s %8x %u:%u %lu %255s",
                        &start, &end, perms, &offset, &major, &minor, &inode, file);
                printf ("%s\n", file);
                if (inverse_partial_cmp (object_name, file)) {
                        printf ("%s -- %s --> 0x%x/0x%x\n", object_name, file, start, end);
                        *map_start = start;
                        *map_end = end;
                        return 0;
                }
        }
        fclose (in);
        return -1;
}
