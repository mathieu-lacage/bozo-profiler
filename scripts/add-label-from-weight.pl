#!/usr/bin/env perl

while (<>) {
    if (/(.*\[[^\]]*)weight=([0-9]*)([^\]]*\].*;)/) {
	$weight = $2;
	print "$1weight=$weight, label=\"$weight\"$3\n";
    } else {
	print;
    }
}
