#!/usr/bin/env perl

while (<>) {
    if (/(.*\[[^\]]*)weight=([0-9]*)([^\]]*\].*;)/) {
	$weight = log ($2);
	print "$1weight=$weight$3\n";
    } else {
	print;
    }
}
