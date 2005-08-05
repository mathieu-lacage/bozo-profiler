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

#include "dwarf2-utils.h"
#include "dwarf2-constants.h"

char const *
dwarf2_utils_attr_name_to_string (uint64_t name)
{
        char const *str = "";

#define FOO(x) \
case DW_AT_##x: \
        str = #x; \
        break; \

        switch (name) {
        FOO(SIBLING);
        FOO(LOCATION);
        FOO(NAME);
        FOO(ORDERING);
        FOO(BYTE_SIZE);
        FOO(BIT_OFFSET);
        FOO(BIT_SIZE);
        FOO(STMT_LIST);
        FOO(LOW_PC);
        FOO(HIGH_PC);
        FOO(LANGUAGE);
        FOO(DISCR);
        FOO(DISCR_VALUE);
        FOO(VISIBILITY);
        FOO(IMPORT);
        FOO(STRING_LENGTH);
        FOO(COMMON_REFERENCE);
        FOO(COMP_DIR);
        FOO(CONST_VALUE);
        FOO(CONTAINING_TYPE);
        FOO(DEFAULT_VALUE);
        FOO(INLINE);
        FOO(IS_OPTIONAL);
        FOO(LOWER_BOUND);
        FOO(PRODUCER);
        FOO(PROTOTYPED);
        FOO(RETURN_ADDR);
        FOO(START_SCOPE);
        FOO(STRIDE_SIZE);
        FOO(UPPER_BOUND);
        FOO(ABSTRACT_ORIGIN);
        FOO(ACCESSIBILITY);
        FOO(ADDRESS_CLASS);
        FOO(ARTIFICIAL);
        FOO(BASE_TYPES);
        FOO(CALLING_CONVENTION);
        FOO(COUNT);
        FOO(DATA_MEMBER_LOCATION);
        FOO(DECL_COLUMN);
        FOO(DECL_FILE);
        FOO(DECL_LINE);
        FOO(DECLARATION);
        FOO(DISCR_LIST);
        FOO(ENCODING);
        FOO(EXTERNAL);
        FOO(FRAME_BASE);
        FOO(FRIEND);
        FOO(IDENTIFIER_CASE);
        FOO(MACRO_INFO);
        FOO(NAMELIST_ITEM);
        FOO(PRIORITY);
        FOO(SEGMENT);
        FOO(SPECIFICATION);
        FOO(STATIC_LINK);
        FOO(TYPE);
        FOO(USE_LOCATION);
        FOO(VARIABLE_PARAMETER);
        FOO(VIRTUALITY);
        FOO(VTABLE_ELEM_LOCATION);
        FOO(LO_USER);
        FOO(HI_USER);
#undef FOO
        }
        return str;
}
