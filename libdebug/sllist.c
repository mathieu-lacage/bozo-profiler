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

#include "sllist.h"

#include "null.h"

struct sllist *
sllist_append (struct sllist *start, struct sllist *item)
{
        struct sllist *tmp;
        for (tmp = start; tmp != NULL; tmp = tmp->next) ;
        tmp->next = item;
        item->next = NULL;
        return start;
}

struct sllist *
sllist_insert (struct sllist *location, struct sllist *item)
{
        struct sllist *next;
        next = location->next;
        location->next = item;
        item->next = next;
        return location;
}

struct sllist *
sllist_prepend (struct sllist *start, struct sllist *item)
{
        item->next = start;
        return item;
}

struct sllist *
sllist_search_by_item (struct sllist *start, struct sllist *item)
{
        struct sllist *tmp;
        for (tmp = start; tmp != NULL; tmp = tmp->next) {
                if (tmp == item) {
                        return tmp;
                }
        }
        return NULL;
}
