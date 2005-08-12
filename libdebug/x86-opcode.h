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

  Copyright (C) 2005  Mathieu Lacage
  Author: Mathieu Lacage <mathieu@gnu.org>
*/

#ifndef X86_OPCODE
#define X86_OPCODE

#include <stdint.h>

enum x86_mode_e {
        X86_MODE_16,
        X86_MODE_32,
        X86_MODE_64
};

struct x86_opcode_parser {
        enum x86_state_e {
                X86_STATE_PREFIX_OR_OPCODE0,
                X86_STATE_OPCODE1,
                X86_STATE_MODRM,
                X86_STATE_SIB,
                X86_STATE_DISPLACEMENT,
                X86_STATE_IMMEDIATE,
                X86_STATE_DONE,
                X86_STATE_ERROR
        } state;
        enum x86_prefixes_e {
                X86_PREFIX_LOCK   = (1<<0),
                X86_PREFIX_REPNEZ = (1<<1),
                X86_PREFIX_REPEZ  = (1<<2),
                X86_PREFIX_CS     = (1<<3),
                X86_PREFIX_SS     = (1<<4),
                X86_PREFIX_DS     = (1<<5),
                X86_PREFIX_ES     = (1<<6),
                X86_PREFIX_FS     = (1<<7),
                X86_PREFIX_GS     = (1<<8),
                X86_PREFIX_BRANCH_TAKEN     = (1<<9),
                X86_PREFIX_BRANCH_NOT_TAKEN = (1<<10),
                X86_PREFIX_OP_SIZE = (1<<11),
                X86_PREFIX_AD_SIZE = (1<<12),
                X86_PREFIX_REX     = (1<<13)
        } prefixes;
        uint8_t opcode0;
        uint8_t opcode1;
        uint8_t sib;
        uint8_t modrm;
        uint64_t displacement;
        uint64_t immediate;
        enum x86_mode_e mode;
        uint8_t tmp;
};

void x86_opcode_initialize (struct x86_opcode_parser *parser, enum x86_mode_e);

/* returns now many bytes were read.
 */
uint32_t x86_opcode_parse (struct x86_opcode_parser *parser, 
                           uint8_t *buffer, uint32_t size);

void x86_opcode_print (struct x86_opcode_parser *parser);

int x86_opcode_ok (struct x86_opcode_parser *parser);
int x86_opcode_error (struct x86_opcode_parser *parser);


#ifdef RUN_SELF_TESTS
int x86_opcode_run_self_tests (void);
#endif /* RUN_SELF_TESTS */

#endif /* X86_OPCODE */
