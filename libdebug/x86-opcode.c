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

#include <stdio.h>
#include <assert.h>

#include "x86-opcode.h"

#define X86_TRACE 1

static char const *
state_to_string (enum x86_state_e state)
{
#define FOO(x) \
 case X86_STATE_##x: \
         return #x; \
         break;

        switch (state) {
                FOO (PREFIX_OR_OPCODE0);
                FOO (OPCODE1);
                FOO (MODRM);
                FOO (SIB);
                FOO (DISPLACEMENT);
                FOO (IMMEDIATE);
                FOO (ERROR);
        }
#undef FOO
        assert (0);
        return "0xdeadbeaf";
}

enum x86_data_type {
        X86_DATA_NONE,
        X86_DATA_BYTE,
        X86_DATA_WORD
};

/* Calculates if modrm is present in 2-byte opcodes
 * when the first byte of the 2-byte opcode is 0xff.
 * This bitmap table was built from the intel opcode 
 * maps
 */
static int
x86_modrm_opcode1_present (uint8_t opcode1)
{
        static uint8_t modrm_bitmap1[] = {
                0x0f, /* grp 6,7 LAR, LSL */
                0x00, /* INVD, WBINVD, illegal */

                0xff, /* MOV*PS */
                0x01, /* PREFETCH */

                0xff, /* MOV */
                0xff, /* */

                0x00, /* */
                0x00, /* */

                0xff, /* CMOVcc */
                0xff, /* CMOVcc */

                0xff, /* *PS */
                0xff, /* *PS */

                0xff, /* */
                0xff, /* */
                
                0x7f, /* */
                0xc0, /* MOVD, MOVQ */

                0x00, /* Jcc, Jv */
                0x00, /* Jcc, Jv */

                0x00, /* SETcc */
                0x00, /* SETcc */

                0x38, /* BT, SJLD */
                0xb8, /* BTS, SHRD, IMUL */

                0xff, /* */
                0xfc, /* MOVSX, Bxx, invalid opcode */

                0xff,
                0x00,

                0xfe, /* PSRLx, PADDQ, PMULLW, MOVQ, */
                0xff, /* Pxxx */

                0xff, /* */
                0xff, /* */

                0xfe, /* PSLLx, PMULUDQ, */
                0x7f  /* PSUBx, PADDx */
        };
        uint8_t nbyte = opcode1 / 8;
        uint8_t nbit  = opcode1 % 8;
        uint8_t bitmask = modrm_bitmap1[nbyte];
        if ((bitmask >> nbit) & 0x01) {
                /* modrm byte */
                return 1;
        } else {
                return 0;
        }
}

/* Calculates if modrm is present in 1-byte opcodes.
 * This bitmap table was built from the intel opcode 
 * maps
 */
static int
x86_modrm_opcode0_present (uint8_t opcode0)
{
        static uint8_t modrm_bitmap0[] = {
                0x0f, /* ADD */
                0x0f, /* OR */

                0x0f, /* ADC */
                0x0f, /* SBB */

                0x0f, /* AND */
                0x0f, /* SUB */

                0x0f, /* XOR */
                0x0f, /* CMP */

                0x00, /* INC */
                0x00, /* DEC */

                0x00, /* PUSH */
                0x00, /* POP */

                0x0c, /* BOUND, ARPL */
                0x0a, /* IMUL */

                0x00, /* Jxx */
                0x00, /* Jxx */

                0xff, /* immgrp, TEST, XCHG */
                0xff, /* MOV, LEA, POP */

                0x00, /* XCHG, NOP */
                0x00, /* */

                0x00, /* MOV, CMP */
                0x00, /* */

                0x00, /* MOV */
                0x00, /* MOV */

                0xf3, /* shiftgrp, LES, LDS, grpMOV */
                0x00, /* */

                0x0f, /* shiftgrp */
                0xff, /* ESC */

                0x00, /* LOOPx, JxCXZ, IN, OUT*/
                0x00, /* CALL, JMP, IN, OUT */

                0xc0, /* unarygrp */
                0xc0  /* INC/DEC */
        };
        uint8_t nbyte = opcode0 / 8;
        uint8_t nbit  = opcode0 % 8;
        uint8_t bitmask = modrm_bitmap0[nbyte];
        if ((bitmask >> nbit) & 0x01) {
                /* modrm byte */
                return 1;
        } else {
                return 0;
        }
}

static enum x86_data_type
x86_immediate_opcode0 (uint8_t opcode0)
{
        return X86_DATA_NONE;
}
static enum x86_data_type
x86_immediate_opcode1 (uint8_t opcode1)
{
        return X86_DATA_NONE;
}

static void
x86_opcode_set_state (struct x86_opcode_parser *parser, enum x86_state_e next_state)
{
#ifdef X86_TRACE
        printf ("%s --> %s\n", 
                state_to_string (parser->state), 
                state_to_string (next_state));
#endif /* X86_TRACE */
        parser->state = next_state;
}

static uint8_t
x86_operand_size (struct x86_opcode_parser *parser, enum x86_data_type type)
{
        if (type == X86_DATA_NONE) {
                return 0;
        } else if (type == X86_DATA_BYTE) {
                return 1;
        }
        assert (type == X86_DATA_WORD);
        if (parser->mode == X86_MODE_16) {
                if (parser->prefixes & X86_PREFIX_OP_SIZE) {
                        return 4;
                } else {
                        return 2;
                }
        } else if (parser->mode == X86_MODE_32) {
                if (parser->prefixes & X86_PREFIX_OP_SIZE) {
                        return 2;
                } else {
                        return 4;
                }
        }
        assert (0);
        return 0;
}


void 
x86_opcode_initialize (struct x86_opcode_parser *parser, 
                       enum x86_mode_e mode)
{
        parser->mode = mode;
        parser->state = X86_STATE_PREFIX_OR_OPCODE0;
}

uint32_t
x86_opcode_parse (struct x86_opcode_parser *parser, 
                  uint8_t *buffer, uint32_t size)
{
        uint32_t read = 0;
#if 0
        read = 0;
        while (read < size) {
                uint32_t mod = read % 16;
                if (mod == 0 ) {
                        printf ("\n");
                }
                printf ("%02x ", *buffer);
                read++;
                buffer++;
        }
#endif
        read = 0;
        while (read < size) {
                enum x86_state_e next_state;
                uint8_t byte = *buffer;
                next_state = X86_STATE_ERROR;
                switch (parser->state) {
                case X86_STATE_PREFIX_OR_OPCODE0:
                        if (byte == 0xf0) {
                                parser->prefixes |= X86_PREFIX_LOCK;
                                next_state = X86_STATE_PREFIX_OR_OPCODE0;
                        } else if (byte == 0xf2) {
                                parser->prefixes |= X86_PREFIX_REPNEZ;
                                next_state = X86_STATE_PREFIX_OR_OPCODE0;
                        } else if (byte == 0xf3) {
                                parser->prefixes |= X86_PREFIX_REPEZ;
                                next_state = X86_STATE_PREFIX_OR_OPCODE0;
                        } else if (byte == 0x2e) {
                                parser->prefixes |= X86_PREFIX_CS;
                                next_state = X86_STATE_PREFIX_OR_OPCODE0;
                        } else if (byte == 0x36) {
                                parser->prefixes |= X86_PREFIX_SS;
                                next_state = X86_STATE_PREFIX_OR_OPCODE0;
                        } else if (byte == 0x3e) {
                                parser->prefixes |= X86_PREFIX_DS;
                                next_state = X86_STATE_PREFIX_OR_OPCODE0;
                        } else if (byte == 0x26) {
                                parser->prefixes |= X86_PREFIX_ES;
                                next_state = X86_STATE_PREFIX_OR_OPCODE0;
                        } else if (byte == 0x64) {
                                parser->prefixes |= X86_PREFIX_FS;
                                next_state = X86_STATE_PREFIX_OR_OPCODE0;
                        } else if (byte == 0x65) {
                                parser->prefixes |= X86_PREFIX_GS;
                                next_state = X86_STATE_PREFIX_OR_OPCODE0;
                        } else if (byte == 0x66) {
                                parser->prefixes |= X86_PREFIX_OP_SIZE;
                                next_state = X86_STATE_PREFIX_OR_OPCODE0;
                        } else if (byte == 0x67) {
                                parser->prefixes |= X86_PREFIX_AD_SIZE;
                                next_state = X86_STATE_PREFIX_OR_OPCODE0;
                        } else if (byte == 0x0f) {
                                /* a second opcode byte is coming. */
                                next_state = X86_STATE_OPCODE1;
                        } else {
                                /* first and only opcode byte. */
                                parser->opcode0 = byte;
                                parser->immediate = 
                                        x86_operand_size (parser, 
                                                          x86_immediate_opcode0 (parser->opcode0));
                                if (x86_modrm_opcode0_present (parser->opcode0)) {
                                        next_state = X86_STATE_MODRM;
                                } else {
                                        next_state = X86_STATE_IMMEDIATE;
                                }
                        }
                        break;
                case X86_STATE_OPCODE1:
                        parser->opcode1 = byte;
                        parser->immediate = 
                                x86_operand_size (parser, 
                                                  x86_immediate_opcode1 (parser->opcode1));
                        if (x86_modrm_opcode1_present (parser->opcode1)) {
                                next_state = X86_STATE_MODRM;
                        } else {
                                next_state = X86_STATE_IMMEDIATE;
                        }
                        break;
                case X86_STATE_MODRM: {
                        uint8_t rm = byte & 0x07;
                        uint8_t mod = (byte >> 6) & 0x03;
                        //uint8_t reg = (byte >> 3) & 0x07;
                        enum x86_data_type disp_type;
                        if (parser->mode == X86_MODE_32) {
                                if ((mod == 0 && rm == 5) ||
                                    (mod == 2)) {
                                        disp_type = X86_DATA_WORD;
                                } else if (mod == 1) {
                                        disp_type = X86_DATA_BYTE;
                                } else {
                                        disp_type = X86_DATA_NONE;
                                }
                        } else if (parser->mode == X86_MODE_16) {
                                if ((mod == 0 && rm == 6) ||
                                    (mod == 2)) {
                                        disp_type = X86_DATA_WORD;
                                } else if (mod == 1) {
                                        disp_type = X86_DATA_BYTE;
                                } else {
                                        disp_type = X86_DATA_NONE;
                                }
                        } else {
                                disp_type = X86_DATA_NONE;
                                assert (0);
                        }
                        parser->displacement = x86_operand_size (parser, disp_type);
                        if (parser->mode == X86_MODE_32 &&
                            mod != 3 &&
                            rm == 4) {
                                next_state = X86_STATE_SIB;
                        } else {
                                next_state = X86_STATE_DISPLACEMENT;
                        }
                } break;
                case X86_STATE_SIB:
                        next_state = X86_STATE_DISPLACEMENT;
                        break;
                case X86_STATE_DISPLACEMENT:
                        if (parser->displacement > 0) {
                                parser->displacement--;
                                next_state = X86_STATE_DISPLACEMENT;
                        } else {
                                next_state = X86_STATE_IMMEDIATE;
                        }
                        break;
                case X86_STATE_IMMEDIATE:
                        if (parser->immediate > 0) {
                                parser->immediate--;
                                next_state = X86_STATE_IMMEDIATE;
                        } else {
                                next_state = X86_STATE_PREFIX_OR_OPCODE0;
                        }
                        break;
                case X86_STATE_ERROR:
                        goto out;
                        break;
                }
                x86_opcode_set_state (parser, next_state);
                read++;
                buffer++;
        }
 out:
        return read;
}
