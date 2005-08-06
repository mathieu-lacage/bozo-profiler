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

  Copyright (C) 2005 Mathieu Lacage
  Author: Mathieu Lacage <mathieu@gnu.org>
*/

#include "timestamp.h"

#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>

double one_run (void)
{
        uint64_t n, i;
        uint64_t start, end;
        struct timeval time_start, time_end;
        struct timezone tmp;
        double v;
        double hw_delta;
        double time_delta;
        double ratio;

        while (gettimeofday (&time_start, &tmp) == -1) {}
        start = timestamp_read ();
        n = 100000;
        v = 1.0;
        for (i = 0; i < n; i++) {
                v *= v;
        }
        end = timestamp_read ();
        while (gettimeofday (&time_end, &tmp) == -1) {}

        hw_delta = end - start;
        time_delta = time_end.tv_usec - time_start.tv_usec;
        ratio = time_delta/hw_delta;
        return ratio;
}

double avg_runs (void)
{
        double total;
        uint8_t i, n;
        total = 0.0;
        n = 10;
        for (i = 0; i < n; i++) {
                total += one_run ();
        }
        return total / n;
}

int 
test_performance (int argc, char *argv[])
{
        printf ("ratio: %g\n", avg_runs ());

        return 0;
}
