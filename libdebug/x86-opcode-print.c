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

#include "x86-opcode.h"
#include "x86-opcode-print.h"

#include <stdint.h>
#include <assert.h>
#include <stdio.h>

static char const *
x86_opcode1_to_string (struct x86_opcode_parser *parser)
{
        uint8_t opcode1 = parser->opcode1;
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
        static char const *opcode1_grp6_to_string [] = {
                "sldt", "str", "lldt", "ltr", 
                "verr", "verw", "invalid", "invalid"
        };
        static char const *opcode1_grp7_to_string [] = {
                "sgdt", "sidt", "lgdt", "lidt",
                "smsw", "invalid", "lmsw", "invlpg"
        };
        static char const *opcode1_grp8_to_string [] = {
                "invalid", "invalid", "invalid", "invalid", 
                "bt", "bts", "btr", "btc"
        };
        static char const *opcode1_grp14_to_string [] = {
                "invalid", "invalid", "psrlq", "psrldq", 
                "invalid", "invalid", "psllq", "pslldq"
        };
        static char const *opcode1_grp15_to_string [] = {
                "fxsave", "fxrstor", "ldmxcsr", "stmxcsr",
                "invalid", "lfence", "mfence", "clflush/sfence"
        };
        static char const *opcode1_grp16_to_string [] = {
                "prefetch nta", "prefetch t0", "prefetch t1", "prefetch t2",
                "invalid", "invalid", "invalid", "invalid"
        };
        uint8_t index = (parser->modrm >> 3) & 0x7;
        if (opcode1 == 0) {
                assert (index < sizeof (opcode1_grp6_to_string));
                return opcode1_grp6_to_string[index];
        } else if (opcode1 == 1) {
                assert (index < sizeof (opcode1_grp7_to_string));
                return opcode1_grp7_to_string[index];
        } else if (opcode1 == 0xba) {
                assert (index < sizeof (opcode1_grp8_to_string));
                return opcode1_grp8_to_string[index];
        } else if (opcode1 == 0xc7) {
                if (index == 1) {
                        return "cmpxch8b";
                } else {
                        return "invalid";
                }
        } else if (opcode1 == 0xb9) {
                return "invalid";
        } else if (opcode1 == 0x71) {
                /* grp 12. */
                if (index == 2) {
                        return "psrlw";
                } else if (index == 4) {
                        return "psraw";
                } else if (index == 6) {
                        return "psllw";
                } else {
                        return "invalid";
                }
        } else if (opcode1 == 0x72) {
                /* grp 13 */
                if (index == 2) {
                        return "psrld";
                } else if (index == 4) {
                        return "psrad";
                } else if (index == 6) {
                        return "pslld";
                } else {
                        return "invalid";
                }
        } else if (opcode1 == 0x73) {
                /* grp 14 */
                assert (index < sizeof (opcode1_grp14_to_string));
                return opcode1_grp14_to_string[index];
        } else if (opcode1 == 0xae) {
                /* grp 15 */
                uint8_t mod = (parser->modrm >> 6) & 0x3;
                assert (index < sizeof (opcode1_grp15_to_string));
                if (index == 7) {
                        if (mod) {
                                /* XXX */
                        } else {
                                /* XXX */
                        }
                } else {
                        return opcode1_grp15_to_string[index];
                }
        } else if (opcode1 == 0x18) {
                /* grp 16 */
                assert (index < sizeof (opcode1_grp16_to_string));
                return opcode1_grp16_to_string[index];
        }
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
        static char const *opcode0_esc_d8_0_to_string [] = {
                "fadd", "fmul", "fcom", "fcomp",
                "fsub", "fsubr", "fdiv", "fdivr"
        };
        static char const *opcode0_esc_d9_0_to_string [] = {
                "fld", "invalid", "fst", "fstp",
                "fldenv", "fldcw", "fstenv", "fstcw"
        };
        static char const *opcode0_esc_d9_1_to_string [] = {
                "fld", "fld", "fld", "fld", 
                "fld", "fld", "fld", "fld", 
                "fxch", "fxch", "fxch", "fxch", 
                "fxch", "fxch", "fxch", "fxch", 
                
                "fnop", "invalid", "invalid", "invalid", 
                "invalid", "invalid", "invalid", "invalid", 
                "invalid", "invalid", "invalid", "invalid", 
                "invalid", "invalid", "invalid", "invalid", 

                "fchs", "",
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
        } else if (opcode0 == 0xd8) {
                uint8_t modrm = parser->modrm;
                uint8_t nnn = (modrm >> 3) & 0x7;
                uint8_t h = (modrm >> 4) & 0x0f;
                uint8_t l = modrm & 0x0f;
                if (modrm <= 0xbf) {
                        return opcode0_esc_d8_0_to_string[nnn];
                } else if (h == 0x0c && l <= 0x07) {
                        return "fadd";
                } else if (h == 0x0c && l >= 0x08) {
                        return "fcom";
                } else if (h == 0x0d && l <= 0x07) {
                        return "fsub";
                } else if (h == 0x0d && l >= 0x08) {
                        return "fdiv";
                } else if (h == 0x0e && l <= 0x07) {
                        return "fmul";
                } else if (h == 0x0e && l >= 0x08) {
                        return "fcomp";
                } else if (h == 0x0f && l <= 0x07) {
                        return "fsubr";
                } else if (h == 0x0f && l >= 0x08) {
                        return "fdivr";
                } else {
                        assert (0);
                }
        } else if (opcode0 == 0xd9) {
                uint8_t modrm = parser->modrm;
                uint8_t nnn = (modrm >> 3) & 0x7;
                if (modrm <= 0xbf) {
                        return opcode0_esc_d9_0_to_string[nnn];
                } else if (0) {
                        return opcode0_esc_d9_1_to_string[nnn];
                }
        } else if (opcode0 <= 0xdf) {
                return "esc";
        } else {
                return opcode0_to_string[opcode0];
        }            
        return "invalid one-byte opcode";
}

void x86_opcode_print (struct x86_opcode_parser *parser)
{
        if (parser->opcode0 == 0x0f) {
                printf ("%s\n", x86_opcode1_to_string (parser));
        } else {
                printf ("%s\n", x86_opcode0_to_string (parser));
        }
}
