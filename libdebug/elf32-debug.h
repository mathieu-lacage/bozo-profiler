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

#ifndef ELF32_DEBUG_H
#define ELF32_DEBUG_H

#include <stdint.h>
#include "elf32-parser.h"


char const *sh_type_to_string (uint32_t sh_type);
char const *e_type_to_string (uint16_t e_type);
char const *e_machine_to_string (uint16_t e_machine);
void elf32_section_header_print (struct elf32_section_header *section_header);
void elf32_header_print (struct elf32_header *elf32_header);


#endif /* ELF32_DEBUG_H */
