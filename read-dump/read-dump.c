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

  Copyright (C) 2004,2005  INRIA
  Author: Mathieu Lacage <lacage@sophia.inria.fr>
*/

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib/gprintf.h>

#include "symbol.h"
#include "outside-map.h"
#include "mbool.h"
#include "record.h"
#include "load-map.h"
#include "elf32-parser.h"
#include "memory-reader.h"
#include "dwarf2-parser.h"
#include "utils.h"
#include "dwarf2-cache.h"

struct Args {
        enum {
                OUTPUT_DOT,
                OUTPUT_ALL_DOT,
                OUTPUT_MAT
        } output_type;
        mbool is_valid;
};


static int 
read_data (int fd, uint8_t *data, uint32_t size)
{
        uint32_t left = size;
        while (left > 0) {
                ssize_t bytes_read;
                bytes_read = read (fd, data, left);
                if (bytes_read == 0) {
                        goto error;
                }
                left -= bytes_read;
                data += bytes_read;
        }
        return size;
 error:
        return 0;
}

static int 
print_symbol_cb (struct elf32_symbol const *symbol, 
                 uint64_t address,
                 char const *real_name, 
                 void *context)
{
        printf ("\"%s\"", real_name);
        return 1;
}

struct output_formatting {
        struct Args const *args;
        struct load_map *map;
        void (*enter) (struct output_formatting *out, uint64_t call_address, uint64_t this_address);
        void (*leave) (struct output_formatting *out, uint64_t call_address, uint64_t this_address);
        void (*start) (struct output_formatting *out);
        void (*end) (struct output_formatting *out);
};

static 
void mat_enter (struct output_formatting *out, 
                uint64_t call_address, 
                uint64_t this_address)
{
        printf ("enter ");
        symbol_iterate_names (out->map, this_address, print_symbol_cb, NULL);
        printf (" from ");
        symbol_iterate_names (out->map, call_address, print_symbol_cb, NULL);        
        printf ("\n");
}

static 
void mat_leave (struct output_formatting *out, 
                uint64_t call_address, 
                uint64_t this_address)
{
        printf ("leave ");
        symbol_iterate_names (out->map, this_address, print_symbol_cb, NULL);
        printf (" from ");
        symbol_iterate_names (out->map, call_address, print_symbol_cb, NULL);
        printf ("\n");
}

static void 
mat_start (struct output_formatting *out)
{}

static void 
mat_end (struct output_formatting *out)
{}

static void
mat_init (struct output_formatting *out)
{
        out->enter = mat_enter;
        out->leave = mat_leave;
        out->start = mat_start;
        out->end   = mat_end;
}

static int
resolve_address_to_symbol (struct load_map *map, uint64_t address, 
                           struct dwarf2_symbol_information *symbol,
                           struct memory_reader *reader)
{
        struct memory_reader abbrev_reader;
        if (!utils_goto_map_entry (map, address)) {
                goto error;
        }

        address -= map->get_base_address (map);

        if (utils_filename_to_memory_reader (map->get_filename (map), 
                                             reader) == -1) {
                goto error;
        }
        if (utils_filename_to_memory_reader (map->get_filename (map), 
                                             &abbrev_reader) == -1) {
                goto error;
        }
        if (dwarf2_lookup (address, symbol,
                           READER (&abbrev_reader),
                           READER (reader)) == -1) {
                goto error;
        }
        
        return 0;
 error:
        return -1;
}

#if 0
static void
print_string (struct reader *reader, uint32_t offset)
{
        uint8_t b;
        reader->seek (reader, offset);
        do {
                b = reader->read_u8 (reader);
                if (b != 0) {
                        printf ("%c", b);
                }
        } while (b != 0);
}
static void
print_symbol (struct output_formatting *out,
              struct dwarf2_symbol_information *symbol,
              struct reader *reader)
{
        printf ("\"");
        if (out->args->output_type == OUTPUT_ALL_DOT) {
                if (symbol->valid_fields & DWARF2_SYMBOL_COMP_DIRNAME_OFFSET) {
                        print_string (READER (reader), symbol->comp_dirname_offset);
                        printf(":");
                }
                if (symbol->valid_fields & DWARF2_SYMBOL_DIRNAME_OFFSET) {
                        print_string (READER (reader), symbol->dirname_offset);
                        printf(":");
                }
        }
        print_string (READER (reader), symbol->filename_offset);
        printf(":");
        print_string (READER (reader), symbol->name_offset);
        printf ("\"");
}
#endif
static char *
symbol_to_string (struct output_formatting *out,
                  struct dwarf2_symbol_information *symbol,
                  struct memory_reader *reader)
{
        char *retval;
        char *comp_dirname_str;
        char *dirname_str;
        char *filename_str;
        char *name_str;
        if (symbol->valid_fields & DWARF2_SYMBOL_FILENAME_OFFSET) {
                filename_str = reader->buffer + symbol->filename_offset;
        } else {
                filename_str = NULL;
        }
        if (symbol->valid_fields & DWARF2_SYMBOL_NAME_OFFSET) {
                name_str = reader->buffer + symbol->name_offset;
        } else {
                name_str = NULL;
        }
        if (symbol->valid_fields & DWARF2_SYMBOL_COMP_DIRNAME_OFFSET) {
                comp_dirname_str = reader->buffer + symbol->comp_dirname_offset;
        } else {
                comp_dirname_str = NULL;
        }
        if (symbol->valid_fields & DWARF2_SYMBOL_DIRNAME_OFFSET) {
                dirname_str = reader->buffer + symbol->dirname_offset;
        } else {
                dirname_str = NULL;
        }
        
        if (out->args->output_type == OUTPUT_ALL_DOT) {
                if (comp_dirname_str != NULL && dirname_str != NULL) {
                        retval = g_strdup_printf ("\"%s:%s:%s:%s\"", comp_dirname_str, dirname_str, filename_str, name_str);
                } else if (comp_dirname_str != NULL) {
                        retval = g_strdup_printf ("\"%s:%s:%s\"", comp_dirname_str, filename_str, name_str);
                } else if (dirname_str != NULL) {
                        retval = g_strdup_printf ("\"%s:%s:%s\"", dirname_str, filename_str, name_str);
                } else {
                        retval = 0;
                        g_assert (FALSE);
                }
        } else {
                retval = g_strdup_printf ("\"%s:%s\"", filename_str, name_str);
        }
        return retval;
}

static char *
elf32_symbol_to_string (struct output_formatting *out,
                        struct load_map *map, uint64_t address)
{
        struct elf32_header elf32;
        struct elf32_symbol_iterator i;
        struct memory_reader reader;

        if (!utils_goto_map_entry (map, address)) {
                goto error;
        }
        address -= map->get_base_address (map);
        
        if (utils_get_iterator_for_filename (map->get_filename (map),
                                             &i, &elf32, &reader) == -1) {
                goto error1;
        }
        while (elf32_symbol_iterator_has_next (&i)) {
                if (address >= i.symbol.st_value &&
                    address < i.symbol.st_value + i.symbol.st_size) {
                        char const *real_name;
                        char *retval;
                        if (reader.reader.status < 0) {
                                goto error1;
                        }
                        real_name = reader.buffer + i.name_offset;
                        retval = g_strdup_printf ("\"%s:%s\"", map->get_filename (map), real_name);
                        return retval;
                }
                elf32_symbol_iterator_next (&i, READER (&reader));
        }

 error1:
        return g_strdup_printf ("\"%s:0x%llx\"", map->get_filename (map), address);
 error:
        return strdup ("unknown");
}


static void 
dot_enter (struct output_formatting *out, 
           uint64_t call_address, 
           uint64_t this_address)
{
        struct memory_reader reader;
        char *symbol_name;
        uint64_t call_line, this_line;
        /* the call address is actually the address to which the 
         * called function must go back. Of course, this is not 
         * the address of the code which called the function: it
         * is the address of the next instruction. The line
         * number of the next instruction might thus not be
         * the line number of the call instruction so we
         * decrease the call_address a small bit to make sure
         * we calculate the address of the calling function.
         */
        call_address--;
        if (dwarf2_cache_lookup (call_address, &symbol_name, &call_line) == -1) {
                struct dwarf2_symbol_information symbol;
                if (resolve_address_to_symbol (out->map, call_address, 
                                               &symbol, &reader) == -1) {
                        symbol_name = elf32_symbol_to_string (out, out->map, call_address);
                        call_line = -1;
                } else {
                        symbol_name = symbol_to_string (out, &symbol, &reader);
                        if (symbol.valid_fields & DWARF2_SYMBOL_LINE) {
                                call_line = symbol.line;
                        } else {
                                call_line = -1;
                        }
                }
                dwarf2_cache_add (call_address, symbol_name, call_line);
        }
        printf (symbol_name);
        
        printf (" -> ");
        if (dwarf2_cache_lookup (this_address, &symbol_name, &this_line) == -1) {
                struct dwarf2_symbol_information symbol;
                if (resolve_address_to_symbol (out->map, this_address, 
                                               &symbol, &reader) == -1) {
                        symbol_name = elf32_symbol_to_string (out, out->map, this_address);
                } else {
                        symbol_name = symbol_to_string (out, &symbol, &reader);
                        if (symbol.valid_fields & DWARF2_SYMBOL_LINE) {
                                this_line = symbol.line;
                        } else {
                                this_line = -1;
                        }
                }
                dwarf2_cache_add (this_address, symbol_name, call_line);
        }
        printf (symbol_name);
        if (call_line != -1) {
                printf (" [label=%llu]", call_line);
        }
        printf (";\n");
}

static void 
dot_leave (struct output_formatting *out, 
           uint64_t call_address, 
           uint64_t this_address)
{}

static void 
dot_start (struct output_formatting *out)
{
        dwarf2_cache_initialize ();
        printf ("digraph G {\n");
}

static void 
dot_end (struct output_formatting *out)
{
        printf ("}\n");
}


static void
dot_init (struct output_formatting *out)
{
        out->enter = dot_enter;
        out->leave = dot_leave;
        out->start = dot_start;
        out->end   = dot_end;
}

#include <sys/time.h>
static struct timeval start = {0,};
static void
log_start (void)
{
        struct timezone tmp;
        while (gettimeofday (&start, &tmp) == -1) {}
}
static void
log_enter_record (void)
{
        static int n = 0;
        static int np = 0;

        if (n == 0) {
                struct timeval current;
                struct timezone tmp;
                long long delta;
                double rate;
                while (gettimeofday (&current, &tmp) == -1) {}
                delta = current.tv_sec-start.tv_sec;
                if (delta < 10) {
                        delta *= 1000000;
                        delta += current.tv_usec-start.tv_usec;
                        delta /= 1000000;
                }
                rate = 1000.0*np/delta;
                fprintf (stderr, "%d  %g resolutions/s\n", np, rate);
                np++;
        }
        n++;
        n %= 1000;

}


static void 
dump_file (int fd, struct Args const *args)
{
        uint32_t bytes_read;
        void *call_site;
        void *this_function;
        uint64_t call_address;
        uint64_t this_address;
        void *base_address;
        char buffer[1024];
        int i, type, map_len, str_len;
        struct load_map map;
        struct output_formatting out;

        outside_map_initialize (&map);

        out.args = args;
        if (args->output_type == OUTPUT_DOT ||
            args->output_type == OUTPUT_ALL_DOT) {
                dot_init (&out);
        } else if (args->output_type == OUTPUT_MAT) {
                mat_init (&out);
        }

        out.start (&out);

        log_start ();

        while (TRUE) {
                bytes_read = read_data (fd, (uint8_t *)&type, sizeof (type));
                if (bytes_read == 0) {
                        break;
                }
                switch (type) {
                case RECORD_TYPE_ENTER:
                        log_enter_record ();
                        bytes_read = read_data (fd, (uint8_t *)&this_function, sizeof (this_function));
                        if (bytes_read == 0) {
                                printf ("error reading enter record\n");
                                goto out;
                        }
                        bytes_read = read_data (fd, (uint8_t *)&call_site, sizeof (call_site));
                        if (bytes_read == 0) {
                                printf ("error reading enter record 2\n");
                                goto out;
                        }
                        call_address = (uint32_t) call_site;
                        this_address = (uint32_t) this_function;
                        out.enter (&out, call_address, this_address);
                        break;
                case RECORD_TYPE_LEAVE:
                        bytes_read = read_data (fd, (uint8_t *)&this_function, sizeof (this_function));
                        if (bytes_read == 0) {
                                printf ("error reading leave record\n");
                                goto out;
                        }
                        bytes_read = read_data (fd, (uint8_t *)&call_site, sizeof (call_site));
                        if (bytes_read == 0) {
                                printf ("error reading leave record 2\n");
                                goto out;
                        }
                        call_address = (uint32_t) call_site;
                        this_address = (uint32_t) this_function;
                        out.leave (&out, call_address, this_address);
                        break;
                case RECORD_TYPE_MAP:
                        dwarf2_cache_flush ();
                        outside_map_reset (&map);
                        bytes_read = read_data (fd, (uint8_t *)&map_len, sizeof (map_len));
                        if (bytes_read == 0) {
                                printf ("error reading map record\n");
                                goto out;
                        }
                        for (i = 0; i < map_len; i++) {
                                bytes_read = read_data (fd, (uint8_t *)&base_address, sizeof (base_address));
                                if (bytes_read == 0) {
                                        printf ("error reading map record 2\n");
                                        goto out;
                                }
                                bytes_read = read_data (fd, (uint8_t *)&str_len, sizeof (str_len));
                                if (bytes_read == 0) {
                                        printf ("error reading map record 3\n"); 
                                        goto out;
                                }
                                if (str_len > 1024) {
                                        printf ("string too long\n");
                                        goto out;
                                }
                                bytes_read = read_data (fd, (uint8_t *)buffer, str_len);
                                if (bytes_read == 0) {
                                        printf ("error reading map record 4\n");
                                        goto out;
                                }
                                outside_map_append (&map, (uint32_t)base_address, buffer);
                                //printf ("map %p %s\n", base_address, buffer);
                        }
                        out.map = &map;
                        break;
                }
        }

        out.end (&out);

 out:
        return;
}

static void
output_use (char const *name)
{
        printf ("Usage: \"%s\" [options]\n", name);
        printf ("\t--output-dot: output on stdout the flow graph in dot format (graphviz)\n");
        printf ("\t--output-all-dot: output on stdout the flow graph in dot format (graphviz) with extra information\n");
        printf ("\t--output-mat: output on stdout the flow graph in an adhoc format\n");
        printf ("\t--help: output on stdout this help\n");
        printf ("This program consumes a dump file from its standard intput and\n"
                "outputs a human-readable version on its standard output.\n");
}

static void
parse_args (int argc, char *argv[], struct Args *args)
{
        char *program_name;
        program_name = argv[0];
        args->is_valid = TRUE;
        args->output_type = OUTPUT_MAT;
        argv++;
        while (*argv != 0) {
                if (strcmp (*argv, "--output-dot") == 0) {
                        args->output_type = OUTPUT_DOT;
                } else if (strcmp (*argv, "--output-all-dot") == 0) {
                        args->output_type = OUTPUT_ALL_DOT;
                } else if (strcmp (*argv, "--output-mat") == 0) {
                        args->output_type = OUTPUT_MAT;
                } else if (strcmp (*argv, "--help") == 0) {
                        output_use (program_name);
                } else {
                        args->is_valid = FALSE;
                }
                argv++;
        }
}

int main (int argc, char *argv[])
{
        struct Args args;
        parse_args (argc, argv, &args);

        if (args.is_valid) {
                /* read dump data from stdin. */
                dump_file (0, &args);
        }

        return 0;
}
