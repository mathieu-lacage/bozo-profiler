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
dwarf2_utils_tag_to_string (uint64_t tag)
{
        char const *str = "";

#define FOO(x) \
case DW_TAG_##x: \
        str = #x; \
        break; 

        switch (tag) {
                DW_TAG_ARRAY_TYPE);
        DW_TAG_CLASS_TYPE);
DW_TAG_ENTRY_POINT);
        DW_TAG_ENUMERATION_TYPE
        DW_TAG_FORMAL_PARAMETER
        DW_TAG_IMPORTED_DECLARATION
        DW_TAG_LABEL                     = 0x0a,
        DW_TAG_LEXICAL_BLOCK             = 0x0b,
        DW_TAG_MEMBER                    = 0x0d,
        DW_TAG_POINTER_TYPE              = 0x0f,
        DW_TAG_REFERENCE_TYPE            = 0x10,
        DW_TAG_COMPILE_UNIT              = 0x11,
        DW_TAG_STRING_TYPE               = 0x12,
        DW_TAG_STRUCTURE_TYPE            = 0x13,
        DW_TAG_SUBROUTINE_TYPE           = 0x15,
        DW_TAG_TYPEDEF                   = 0x16,
        DW_TAG_UNION_TYPE                = 0x17,
        DW_TAG_UNSPECIFIED_PARAMETERS    = 0x18,
        DW_TAG_VARIANT                   = 0x19,
        DW_TAG_COMMON_BLOCK              = 0x1a,
        DW_TAG_COMMON_INCLUSION          = 0x1b,
        DW_TAG_INHERITANCE               = 0x1c,
        DW_TAG_INLINED_SUBROUTINE        = 0x1d,
        DW_TAG_MODULE                    = 0x1e,
        DW_TAG_PTR_TO_MEMBER_TYPE        = 0x1f,
        DW_TAG_SET_TYPE                  = 0x20,
        DW_TAG_SUBRANGE_TYPE             = 0x21,
        DW_TAG_WITH_STMT                 = 0x22,
        DW_TAG_ACCESS_DECLARATION        = 0x23,
        DW_TAG_BASE_TYPE                 = 0x24,
        DW_TAG_CATCH_BLOCK               = 0x25,
        DW_TAG_CONST_TYPE                = 0x26,
        DW_TAG_CONSTANT                  = 0x27,
        DW_TAG_ENUMERATOR                = 0x28,
        DW_TAG_FILE_TYPE                 = 0x29,
        DW_TAG_FRIEND                    = 0x2a,
        DW_TAG_NAMELIST                  = 0x2b,
        DW_TAG_NAMELIST_ITEM             = 0x2c,
        DW_TAG_PACKED_TYPE               = 0x2d,
        DW_TAG_SUBPROGRAM                = 0x2e,
        DW_TAG_TEMPLATE_TYPE_PARAM       = 0x2f,
        DW_TAG_TEMPLATE_VALUE_PARAM      = 0x30,
        DW_TAG_THROWN_TYPE               = 0x31,
        DW_TAG_TRY_BLOCK                 = 0x32,
        DW_TAG_VARIANT_PART              = 0x33,
        DW_TAG_VARIABLE                  = 0x34,
        DW_TAG_VOLATILE_TYPE             = 0x35,
        DW_TAG_LO_USER                   = 0x4080,
        DW_TAG_HI_USER                   = 0xffff,
                
        }

#undef FOO
        return str;
}

char const *
dwarf2_utils_attr_name_to_string (uint64_t name)
{
        char const *str = "";

#define FOO(x) \
case DW_AT_##x: \
        str = #x; \
        break; 

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
