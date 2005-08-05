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

#ifndef ELF32_PARSER_H
#define ELF32_PARSER_H

#include <stdint.h>
#include "reader.h"
#include "mbool.h"

#define ELF32_ST_TYPE(i)   ((i)&0xf)



enum {
        STT_NOTYPE  = 0,
        STT_OBJECT  = 1,
        STT_FUNC    = 2,
        STT_SECTION = 3,
        STT_FILE    = 4,
        STT_COMMON  = 5,
        STT_TLS     = 6,
        STT_LOOS    = 10,
        STT_HIOS    = 12,
        STT_LOPROC  = 13,
        STT_HIPROC  = 15
};

/* file type */
enum {
        ET_NONE   = 0,
	ET_REL    = 1,
	ET_EXEC   = 2,
	ET_DYN    = 3,
	ET_CORE   = 4,
	ET_LOOS   = 0xfe00,
	ET_HIOS   = 0xfeff,
	ET_LOPROC = 0xff00,
	ET_HIPROC = 0xffff
};

enum {
	SHN_UNDEF      = 0,
	SHN_LORESERVE  = 0xff00,
	SHN_LOPROC     = 0xff00,
	SHN_HIPROC     = 0xff1f,
	SHN_LOOS       = 0xff20,
	SHN_HIOS       = 0xff3f,
	SHN_ABS        = 0xfff1,
	SHN_COMMON     = 0xfff2,
	SHN_XINDEX     = 0xffff,
	SHN_HIRESERVE  = 0xffff
};

struct elf32_symbol {
        uint32_t st_name;
        uint32_t st_value;
        uint32_t st_size;
        uint8_t st_info;
        uint8_t st_other;
        uint16_t st_shndx;
};                                                                                                                                                             
struct elf32_section_header {
        uint32_t sh_name;
        uint32_t sh_type;
        uint32_t sh_flags;
        uint32_t sh_addr;
        uint32_t sh_offset;
        uint32_t sh_size;
        uint32_t sh_link;
        uint32_t sh_info;
        uint32_t sh_addralign;
        uint32_t sh_entsize;
};

struct elf32_header {
        uint16_t e_type;
        uint16_t e_machine;
        uint32_t e_entry;
        uint32_t e_phoff;
        uint32_t e_shoff;
        uint32_t e_flags;
        uint16_t e_ehsize;
        uint16_t e_phentsize;
        uint16_t e_phnum;
        uint16_t e_shentsize;
        uint16_t e_shnum;
        uint16_t e_shstrndx;
};

/**
 * reader is configured to read bytes from the symbol string name.
 * should return != 0 if it wants to stop the iteration.
 */
typedef int (*elf32_iter_cb) (struct elf32_symbol const *symbol, struct reader *reader, void *context);

/* return -1 on error. */
int elf32_parser_initialize (struct elf32_header *elf32_header, struct reader *reader);
/* 
 * Search for the first symbol which matches _exactly_ the requested name.
 */
int elf32_parser_find_symbol_slow (struct elf32_header *elf32_header, char const *name, 
                                   struct elf32_symbol *symbol, struct reader *reader);
/* store in start and end where the binary starts and ends in memory 
 * (relative to the base address, of course).
 */
int elf32_parser_get_bounds (struct elf32_header const *elf32, uint32_t *start, uint32_t *end, struct reader *reader);


int elf32_parser_read_section_by_name (struct elf32_header const *elf32, 
                                       char const *name,
                                       struct elf32_section_header *header, 
                                       struct reader *reader);

struct elf32_symbol_iterator {
        struct elf32_symbol symbol;
        uint32_t name_offset;
        /* private below */
        uint32_t i;
        uint32_t strtab_sh_offset;
        struct elf32_section_header symtab_section_header;
};

int   elf32_symbol_iterator_initialize (struct elf32_header *elf32, 
                                        struct elf32_symbol_iterator *i,
                                        struct reader *reader);
mbool elf32_symbol_iterator_has_next (struct elf32_symbol_iterator *i); 
int   elf32_symbol_iterator_next     (struct elf32_symbol_iterator *i, struct reader *reader); 
int   elf32_symbol_iterator_first    (struct elf32_symbol_iterator *i, struct reader *reader); 

#endif /* ELF32_PARSER_H */
