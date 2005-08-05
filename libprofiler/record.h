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

#ifndef RECORDER_H
#define RECORDER_H

#include <signal.h>
#include "circular-buffer.h"

struct managed_thread {
        struct circular_buffer_t buffer;
        sig_atomic_t pending_map_change;
};

#define RECORD_TYPE_ENTER 1
#define RECORD_TYPE_LEAVE 2
#define RECORD_TYPE_MAP   3

struct record_t {
        int type;
        void *this_function;
        void *call_site;
};

/* invoked only from the single thread of a new process
 * created with fork. This sets up an environment for
 * the upcoming calls from this thread.
 */
void record_reset (void);

void record_map_change_pending (struct managed_thread *thread);



#endif /* RECORDER_H */
