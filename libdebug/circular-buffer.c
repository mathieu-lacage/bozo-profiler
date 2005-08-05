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


/**
 * This file implements a pretty classic circular buffer.
 * I use the classic trick of using read and write indexes
 * which vary in the [0..2n] interval rather than the real
 * bounds of the array ([0..n]). This trick is used to 
 * disambiguate the empty and full buffer conditions without
 * wasting a slot in the buffer.
 *
 * I am a bit sick of having to implement this thing over
 * over again.
 */

#include "circular-buffer.h"
#include "null.h"

void
circular_buffer_initialize (struct circular_buffer_t *buffer)
{
        buffer->readIndex = 0;
        buffer->writeIndex = 0;
}

uint32_t 
circular_buffer_get_write_size (struct circular_buffer_t *buffer)
{
        uint32_t left;
        uint32_t readIndex  = buffer->readIndex % CIRCULAR_BUFFER_SIZE;
        uint32_t writeIndex = buffer->writeIndex % CIRCULAR_BUFFER_SIZE;

        if (buffer->readIndex == buffer->writeIndex) {
                left = CIRCULAR_BUFFER_SIZE - writeIndex;
        } else if (readIndex == writeIndex) {
                left = 0;
        } else if (readIndex > writeIndex) {
                left = readIndex - writeIndex;
        } else {
                left = CIRCULAR_BUFFER_SIZE - writeIndex;
        }
        return left;
}

uint8_t *
circular_buffer_peek_write_ptr (struct circular_buffer_t *buffer)
{
        uint8_t *data;
        data = &(buffer->data[buffer->writeIndex % CIRCULAR_BUFFER_SIZE]);
        return data;
}
void
circular_buffer_commit_write (struct circular_buffer_t *buffer, uint32_t size)
{
        if (circular_buffer_get_write_size (buffer) < size) {
                return;
        }

        buffer->writeIndex += size;
        buffer->writeIndex %= CIRCULAR_BUFFER_SIZE * 2;
}

uint32_t
circular_buffer_get_read_size (struct circular_buffer_t *buffer)
{
        uint32_t left;
        uint32_t readIndex  = buffer->readIndex % CIRCULAR_BUFFER_SIZE;
        uint32_t writeIndex = buffer->writeIndex % CIRCULAR_BUFFER_SIZE;
        
        if (buffer->readIndex == buffer->writeIndex) {
                left = 0;
        } else if (readIndex == writeIndex) {
                left = CIRCULAR_BUFFER_SIZE - readIndex;
        } else if (readIndex < writeIndex) {
                left = writeIndex - readIndex;
        } else {
                left = CIRCULAR_BUFFER_SIZE - readIndex;
        }
        return left;
}

uint8_t *
circular_buffer_peek_read_ptr (struct circular_buffer_t *buffer)
{
        uint8_t *data;
        data = &(buffer->data[buffer->readIndex % CIRCULAR_BUFFER_SIZE]);
        return data;
}
void
circular_buffer_commit_read (struct circular_buffer_t *buffer, uint32_t size)
{
        if (circular_buffer_get_read_size (buffer) < size) {
                return;
        }

        buffer->readIndex += size;
        buffer->readIndex %= CIRCULAR_BUFFER_SIZE * 2;
}

#ifdef RUN_SELF_TESTS

#include <stdlib.h>
#include <stdio.h>

#define TEST_READ_SIZE(buffer,expected) \
if (circular_buffer_get_read_size (buffer) != expected) { \
        printf ("(l%d) -- circular_buffer: read size is %d. Should be %d\n", \
                __LINE__, \
                circular_buffer_get_read_size (buffer), \
                expected); \
}

#define TEST_WRITE_SIZE(buffer,expected) \
if (circular_buffer_get_write_size (buffer) != expected) { \
        printf ("(l%d) -- circular_buffer: write size is %d. Should be %d\n", \
                __LINE__, \
                circular_buffer_get_write_size (buffer), \
                expected); \
}


void
circular_buffer_run_test (void)
{
        struct circular_buffer_t *buffer;
        buffer = (struct circular_buffer_t *)malloc (sizeof (*buffer));
        if (buffer == NULL) {
                return;
        }

        circular_buffer_initialize (buffer);
        TEST_READ_SIZE (buffer, 0);
        circular_buffer_commit_write (buffer, 1);
        TEST_READ_SIZE (buffer, 1);
        circular_buffer_commit_write (buffer, 1);
        TEST_READ_SIZE (buffer, 2);
        circular_buffer_commit_write (buffer, 10);
        TEST_READ_SIZE (buffer, 12);
        circular_buffer_commit_write (buffer, CIRCULAR_BUFFER_SIZE-13);
        TEST_READ_SIZE (buffer, CIRCULAR_BUFFER_SIZE-1);
        TEST_WRITE_SIZE (buffer, 1);
        circular_buffer_commit_write (buffer, 1);
        TEST_READ_SIZE (buffer, CIRCULAR_BUFFER_SIZE);
        TEST_WRITE_SIZE (buffer, 0);
        circular_buffer_commit_write (buffer, 1);
        TEST_READ_SIZE (buffer, CIRCULAR_BUFFER_SIZE);
        TEST_WRITE_SIZE (buffer, 0);

        circular_buffer_commit_read (buffer, 1);
        TEST_WRITE_SIZE (buffer, 1);
        TEST_READ_SIZE (buffer, CIRCULAR_BUFFER_SIZE-1);
        circular_buffer_commit_read (buffer, 1);
        TEST_WRITE_SIZE (buffer, 2);
        TEST_READ_SIZE (buffer, CIRCULAR_BUFFER_SIZE-2);
        circular_buffer_commit_write (buffer, 1);
        TEST_WRITE_SIZE (buffer, 1);
        TEST_READ_SIZE (buffer, CIRCULAR_BUFFER_SIZE-2);
        circular_buffer_commit_write (buffer, 1);
        TEST_WRITE_SIZE (buffer, 0);
        TEST_READ_SIZE (buffer, CIRCULAR_BUFFER_SIZE-2);
        circular_buffer_commit_write (buffer, 1);
        TEST_WRITE_SIZE (buffer, 0);
        TEST_READ_SIZE (buffer, CIRCULAR_BUFFER_SIZE-2);
        circular_buffer_commit_read (buffer, 1);
        TEST_WRITE_SIZE (buffer, 1);
        TEST_READ_SIZE (buffer, CIRCULAR_BUFFER_SIZE-3);
        circular_buffer_commit_read (buffer, 1);
        TEST_WRITE_SIZE (buffer, 2);
        TEST_READ_SIZE (buffer, CIRCULAR_BUFFER_SIZE-4);
        circular_buffer_commit_write (buffer, 1);
        TEST_WRITE_SIZE (buffer, 1);
        TEST_READ_SIZE (buffer, CIRCULAR_BUFFER_SIZE-4);
        circular_buffer_commit_write (buffer, 1);
        TEST_WRITE_SIZE (buffer, 0);
        TEST_READ_SIZE (buffer, CIRCULAR_BUFFER_SIZE-4);


        circular_buffer_initialize (buffer);
        TEST_READ_SIZE (buffer, 0);
        TEST_WRITE_SIZE (buffer, CIRCULAR_BUFFER_SIZE);
        circular_buffer_commit_write (buffer, CIRCULAR_BUFFER_SIZE);
        TEST_READ_SIZE (buffer, CIRCULAR_BUFFER_SIZE);
        TEST_WRITE_SIZE (buffer, 0);
        circular_buffer_commit_read (buffer, CIRCULAR_BUFFER_SIZE);
        TEST_READ_SIZE (buffer, 0);
        TEST_WRITE_SIZE (buffer, CIRCULAR_BUFFER_SIZE);
        circular_buffer_commit_write (buffer, CIRCULAR_BUFFER_SIZE);
        TEST_READ_SIZE (buffer, CIRCULAR_BUFFER_SIZE);
        TEST_WRITE_SIZE (buffer, 0);
        circular_buffer_commit_read (buffer, CIRCULAR_BUFFER_SIZE);
        TEST_READ_SIZE (buffer, 0);
        TEST_WRITE_SIZE (buffer, CIRCULAR_BUFFER_SIZE);


        free (buffer);
}

#endif

