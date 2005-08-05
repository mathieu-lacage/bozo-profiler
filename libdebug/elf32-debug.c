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

#include "elf32-debug.h"
#include "elf32-constants-private.h"
#include "elf32-parser.h"

#define CASE_RETURN(x) \
  case x: \
    return #x; \
    break




char const *
e_type_to_string (uint16_t e_type)
{
        switch (e_type) {
        case ET_NONE:
                return "ET_NONE";
                break;
        case ET_REL:
                return "ET_REL";
                break;
        case ET_EXEC:
                return "ET_EXEC";
                break;
        case ET_DYN:
                return "ET_DYN";
                break;
        case ET_CORE:
                return "ET_CORE";
                break;
        default:
                if (e_type > ET_CORE &&
                    e_type < ET_LOOS) {
                        return "unknown";
                } else if (e_type >= ET_LOOS &&
                           e_type >= ET_HIOS) {
                        return "os-specific";
                } else if (e_type >= ET_LOPROC) {
                        return "processor-specific";
                }
                break;
        }
        return "DEADBEAF";
}

char const *
e_machine_to_string (uint16_t e_machine)
{
        switch (e_machine) {
                CASE_RETURN (EM_M32);
                CASE_RETURN (EM_SPARC);
                CASE_RETURN (EM_386);
                CASE_RETURN (EM_68K);
                CASE_RETURN (EM_88K);
                CASE_RETURN (EM_860);
                CASE_RETURN (EM_MIPS);
                CASE_RETURN (EM_MIPS_RS4_BE);
        default:
                return "unknown";
                break;
        }
}

void
elf32_section_header_print (struct elf32_section_header *section_header)
{
        printf ("sh_name: 0x%x\n", section_header->sh_name);
        printf ("sh_type: %s\n", sh_type_to_string (section_header->sh_type));
        printf ("sh_flags: 0x%x\n", section_header->sh_flags);
        printf ("sh_addr: 0x%x\n", section_header->sh_addr);
        printf ("sh_offset: 0x%x\n", section_header->sh_offset);
        printf ("sh_size: 0x%x\n", section_header->sh_size);
        printf ("sh_link: 0x%x\n", section_header->sh_link);
        printf ("sh_info: 0x%x\n", section_header->sh_info);
        printf ("sh_addralign: 0x%x\n", section_header->sh_addralign);
        printf ("sh_entsize: 0x%x\n", section_header->sh_entsize);
}

void
elf32_header_print (struct elf32_header *elf32_header)
{
        printf ("e_type: %s\n", e_type_to_string (elf32_header->e_type));
        printf ("e_machine: %s\n", e_machine_to_string (elf32_header->e_machine));
        printf ("e_entry: 0x%x\n", elf32_header->e_entry);
        printf ("e_phoff: 0x%x\n", elf32_header->e_phoff);
        printf ("e_shoff: 0x%x\n", elf32_header->e_shoff);
        printf ("e_flags: 0x%x\n", elf32_header->e_flags);
        printf ("e_ehsize: 0x%x\n", elf32_header->e_ehsize);
        printf ("e_phentsize: 0x%x\n", elf32_header->e_phentsize);
        printf ("e_phnum: 0x%x\n", elf32_header->e_phnum);
        printf ("e_shentsize: 0x%x\n", elf32_header->e_shentsize);
        printf ("e_shnum: 0x%x\n", elf32_header->e_shnum);
        printf ("e_shstrndx: 0x%x\n", elf32_header->e_shstrndx);
}

char const *
sh_type_to_string (uint32_t sh_type)
{
        switch (sh_type) {
                CASE_RETURN (SHT_NULL);
                CASE_RETURN (SHT_PROGBITS);
                CASE_RETURN (SHT_SYMTAB);
                CASE_RETURN (SHT_STRTAB);
                CASE_RETURN (SHT_RELA);
                CASE_RETURN (SHT_HASH);
                CASE_RETURN (SHT_DYNAMIC);
                CASE_RETURN (SHT_NOTE);
                CASE_RETURN (SHT_NOBITS);
                CASE_RETURN (SHT_REL);
                CASE_RETURN (SHT_SHLIB);
                CASE_RETURN (SHT_DYNSYM);
                CASE_RETURN (SHT_INIT_ARRAY);
                CASE_RETURN (SHT_FINI_ARRAY);
                CASE_RETURN (SHT_PREINIT_ARRAY);
                CASE_RETURN (SHT_GROUP);
                CASE_RETURN (SHT_SYMTAB_SHNDX);
        default:
                if (sh_type >= SHT_LOOS &&
                    sh_type <= SHT_HIOS) {
                        return "os-specific";
                } else if (sh_type >= SHT_LOPROC &&
                           sh_type <= SHT_HIPROC) {
                        return "processor-specific";
                } else if (sh_type >= SHT_LOUSER &&
                           sh_type <= SHT_HIUSER) {
                        return "user-specific";
                } else {
                        return "unknown";
                }
                break;
        }
}
