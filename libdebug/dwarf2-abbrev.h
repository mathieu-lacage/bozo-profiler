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

#ifndef DWARF2_ABBREV_H
#define DWARF2_ABBREV_H

#include <stdint.h>
#include <stdbool.h>
struct reader;

struct dwarf2_abbrev {
        uint32_t start; /* start of .debug_abbrev from start of file */
        uint32_t end;   /* end of .debug_abbrev from start of file */
};

#define CACHE_SIZE (16)
struct dwarf2_abbrev_cu {
        uint32_t start; /* offset from start of file */
        uint32_t end;   /* offset from start of file */
        struct cache {
                uint8_t keys[CACHE_SIZE];
                uint32_t values[CACHE_SIZE];
                uint8_t last_used[CACHE_SIZE];
                uint8_t time;
        } cache;
};

struct dwarf2_abbrev_decl {
        uint32_t offset;     /* offset from start of file to this decl. */
        uint64_t abbr_code;  /* abbrev code for this entry */
        uint64_t tag;        /* abbrev tag for this entry */
        uint8_t children;    /* children for this entry */
};

struct dwarf2_abbrev_attr {
        uint64_t form;
        uint64_t name;
};


void dwarf2_abbrev_initialize (struct dwarf2_abbrev *abbrev,
                               uint32_t abbrev_start /* offset from start of file */,
                               uint32_t abbrev_end /* offset from start of file */);

void dwarf2_abbrev_initialize_cu (struct dwarf2_abbrev *abbrev,
                                  struct dwarf2_abbrev_cu *abbrev_cu,
                                  uint32_t offset /* offset from start of file */);

void dwarf2_abbrev_cu_read_decl (struct dwarf2_abbrev_cu *abbrev_cu,
                                 struct dwarf2_abbrev_decl *decl,
                                 uint64_t code,
                                 struct reader *reader);

void dwarf2_abbrev_decl_read_attr_first (struct dwarf2_abbrev_decl *decl,
                                         struct dwarf2_abbrev_attr *attr,
                                         uint32_t *new_offset,
                                         struct reader *reader);
void dwarf2_abbrev_read_attr (uint32_t cur_offset,
                              struct dwarf2_abbrev_attr *attr,
                              uint32_t *new_offset,
                              struct reader *reader);
bool dwarf2_abbrev_attr_is_last (struct dwarf2_abbrev_attr *attr);


#endif /* DWARF2_ABBREV_H */
