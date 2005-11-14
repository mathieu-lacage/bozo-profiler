#!/usr/bin/env perl

while (<>) {
    if (/([^\t]+)\t((0x[0-9a-f]+ )+)/) {
	print $_;
    }
}
