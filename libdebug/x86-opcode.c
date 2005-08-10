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
        X86_DATA_WORD,
        X86_DATA_WORD_FIXED,
        X86_DATA_DOUBLEWORD,
        X86_POINTER
};

/* Calculates if modrm is present in 2-byte opcodes
 * when the first byte of the 2-byte opcode is 0xff.
 * This bitmap table was built from the intel opcode 
 * maps
 */
static int
x86_opcode1_has_modrm (uint8_t opcode1)
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

                0xff, /* xxxPS */
                0xff, /* *PS */

                0xff, /* */
                0xff, /* */
                
                0x7f, /* */
                0xff, /* MMX, MOVD, MOVQ */

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
x86_opcode0_has_modrm (uint8_t opcode0)
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

static uint8_t
x86_operand_size (struct x86_opcode_parser *parser)
{
        switch (parser->mode) {
        case X86_MODE_64:
                if (parser->prefixes & X86_PREFIX_REX) {
                        return 8;
                } else if (parser->prefixes & X86_PREFIX_OP_SIZE) {
                        return 2;
                } else {
                        return 4;
                }
                break;
        case X86_MODE_32:
                if (parser->prefixes & X86_PREFIX_OP_SIZE) {
                        return 2;
                } else {
                        return 4;
                }
                break;
        case X86_MODE_16:
                if (parser->prefixes & X86_PREFIX_OP_SIZE) {
                        return 4;
                } else {
                        return 2;
                }
                break;
        }
        assert (0);
        return 0;
}



static int
x86_opcode_find (uint8_t *data, uint8_t size, uint8_t opcode)
{
        uint8_t i;
        for (i = 0; i < size; i++) {
                if (data[i] == opcode) {
                        return 1;
                }
        }
        return 0;
}

static int
x86_opcode0_is_Iv (uint8_t opcode0)
{
        static uint8_t opcode0_has_Iv [] = {
                0x05, 0x15, 0x25, 0x35,
                0x0d, 0x1d, 0x2d, 0x3d,
                0x81, 0xc7, 0x68, 0x69,
                0xa9, 0xe8, 0xe9
        };
        return x86_opcode_find (opcode0_has_Iv, 
                                sizeof (opcode0_has_Iv), 
                                opcode0);
}

static int
x86_opcode0_is_Ib (uint8_t opcode0)
{
        uint8_t opcode0_has_Ib [] = {
                0x04, 0x14, 0x24, 0x34,
                0x0c, 0x1c, 0x2c, 0x3c,
                0x80, 0x82, 0x83, 
                0x6a, 0x6b,
                0xc0, 0xc1, 0xc6, 
                0xd4, 0xd5,
                0xa0, 0xc8, 0xcd, 0xeb,
                0xe0, 0xe1, 0xe2, 0xe3,
                0xe4, 0xe5, 0xe6, 0xe7,
                0x78, 0x79, 0x7a, 0x7b,
                0x7c, 0x7d, 0x7e, 0x7f
        };
        return x86_opcode_find (opcode0_has_Ib,
                                sizeof (opcode0_has_Ib),
                                opcode0);
}


static uint8_t
x86_opcode0_immediate_operand_size (struct x86_opcode_parser *parser, uint8_t opcode0)
{
        if (opcode0 == 0x9a ||
            opcode0 == 0xea) {
                /* call Ap or Jmp Ap */
                /* XXX It is not clear to me in which case the size of the operand
                 * is really 48 bits here. For now, I assume 32 bits all the time.
                 */
                return 4;
        } else if (opcode0 == 0xc2 ||
                   opcode0 == 0xca) {
                /* Iw operands */
                return 2;
        } else if (opcode0 == 0xc8) {
                /* Iw,Ib operands */
                return 3;
        } else if (x86_opcode0_is_Iv (opcode0)) {
                return x86_operand_size (parser);
        } else if (x86_opcode0_is_Ib (opcode0)) {
                return 1;
        } else {
                return 0;
        }
}
static uint8_t
x86_opcode1_immediate_operand_size (uint8_t opcode1)
{
        static uint8_t opcode1_has_Ib [] = {
                0x70, 0x71, 0x72, 0x73, 
                0xa4,
                0xc2, 0xc4, 0xc5, 0xc6,
                0xac, 0xba
        };
        if (x86_opcode_find (opcode1_has_Ib,
                             sizeof (opcode1_has_Ib),
                             opcode1)) {
                return 1;
        } else {
                return 0;
        }
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
                                parser->immediate_size = x86_opcode0_immediate_operand_size (parser, parser->opcode0);
                                if (x86_opcode0_has_modrm (parser->opcode0)) {
                                        next_state = X86_STATE_MODRM;
                                } else {
                                        next_state = X86_STATE_IMMEDIATE;
                                }
                        }
                        break;
                case X86_STATE_OPCODE1:
                        parser->opcode1 = byte;
                        parser->immediate_size = x86_opcode1_immediate_operand_size (parser->opcode1);
                        if (x86_opcode1_has_modrm (parser->opcode1)) {
                                next_state = X86_STATE_MODRM;
                        } else {
                                next_state = X86_STATE_IMMEDIATE;
                        }
                        break;
                case X86_STATE_MODRM: {
                        uint8_t rm = byte & 0x07;
                        uint8_t mod = (byte >> 6) & 0x03;
                        //uint8_t reg = (byte >> 3) & 0x07;
                        uint8_t disp_size;
                        if (parser->mode == X86_MODE_32) {
                                if ((mod == 0 && rm == 5) ||
                                    (mod == 2)) {
                                        disp_size = x86_operand_size (parser);
                                } else if (mod == 1) {
                                        disp_size = 1;
                                } else {
                                        disp_size = 0;
                                }
                        } else if (parser->mode == X86_MODE_16) {
                                if ((mod == 0 && rm == 6) ||
                                    (mod == 2)) {
                                        disp_size = x86_operand_size (parser);
                                } else if (mod == 1) {
                                        disp_size = 1;
                                } else {
                                        disp_size = 0;
                                }
                        } else {
                                disp_size = 0;
                                assert (0);
                        }
                        parser->displacement_size = disp_size;
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
                        if (parser->displacement_size > 0) {
                                parser->displacement_size--;
                                next_state = X86_STATE_DISPLACEMENT;
                        } else {
                                next_state = X86_STATE_IMMEDIATE;
                        }
                        break;
                case X86_STATE_IMMEDIATE:
                        if (parser->immediate_size > 0) {
                                parser->immediate_size--;
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
