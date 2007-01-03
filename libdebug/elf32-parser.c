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

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "elf32-parser.h"
#include "elf32-debug.h"
#include "elf32-constants-private.h"
#include "reader.h"

#define noERROR_REPORT

#ifdef yesERROR_REPORT
  #define ERROR_REPORT(str, ...) \
    printf ("DEBUG "  __FILE__  ":%d " str "\n", __LINE__, ## __VA_ARGS__);
#else
  #define ERROR_REPORT(str, ...)
#endif


/* version */
#define EV_NONE     0
#define EV_CURRENT  1


#define STN_UNDEF 0


#define PT_NULL     0
#define PT_LOAD     1
#define PT_DYNAMIC  2
#define PT_INTERP   3
#define PT_NOTE     4
#define PT_SHLIB    5
#define PT_PHDR     6
#define PT_TLS      7
#define PT_LOOS     0x60000000
#define PT_HIOS     0x6fffffff
#define PT_LOPROC   0x70000000
#define PT_HIPROC   0x7fffffff


#define ASCII_E 0x45
#define ASCII_L 0x4C
#define ASCII_F 0x46

struct elf32_program_header {
        uint32_t p_type;
        uint32_t p_off;
        uint32_t p_vaddr;
        uint32_t p_paddr;
        uint32_t p_filesz;
        uint32_t p_memsz;
        uint32_t p_flags;
        uint32_t p_align;
};



#if 0
/**
 * This magic hash function is defined in the ELF spec.
 */
static unsigned long
elf_hash (unsigned char const *name)
{
        unsigned long h = 0, g;
        while (*name != 0) {
                h = (h << 4) + *name++;
                if ((g = (h & 0xf0000000))) {
                        h ^= g >> 24;
                }
                h &= ~g;
        }
        return h;
}
#endif


int elf32_parser_initialize (struct elf32_header *elf32_header, struct reader *reader)
{
        uint8_t b;

        reader->seek (reader, 0);

        b = reader->read_u8 (reader);
        if (b != 0x7f) {
                ERROR_REPORT ("invalid ELF magic number");
                goto error;
        }
        b = reader->read_u8 (reader);
        if (b != ASCII_E) {
                ERROR_REPORT ("invalid ELF E");
                goto error;
        }
        b = reader->read_u8 (reader);
        if (b != ASCII_L) {
                ERROR_REPORT ("invalid ELF L");
                goto error;
        }
        b = reader->read_u8 (reader);
        if (b != ASCII_F) {
                ERROR_REPORT ("invalid ELF F");
                goto error;
        }
        /* ei_class */
        b = reader->read_u8 (reader);
        if (b != 1) {
                ERROR_REPORT ("invalid ei_class");
                goto error;
        }
        /* type */
        b = reader->read_u8 (reader);
        if (b == 1) {
                reader->set_lsb (reader);
        } else if (b == 2) {
                reader->set_msb (reader);
        } else {
                ERROR_REPORT ("invalid byte order type");
                goto error;
        }
        /* ei_version */
        b = reader->read_u8 (reader);
        if (b != EV_CURRENT) {
                ERROR_REPORT ("invalid ei_version");
                goto error;
        }
        reader->skip (reader, 9);
        elf32_header->e_type = reader->read_u16 (reader);
        if (elf32_header->e_type == ET_NONE) {
                ERROR_REPORT ("no file type");
                goto error;
        } else if (elf32_header->e_type > ET_CORE &&
                   elf32_header->e_type < ET_LOOS) {
                ERROR_REPORT ("unknown file type: %x", elf32_header->e_type);
                goto error;
        } else if (elf32_header->e_type >= ET_LOOS &&
                   elf32_header->e_type <= ET_HIOS) {
                ERROR_REPORT ("os-specific file");
                goto error;
        } else if (elf32_header->e_type >= ET_LOPROC) {
                ERROR_REPORT ("processor-specific file");
                goto error;
        }
        elf32_header->e_machine = reader->read_u16 (reader);
        if (elf32_header->e_machine != EM_386) {
                ERROR_REPORT ("untested machine type %d", elf32_header->e_machine);
                goto error;
        }

        reader->skip (reader, 4);
        elf32_header->e_entry = reader->read_u32 (reader);
        elf32_header->e_phoff = reader->read_u32 (reader);
        elf32_header->e_shoff = reader->read_u32 (reader);
        elf32_header->e_flags = reader->read_u32 (reader);
        elf32_header->e_ehsize = reader->read_u16 (reader);
        elf32_header->e_phentsize = reader->read_u16 (reader);
        elf32_header->e_phnum = reader->read_u16 (reader);
        elf32_header->e_shentsize = reader->read_u16 (reader);
        elf32_header->e_shnum = reader->read_u16 (reader);
        elf32_header->e_shstrndx = reader->read_u16 (reader);

        /* elf32_header_print (elf32_header); */

        if (reader->status < 0) {
                ERROR_REPORT ("invalid file");
                goto error;
        }

        return 0;
 error:
        return -1;
}

static int
read_symbol (struct elf32_section_header const *symtab_section_header,
             uint32_t index, struct elf32_symbol *symbol, struct reader *reader)
{
        if ((symtab_section_header->sh_size / symtab_section_header->sh_entsize) <= index) {
                ERROR_REPORT ("symbol index invalid: %d >= %d", index, 
                              (symtab_section_header->sh_size / symtab_section_header->sh_entsize));
                goto error;
        }
        reader->seek (reader, symtab_section_header->sh_offset);
        reader->skip (reader, index * symtab_section_header->sh_entsize);
        symbol->st_name  = reader->read_u32 (reader);
        symbol->st_value = reader->read_u32 (reader);
        symbol->st_size  = reader->read_u32 (reader);
        symbol->st_info  = reader->read_u8 (reader);
        symbol->st_other = reader->read_u8 (reader);
        symbol->st_shndx = reader->read_u16 (reader);

        if (reader->status < 0) {
                ERROR_REPORT ("invalid file");
                goto error;
        }
        
        return 0;
 error:
        return -1;
}

static int
read_nsections (struct elf32_header const *elf32_header, uint32_t *nsections, struct reader *reader)
{
        if (elf32_header->e_shoff == 0 || 
            elf32_header->e_shentsize == 0) {
                /* is this at all possible ? */
                printf ("impossible thing happened\n");
                *nsections = 0;
        } else if (elf32_header->e_shnum == SHN_UNDEF) {
                uint32_t sh_size;
                // number of section header table entries is bigger than
                // SHN_LORESERVE. real number of section header table entries
                // is contained in first entry's sh_size.
                reader->seek (reader, elf32_header->e_shoff);
                reader->skip (reader, 4+4+4+4+4);
                sh_size = reader->read_u32 (reader);
                *nsections = sh_size;
        } else {
                *nsections = elf32_header->e_shnum;
        }
        if (reader->status < 0) {
                return -1;
        } else {
                return 0;
        }
}

static int
read_section_header_by_number (struct elf32_header const *elf32_header, 
                               struct elf32_section_header *section_header, 
                               uint32_t n, struct reader *reader)
{
        uint32_t nsections;
        if (elf32_header->e_shoff == 0 || 
            elf32_header->e_shentsize == 0 ||
            read_nsections (elf32_header, &nsections, reader) == -1 ||
            nsections < n) {
                goto error;
        }

        reader->seek (reader, elf32_header->e_shoff);
        reader->skip (reader, elf32_header->e_shentsize * n);

        section_header->sh_name = reader->read_u32 (reader);
        section_header->sh_type = reader->read_u32 (reader);
        section_header->sh_flags = reader->read_u32 (reader);
        section_header->sh_addr = reader->read_u32 (reader);
        section_header->sh_offset = reader->read_u32 (reader);
        section_header->sh_size = reader->read_u32 (reader);
        section_header->sh_link = reader->read_u32 (reader);
        section_header->sh_info = reader->read_u32 (reader);
        section_header->sh_addralign = reader->read_u32 (reader);
        section_header->sh_entsize = reader->read_u32 (reader);

        if (reader->status < 0) {
                ERROR_REPORT ("error reading section");
                goto error;
        }
        return 0;
 error:
        return -1;
}

static int 
read_section_header_by_type (struct elf32_header const *elf32_header, 
                             struct elf32_section_header *ret_section_header, 
                             uint32_t sh_type, struct reader *reader)
{
        uint32_t i;
        int status;
        uint32_t nsections;
        if (read_nsections (elf32_header, &nsections, reader) == -1) {
                goto error;
        }
        for (i = 0; i < nsections; i++) {
                struct elf32_section_header tmp_section_header;
                status = read_section_header_by_number (elf32_header, &tmp_section_header, i, reader);
                if (status == -1) {
                        ERROR_REPORT ("error reading section number %d", i);
                        goto error;
                }
                if (tmp_section_header.sh_type == sh_type) {
                        *ret_section_header = tmp_section_header;
                        goto ok;
                }
        }
 error:
        ERROR_REPORT ("did not find section type %s", sh_type_to_string (sh_type));
        return -1;
 ok:
        if (reader->status < 0) {
                ERROR_REPORT ("invalid file");
                goto error;
        }
        return 0;
}

bool 
elf32_symbol_iterator_has_next (struct elf32_symbol_iterator *i)
{
        uint32_t nsymbols;
        nsymbols = i->symtab_section_header.sh_size / 
                i->symtab_section_header.sh_entsize;
        if (i->i < nsymbols) {
                return true;
        } else {
                return false;
        }
        
}

int
elf32_symbol_iterator_next (struct elf32_symbol_iterator *i,
                            struct reader *reader)
{
        i->i++;
        if (!elf32_symbol_iterator_has_next (i)) {
                goto error;
        }
        if (read_symbol (&i->symtab_section_header, i->i, 
                         &i->symbol, reader) == -1) {
                ERROR_REPORT ("unable to find symbol in symbol table");
                goto error;
        }
        i->name_offset = i->strtab_sh_offset + i->symbol.st_name;

        return 0;
 error:
        return -1;
}

int
elf32_symbol_iterator_first (struct elf32_symbol_iterator *i,
                             struct reader *reader)
{
        i->i = 0;
        if (read_symbol (&i->symtab_section_header, i->i, 
                         &i->symbol, reader) == -1) {
                ERROR_REPORT ("unable to find symbol in symbol table");
                goto error;
        }
        i->name_offset = i->strtab_sh_offset + i->symbol.st_name;
        return 0;
 error:
        return -1;
}

int 
elf32_symbol_iterator_initialize (struct elf32_header *elf32, 
                                  struct elf32_symbol_iterator *i,
                                  struct reader *reader)
{
        struct elf32_section_header strtab_section_header;
        if (read_section_header_by_type (elf32, 
                                         &i->symtab_section_header, 
                                         SHT_SYMTAB, 
                                         reader) == -1) {
                if (read_section_header_by_type (elf32, 
                                                 &i->symtab_section_header, 
                                                 SHT_DYNSYM, 
                                                 reader) == -1) {
                        ERROR_REPORT ("unable to find symbol table section");
                        goto error;
                }
        }
        if (read_section_header_by_number (elf32, 
                                           &strtab_section_header, 
                                           i->symtab_section_header.sh_link, 
                                           reader) == -1) {
                ERROR_REPORT ("unable to find symbol name table");
                goto error;
        }
        i->strtab_sh_offset = strtab_section_header.sh_offset;

        return elf32_symbol_iterator_first (i, reader);
 error:
        return -1;
}


static int
compare_string (char const *name, struct reader *reader)
{
        int found;
        uint32_t j;
        char c;
        
        j = 0;
        found = false;
        c = (char) reader->read_u8 (reader);
        while (c != 0 && name[j] != 0) {
                /*printf ("%c -- %c\n", c, name[j]);*/
                if (c != name[j]) {
                        found = false;
                        break;
                } else {
                        found = true;
                }
                c = (char) reader->read_u8 (reader);
                j++;
        }
        if (c != name[j]) {
                found = false;
        }
        return found;
}

int 
elf32_parser_find_symbol_slow (struct elf32_header *elf32, char const *name, 
                               struct elf32_symbol *symbol, struct reader *reader)
{
        struct elf32_symbol_iterator i;

        if (elf32_symbol_iterator_initialize (elf32, &i, reader) == -1) {
                goto error;
        }
        while (elf32_symbol_iterator_has_next (&i)) {
                reader->seek (reader, i.name_offset);
                if (compare_string (name, reader)) {
                        goto ok;
                }
                elf32_symbol_iterator_next (&i, reader);
        }
 error:
        return -1;
 ok:
        if (reader->status < 0) {
                goto error;
        }
        *symbol = i.symbol;
        return 0;
}

static int
read_ph (struct elf32_header const *elf32, 
         struct elf32_program_header *ph,
         uint16_t n, struct reader *reader)
{
        reader->seek (reader, elf32->e_phoff);
        reader->skip (reader, n * elf32->e_phentsize);

        ph->p_type = reader->read_u32 (reader);
        ph->p_off = reader->read_u32 (reader);
        ph->p_vaddr = reader->read_u32 (reader);
        ph->p_paddr = reader->read_u32 (reader);
        ph->p_filesz = reader->read_u32 (reader);
        ph->p_memsz = reader->read_u32 (reader);
        ph->p_flags = reader->read_u32 (reader);
        ph->p_align = reader->read_u32 (reader);

        if (reader->status == -1) {
                return -1;
        }

        return 0;
}

int 
elf32_parser_get_bounds (struct elf32_header const *elf32, 
                         uint32_t *start, uint32_t *end, 
                         struct reader *reader)
{
        uint16_t i;
        /**
         * XXX I should be careful with integer overflow below.
         * It might be possible that the p_vaddr + p_memsz addition
         * overflow which would lead to really bad things.
         */
        struct elf32_program_header ph;
        *start = 0xffffffff;
        *end = 0;
        for (i = 0; i < elf32->e_phnum; i++) {
                if (read_ph (elf32, &ph, i, reader) == -1) {
                        goto error;
                }
                if (ph.p_type == PT_LOAD    ||
                    ph.p_type == PT_DYNAMIC ||
                    ph.p_type == PT_INTERP  ||
                    ph.p_type == PT_NOTE    ||
                    ph.p_type == PT_SHLIB   ||
                    ph.p_type == PT_PHDR    ||
                    ph.p_type == PT_TLS) {
                        //printf ("vaddr 0x%x -- memsz 0x%x\n", ph.p_vaddr, ph.p_memsz);
                        if (ph.p_vaddr < *start) {
                                *start = ph.p_vaddr;
                        }
                        if (ph.p_vaddr + ph.p_memsz > *end) {
                                *end = ph.p_vaddr + ph.p_memsz;
                        }
                }
        }
        //printf ("%x -- %x\n", *start, *end);
        return 0;
 error:
        return -1;
}


int 
elf32_parser_read_section_by_name (struct elf32_header const *elf32_header, 
                                   char const *name,
                                   struct elf32_section_header *section_header, 
                                   struct reader *reader)
{
        int status;
        uint32_t name_section_index;
        uint32_t nsections, i;
        struct elf32_section_header name_section_header;
 
        if (elf32_header->e_shstrndx < SHN_LORESERVE) {
                name_section_index = elf32_header->e_shstrndx; 
        } else if (elf32_header->e_shstrndx == SHN_XINDEX) {
                /* the index number of the section which contains 
                 * the strings for the section names is stored in 
                 * the sh_link field of the section header number 
                 * zero.
                 */
                status = read_section_header_by_number (elf32_header, 
                                                        section_header, 
                                                        0, reader);
                if (status == -1) {
                        ERROR_REPORT ("could not read section zero");
                        goto error;
                }
                name_section_index = section_header->sh_link;
        } else if (elf32_header->e_shstrndx == SHN_UNDEF) {
                ERROR_REPORT ("no string table for section names");
                goto error;
        } else {
                ERROR_REPORT ("invalid string table index");
                goto error;
        }
        status = read_section_header_by_number (elf32_header, &name_section_header,
                                                name_section_index, reader);
        if (status == -1) {
                printf ("idx: %x\n", name_section_index);
                ERROR_REPORT ("could not read name section header");
                goto error;
        }
        if (name_section_header.sh_type == SHT_NOBITS) {
                ERROR_REPORT ("invalid section header for section names section");
                goto error;
        }
        if (read_nsections (elf32_header, &nsections, reader) == -1) {
                goto error;
        }
        for (i = 0; i < nsections; i++) {
                struct elf32_section_header tmp_section_header;
                status = read_section_header_by_number (elf32_header, &tmp_section_header, i, reader);
                if (status == -1) {
                        ERROR_REPORT ("error reading section number %d", i);
                        goto error;
                }
                reader->seek (reader, name_section_header.sh_offset);
                reader->skip (reader, tmp_section_header.sh_name);

                if (compare_string (name, reader)) {
                        *section_header = tmp_section_header;
                        goto ok;
                }
        }        
 error:
        return -1;
 ok:
        if (reader->status < 0) {
                ERROR_REPORT ("invalid file");
                goto error;
        }
        return 0;
}
