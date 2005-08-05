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

#ifndef SLLIST_H
#define SLLIST_H

struct sllist {
  struct sllist *next;
};

struct sllist * sllist_append         (struct sllist *start,    struct sllist *item);
struct sllist * sllist_prepend        (struct sllist *start,    struct sllist *item);
struct sllist * sllist_insert         (struct sllist *location, struct sllist *item);
struct sllist * sllist_search_by_item (struct sllist *start,    struct sllist *item);

#endif /* SLINKED_LIST_H */
