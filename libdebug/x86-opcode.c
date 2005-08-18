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

#define nopeX86_TRACE 1

#ifdef X86_TRACE
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
                FOO (DONE);
                FOO (ERROR);
        }
#undef FOO
        assert (0);
        return "0xdeadbeaf";
}
#endif /* X86_TRACE */

enum x86_data_type {
        X86_DATA_NONE,
        X86_DATA_BYTE,
        X86_DATA_WORD,
        X86_DATA_WORD_FIXED,
        X86_DATA_DOUBLEWORD,
        X86_POINTER
};

/********************************************************
 * Calculate whether or not an opcode has a modrm byte.
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
x86_wdw_operand_size (struct x86_opcode_parser *parser)
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

/************************************************************
 *   Calculate the size of the immediate operand.
 */

static int
x86_opcode_find (uint8_t *data, uint8_t size, uint8_t opcode)
{
        uint8_t i;
        for (i = 0; i < size; i++) {
                if (data[i] == opcode) {
                        //printf ("0x%02x -- 0x%02x\n", data[i], opcode);
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
                0xa9, 0xe8, 0xe9, 0xa3, 
                0xa1,
                0xb8, 0xb9, 0xba, 0xbb, 
                0xbc, 0xbd, 0xbe, 0xbf
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
                0x70, 0x71, 0x72, 0x73,
                0x74, 0x75, 0x76, 0x77,
                0x78, 0x79, 0x7a, 0x7b,
                0x7c, 0x7d, 0x7e, 0x7f,
                0xa2, 0xa8,
                0xb0, 0xb1, 0xb2, 0xb3,
                0xb4, 0xb5, 0xb6, 0xb7
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
                return x86_wdw_operand_size (parser);
        } else if (x86_opcode0_is_Ib (opcode0)) {
                return 1;
        } else {
                return 0;
        }
}
static int
x86_opcode1_is_Ib (uint8_t opcode1)
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
static int
x86_opcode1_is_Iv (uint8_t opcode1)
{
        static uint8_t opcode1_has_Iv [] = {
                0x80, 0x81, 0x82, 0x83,
                0x84, 0x85, 0x86, 0x87
        };
        if (x86_opcode_find (opcode1_has_Iv,
                             sizeof (opcode1_has_Iv),
                             opcode1)) {
                return 1;
        } else {
                return 0;
        }
}
static uint8_t
x86_opcode1_immediate_operand_size (struct x86_opcode_parser *parser, uint8_t opcode1)
{
        if (x86_opcode1_is_Ib (opcode1)) {
                return 1;
        } else if (x86_opcode1_is_Iv (opcode1)) {
                return x86_wdw_operand_size (parser);
        } else {
                return 0;
        }
}

static uint8_t
x86_opcode_get_immediate_size (struct x86_opcode_parser *parser)
{
        if (parser->opcode0 == 0x0f) {
                return x86_opcode1_immediate_operand_size (parser, parser->opcode1);
        } else {
                return x86_opcode0_immediate_operand_size (parser, parser->opcode0);
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

static int
x86_opcode_has_sib (struct x86_opcode_parser *parser)
{
        uint8_t rm = parser->modrm & 0x07;
        uint8_t mod = (parser->modrm >> 6) & 0x03;

        if (parser->mode == X86_MODE_32 &&
            mod != 3 &&
            rm == 4) {
                return 1;
        } else {
                return 0;
        }
}

static uint8_t
x86_opcode_get_displacement_size (struct x86_opcode_parser *parser)
{
        uint8_t rm = parser->modrm & 0x07;
        uint8_t mod = (parser->modrm >> 6) & 0x03;
        uint8_t disp_size;
        if (parser->mode == X86_MODE_32) {
                if ((mod == 0 && rm == 5) ||
                    (mod == 2)) {
                        disp_size = x86_wdw_operand_size (parser);
                } else if (mod == 1) {
                        disp_size = 1;
                } else if (x86_opcode_has_sib (parser)) {
                        uint8_t base = parser->sib & 0x7;
                        if (base == 5) {
                                if (mod == 0 || mod == 2) {
                                        disp_size = 4;
                                } else if (mod == 1) {
                                        disp_size = 1;
                                } else {
                                        assert (mod == 3);
                                        disp_size = 0;
                                }
                        } else {
                                disp_size = 0;
                        }
                } else {
                        disp_size = 0;
                }
        } else if (parser->mode == X86_MODE_16) {
                if ((mod == 0 && rm == 6) ||
                    (mod == 2)) {
                        disp_size = x86_wdw_operand_size (parser);
                } else if (mod == 1) {
                        disp_size = 1;
                } else {
                        disp_size = 0;
                }
        } else {
                disp_size = 0;
                assert (0);
        }
        return disp_size;
}

static void 
x86_opcode_reinit (struct x86_opcode_parser *parser)
{
        parser->prefixes = 0;
        parser->state = X86_STATE_PREFIX_OR_OPCODE0;
}

void 
x86_opcode_initialize (struct x86_opcode_parser *parser, 
                       enum x86_mode_e mode)
{
        parser->mode = mode;
        x86_opcode_reinit (parser);
}

int x86_opcode_error (struct x86_opcode_parser *parser)
{
        if (parser->state == X86_STATE_ERROR) {
                return 1;
        } else {
                return 0;
        }
}
int x86_opcode_ok (struct x86_opcode_parser *parser)
{
        if (parser->state == X86_STATE_DONE) {
                return 1;
        } else {
                return 0;
        }
}

uint32_t
x86_opcode_parse (struct x86_opcode_parser *parser, 
                  uint8_t *buffer, uint32_t size)
{
        uint32_t read = 0;
        read = 0;

        if (parser->state == X86_STATE_DONE) {
                x86_opcode_reinit (parser);
        }

        while (read < size &&
               parser->state != X86_STATE_DONE &&
               parser->state != X86_STATE_ERROR) {
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
                                parser->opcode0 = byte;
                                next_state = X86_STATE_OPCODE1;
                        } else {
                                /* first and only opcode byte. */
                                parser->opcode0 = byte;
                                if (x86_opcode0_has_modrm (parser->opcode0)) {
                                        next_state = X86_STATE_MODRM;
                                } else if (x86_opcode_get_immediate_size  (parser) > 0) {
                                        parser->tmp = x86_opcode_get_immediate_size  (parser);
                                        parser->immediate = 0;
                                        next_state = X86_STATE_IMMEDIATE;
                                } else {
                                        next_state = X86_STATE_DONE;
                                }
                        }
                        break;
                case X86_STATE_OPCODE1:
                        parser->opcode1 = byte;
                        if (x86_opcode1_has_modrm (parser->opcode1)) {
                                next_state = X86_STATE_MODRM;
                        } else if (x86_opcode_get_immediate_size (parser) > 0) {
                                parser->tmp = x86_opcode_get_immediate_size (parser);
                                parser->immediate = 0;
                                next_state = X86_STATE_IMMEDIATE;
                        } else {
                                next_state = X86_STATE_DONE;
                        }
                        break;
                case X86_STATE_MODRM: {
                        //uint8_t reg = (byte >> 3) & 0x07;
                        parser->modrm = byte;
                        if (x86_opcode_has_sib (parser)) {
                                next_state = X86_STATE_SIB;
                        } else if (x86_opcode_get_displacement_size (parser) > 0) {
                                parser->displacement = 0;
                                parser->tmp = x86_opcode_get_displacement_size (parser);
                                next_state = X86_STATE_DISPLACEMENT;
                        } else if (x86_opcode_get_immediate_size  (parser) > 0) {
                                parser->tmp = x86_opcode_get_immediate_size  (parser);
                                parser->immediate = 0;
                                next_state = X86_STATE_IMMEDIATE;
                        } else {
                                next_state = X86_STATE_DONE;
                        }
                } break;
                case X86_STATE_SIB:
                        parser->sib = byte;
                        if (x86_opcode_get_displacement_size (parser) > 0) {
                                parser->displacement = 0;
                                parser->tmp = x86_opcode_get_displacement_size (parser);
                                next_state = X86_STATE_DISPLACEMENT;
                        } else if (x86_opcode_get_immediate_size (parser) > 0) {
                                parser->immediate = 0;
                                parser->tmp = x86_opcode_get_immediate_size (parser);
                                next_state = X86_STATE_IMMEDIATE;
                        } else {
                                next_state = X86_STATE_DONE;
                        }
                        break;
                case X86_STATE_DISPLACEMENT:
                        if (parser->tmp > 0) {
                                parser->tmp--;
                                parser->displacement <<= 8;
                                parser->displacement |= byte;
                        }
                        if (parser->tmp > 0) {
                                next_state = X86_STATE_DISPLACEMENT;
                        } else if (x86_opcode_get_immediate_size (parser) > 0) {
                                parser->immediate = 0;
                                parser->tmp = x86_opcode_get_immediate_size (parser);
                                next_state = X86_STATE_IMMEDIATE;
                        } else {
                                next_state = X86_STATE_DONE;
                        }
                        break;
                case X86_STATE_IMMEDIATE:
                        if (parser->tmp > 0) {
                                parser->tmp--;
                                parser->immediate <<= 8;
                                parser->immediate |= byte;
                        }
                        if (parser->tmp > 0) {
                                next_state = X86_STATE_IMMEDIATE;
                        } else {
                                next_state = X86_STATE_DONE;
                        }
                        break;
                case X86_STATE_DONE:
                case X86_STATE_ERROR:
                        assert (0);
                        break;
                }
                x86_opcode_set_state (parser, next_state);
                read++;
                buffer++;
        }
        return read;
}

static char const *
x86_opcode1_to_string (uint8_t opcode1)
{
        static char const *opcode1_to_string [] = {
                "grp 6", "grp 7", "lar", "lsl", 
                "invalid", "invalid", "clts", "invalid",
                "invd", "wbinvd", "invalid", "illegal",
                "invalid", "invalid", "invalid", "invalid",

                "mov", "mov", "mov", "movlp", 
                "unpcklp", "unpckhp", "movhp", "movhp",
                "prefetch", "invalid", "invalid", "invalid",
                "invalid", "invalid", "invalid", "invalid",

                "mov", "mov", "mov", "mov",
                "mov", "invalid", "mov", "invalid",
                "movaps/d", "movaps/d", "cvtxxxxxx", "movntps",
                "cvttxxxxx", "cvtxxxxxx", "ucomiss/d", "comiss/d"

                "wrmsr", "rdtsc", "rdmsr", "rdpmc",
                "sysenter", "sysexit", "invalid", "invalid",
                "invalid", "invalid", "invalid", "invalid", 
                "invalid", "invalid", "invalid", "invalid", 

                "cmovo", "cmovno", "cmovb/c/nae", "cmovae/nb/nc",
                "cmove/z", "cmovne/nz", "cmovbe/na", "cmova/nbe",
                "cmovs", "cmovns", "cmovp/pe", "cmovnp/po",
                "cmovl/nge", "cmovnl/ge", "cmovle/ng", "cmovnle/g"
 
                "movmskp", "sqrt", "rsqrt", "rcp",
                "andp", "andnp", "orp", "xorp",
                "addxx", "mulxx", "cvtxxxx", "cvtxxx",
                "subxx", "minxx", "divxx", "maxxx",

                "punpcklbw", "punpcklwd", "punpckldq", "packsswb",
                "pcmpgtb", "pcmpgtw", "pcmpgtd", "packuswb",
                "punpckhbw", "punpckhwd", "punpckhdq", "packssdw",
                "punpcklqdq", "punpckhqd", "movd", "movxx",

                "pshufxx", "grp 12", "grp 13", "grp 14", 
                "pcmpeqb", "pcmpeqw", "pcmpeqd", "emms",
                "mmx", "mmx", "mmx", "mmx",
                "mmx", "mmx", "movx", "movxx",

                "jo", "jno", "jbc/c/nae", "jae/nb/nc",
                "je/z", "jne/nz", "jbe/na", "ja/nbe",
                "js", "jns", "jp/pe", "jnp/po",
                "jl/nge", "jnl/ge", "jle/ng", "jnle/g",

                "seto", "setno", "setbc/c/nae", "setae/nb/nc",
                "sete/z", "setne/nz", "setbe/na", "seta/nbe",
                "sets", "setns", "setp/pe", "setnp/po",
                "setl/nge", "setnl/ge", "setle/ng", "setnle/g",

                "push %fs", "pop %fs", "cpuid", "bt",
                "shld", "shld", "invalid", "invalid",
                "push %gs", "pop %gs", "rsm", "bts",
                "shrd", "shrd", "grp 15", "imul",

                "cmpxchg", "cmpxchg", "lss", "btr",
                "lfs", "lgs", "movzx", "movzx",
                "invalid", "grp 10", "grp 8", "btc",
                "bsf", "bsr", "movsx", "movsx",

                "xadd", "xadd", "cmpxx", "movnti",
                "pinsrw", "pextrw", "shufpx", "grp 9",
                "bswap %eax", "bswap %ecx", "bswap %edx", "bswap %ebx",
                "bswap %esp", "bswap %ebp", "bswap %esi", "bswap %edi", 

                "invalid", "psrlw", "psrld", "psrlq", 
                "paddq", "pmullq", "movqxxx", "pmovmskb",
                "psubusb", "psubusw", "pminub", "pand",
                "paddusb", "paddusw", "pmaxub", "pandn",

                "pavgb", "psraw", "psrad", "pavgw", 
                "pmulhuw", "pmulhw", "cvtxxxxx", "movntq/dq",
                "psubsb", "psubsw", "pminsw", "por",
                "paddsb", "paddsw", "pmaxsw", "pxor",

                "invalid", "psllw", "pslld", "psllq",
                "pmulludq", "pmaddwd", "psadbw", "maskmovqx",
                "psubb", "psubw", "psubd", "paddb",
                "paddb", "paddw", "paddd", "invalid"
                
        };
        return opcode1_to_string[opcode1];
}

static char const *
x86_opcode0_to_string (struct x86_opcode_parser *parser)
{
        static char const * opcode0_to_string [] = {
                "add", "add", "add", "add", 
                "add", "add", "push %es", "pop %es",
                "or", "or", "or", "or", 
                "or", "or", "push %cs", "error",
                "adc", "adc", "adc", "adc",
                "adc", "adc", "push %ss", "pop %ss",
                "sbb", "sbb", "sbb", "sbb",
                "sbb", "sbb", "push %ds", "pop %ds",
                "and", "and", "and", "and", 
                "and", "and", "seg=%es", "daa",
                "sub", "sub", "sub", "sub", 
                "sub", "sub", "seg=%cs", "das",
                "xor", "xor", "xor", "xor", 
                "xor", "xor", "seg=%ss", "aaa",
                "cmp", "cmp", "cmp", "cmp", 
                "cmp", "cmp", "seg=%ds", "aas",
                "inc", "inc", "inc", "inc", 
                "inc", "inc", "inc", "inc", 
                "dec", "dec", "dec", "dec", 
                "dec", "dec", "dec", "dec", 
                "push", "push", "push", "push", 
                "push", "push", "push", "push", 
                "pop", "pop", "pop", "pop", 
                "pop", "pop", "pop", "pop", 
                "pusha", "popa", "bound", "arpl",
                "seg=%fs", "seg=%gs", "prefix op size", "prefix ad size",
                "push", "imul", "push", "imul",
                "ins", "ins", "outs", "outs",
                "jcc", "jcc", "jcc", "jcc", 
                "jcc", "jcc", "jcc", "jcc", 
                "jcc", "jcc", "jcc", "jcc", 
                "jcc", "jcc", "jcc", "jcc", 
                "grp1", "grp1", "grp1", "grp1",
                "test", "test", "xchg", "xchg",
                "mov", "mov", "mov", "mov", 
                "mov", "lea", "mov", "pop",
                "nop", "xchg", "xchg", "xchg", 
                "xchg", "xchg", "xchg", "xchg", 
                "cbw/cwde", "cwd/cdq", "callf", "fwait/wait",
                "pushf", "popf", "sahf", "lahf",
                "mov", "mov", "mov", "mov", 
                "movs", "movs", "cmps", "cmps", 
                "test", "test", "stos", "stos",
                "lods", "lods", "scas", "scas",
                "mov", "mov", "mov", "mov", 
                "mov", "mov", "mov", "mov", 
                "mov", "mov", "mov", "mov", 
                "mov", "mov", "mov", "mov", 
                "grp2", "grp2", "retn", "retn",
                "les", "lds", "mov", "mov",
                "enter", "leave", "retf", "retf",
                "int3", "int", "into", "iret",
                "grp2", "grp2", "grp2", "grp2",
                "aam", "aad", "invalid", "xlat",
                "escape copro", "escape copro", "escape copro", "escape copro", 
                "escape copro", "escape copro", "escape copro", "escape copro", 
                "loopn", "loop", "loop", "jcxz/jecxz",
                "in", "in", "out", "out", 
                "call", "jmp", "jmp", "jmp",
                "in", "in", "out", "out",
                "prefix lock", "invalid", "prefix repne", "prefix rep",
                "hlt", "cmc", "grp3", "grp3", 
                "clc", "stc", "cli", "sti",
                "cld", "std", "grp4", "grp5"
        };
        static char const *opcode0_grp1_to_string [] = {
                "add", "or", "adc", "sbb",
                "and", "sub", "xor", "cmp"
        };
        static char const *opcode0_grp2_to_string [] = {
                "rol", "ror", "rcl", "rcr",
                "shl/sal", "shr", "invalid", "sar"
        };
        static char const *opcode0_grp3_to_string [] = {
                "test", "invalid", "not", "neg",
                "mul", "imul", "div", "idiv"
        };
        static char const *opcode0_grp4_to_string [] = {
                "inc", "dec", "invalid", "invalid", 
                "invalid", "invalid", "invalid", "invalid", 
        };
        static char const *opcode0_grp5_to_string [] = {
                "inc", "dec", "calln", "callf", 
                "jmpn", "jmpf", "push", "invalid"
        };
        uint8_t opcode0 = parser->opcode0;
        if (opcode0 >= 0x80 && opcode0 <= 0x83) {
                uint8_t index = (parser->modrm >> 3) & 0x7;
                assert (index < sizeof (opcode0_grp1_to_string));
                return opcode0_grp1_to_string[index];
        } else if (opcode0 == 0xc0 || opcode0 == 0xc1 ||
            (opcode0 >= 0xd0 && opcode0 <= 0xd3)) {
                uint8_t index = (parser->modrm >> 3) & 0x7;
                assert (index < sizeof (opcode0_grp2_to_string));
                return opcode0_grp2_to_string[index];
        } else if (opcode0 == 0xf6 || opcode0 == 0xf7) {
                uint8_t index = (parser->modrm >> 3) & 0x7;
                assert (index < sizeof (opcode0_grp3_to_string));
                return opcode0_grp3_to_string[index];
        } else if (opcode0 == 0xfe) {
                uint8_t index = (parser->modrm >> 3) & 0x7;
                assert (index < sizeof (opcode0_grp4_to_string));
                return opcode0_grp4_to_string[index];
        } else if (opcode0 == 0xff) {
                uint8_t index = (parser->modrm >> 3) & 0x7;
                assert (index < sizeof (opcode0_grp5_to_string));
                return opcode0_grp5_to_string[index];
        } else if (opcode0 >= 0xd8 && opcode0 <= 0xdf) {
                /* XXX */
                return "esc";
        } else {
                return opcode0_to_string[opcode0];
        }            
        return "invalid one-byte opcode";
}

void x86_opcode_print (struct x86_opcode_parser *parser)
{
        if (parser->opcode0 == 0x0f) {
                printf ("%s\n", x86_opcode1_to_string (parser->opcode1));
        } else {
                printf ("%s\n", x86_opcode0_to_string (parser));
        }
        //printf ("%s\n", x86_opcode_to_string (parser));
}

#ifdef RUN_SELF_TESTS

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct test {
        struct test *next;
        char *inst;
        uint8_t n_bytes;
        uint8_t array[0];
} *g_tests = NULL;

static void
add_test (char const *inst, uint8_t n, uint8_t c0, ...)
{
        va_list ap;
        uint8_t i;
        struct test *new_test;

        new_test = malloc (sizeof (struct test)+n);
        assert (new_test != NULL);
        new_test->next = g_tests;
        new_test->inst = strdup (inst);
        new_test->n_bytes = n;
        va_start (ap, c0);
        new_test->array[0] = c0;
        for (i = 1; i < n; i++) {
                int arg = va_arg (ap, int);
                new_test->array[i] = (uint8_t)arg & 0xff;
        }
        g_tests = new_test;
}

static void
test_warn (struct test *t, char const *format, ...)
{
        va_list ap;
        uint8_t i;
        printf ("TEST WARNING: \"%s\" ", t->inst);
        for (i = 0; i < t->n_bytes; i++) {
                printf ("%02x ", t->array[i]);
        }
        printf ("\n");
        va_start (ap, format);
        vprintf (format, ap);
        printf ("\n");
}


static int
run_tests (void)
{
        struct x86_opcode_parser parser;
        struct test *tmp;
        int error = 0;

        x86_opcode_initialize (&parser, X86_MODE_32);

        for (tmp = g_tests; tmp != NULL; tmp = tmp->next) {
                uint32_t bytes_read = x86_opcode_parse (&parser, tmp->array, tmp->n_bytes);
                if (bytes_read != tmp->n_bytes) {
                        error = 1;
                        test_warn (tmp, "failed to read enough bytes. Expected: %u, Got: %u", 
                                   tmp->n_bytes, bytes_read);
                }
                if (!x86_opcode_ok (&parser)) {
                        error = 1;
                        test_warn (tmp, "failed to decode.");
                }
        }
        return error;
}


int x86_opcode_run_self_tests (void)
{
        add_test ("test $0x1,%al", 2, 0xa8, 0x01);
        add_test ("jmp *0x804c1b0(,%edx,4)", 7, 0xff, 0x24, 0x95, 0xb0, 0xc1, 0x04, 0x08);
        add_test ("movl $0x804d760,(%esp)", 7, 0xc7, 0x04, 0x24, 0x60, 0xd7, 0x04, 0x08);
        add_test ("je 80498d7 <main+0x367>", 6, 0x0f, 0x84, 0x38, 0x03, 0x00, 0x00);
        add_test ("mov $0x0,%eax", 5, 0xb8, 0x00, 0x00, 0x00, 0x00);
        add_test ("je +3", 2, 0x74, 0x10);
        add_test ("sub $0xc,%esp", 3, 0x83, 0xec, 0x0c);
        add_test ("mov %eax,0x804d234", 5, 0xa3, 0x34, 0xd2, 0x04, 0x08);
        add_test ("je 0x8049509", 2, 0x74, 0x02);
        add_test ("test %eax,%eax", 2, 0x85, 0xc0);
        add_test ("push %edx", 1, 0x52);
        add_test ("add $0x3c3b,%ebx", 6, 0x81, 0xc3, 0x3b, 0x3c, 0x00, 0x00);
        add_test ("push $0x804c264", 5, 0x68, 0x64, 0xc2, 0x04, 0x08);

        if (run_tests ()) {
                printf ("Error while running x86 opcode tests.\n");
                return 1;
        }
        return 0;
}

#endif /* RUN_SELF_TESTS */
