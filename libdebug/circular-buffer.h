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

  Copyright (C) 2004,2005 Mathieu Lacage
  Author: Mathieu Lacage <mathieu@gnu.org>
*/

#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <stdint.h>

#define CIRCULAR_BUFFER_SIZE (1<<19)

struct circular_buffer_t {
        uint32_t readIndex;
        uint32_t writeIndex;
        uint8_t data[CIRCULAR_BUFFER_SIZE];
};

void circular_buffer_initialize (struct circular_buffer_t *buffer);

/* return number of bytes which can be used to perform a single write */
uint32_t circular_buffer_get_write_size (struct circular_buffer_t *buffer);
/* record in the buffer that size bytes have been written to */
void circular_buffer_commit_write (struct circular_buffer_t *buffer, uint32_t size);
/* return a pointer to the current write buffer. You can write up to
 * n bytes in this buffer where n is the value returned by get_write_size ()
 */
uint8_t *circular_buffer_peek_write_ptr (struct circular_buffer_t *buffer);



/* return number of bytes which can be used to perform a single read */
uint32_t circular_buffer_get_read_size (struct circular_buffer_t *buffer);
/* record in the buffer that size bytes have been read from */
void circular_buffer_commit_read (struct circular_buffer_t *buffer, uint32_t size);
/* return a pointer to the current read buffer. You can read up to n
 * bytes in this buffer where n is the value returned by get_read_size ()
 */
uint8_t *circular_buffer_peek_read_ptr (struct circular_buffer_t *buffer);



void circular_buffer_run_test (void);

#endif /* CIRCULAR_BUFFER_H */
