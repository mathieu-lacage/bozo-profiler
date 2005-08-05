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

#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>


struct nptl_thread {
        union {
                void *__padding[16];
        };
        struct {
                void *next; 
                void *prev;
        } list;
        pid_t tid;
        pid_t pid;
};

int 
pthread_utils_to_id (pthread_t pth)
{
        struct nptl_thread *pd;
        pd = (struct nptl_thread *)pth;
        return pd->tid;
}
