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

  Copyright (C) 2005 Mathieu Lacage
  Author: Mathieu Lacage <lacage@sophia.inria.fr>
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

struct bb_start_tmp {
        bool basic_block;
        uint64_t bb_ad;
};

static void 
report_state (struct dwarf2_line_machine_state *state, void *data)
{
        struct bb_start_tmp *tmp = (struct bb_start_tmp *)data;
        if (tmp->basic_block && 
            (tmp->bb_ad != state->address ||
             state->address == 0)) {
                printf ("ad: 0x%llx\n", state->address);
                tmp->basic_block = false;
        }
        if (state->basic_block) {
                tmp->basic_block = true;
                tmp->bb_ad = state->address;
        }
}


int 
test_dw2_bb (int argc, char *argv[])
{
        char const *filename = argv[0];
	int fd;
        struct stat stat_buf;
        uint32_t size;
        uint8_t *data;
        struct memory_reader reader;
        struct bb_start_tmp tmp;


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

        tmp.basic_block = false;
        dwarf2_line_get_all_states (report_state, &tmp, READER(&reader));


 error:
	return -1;
}
