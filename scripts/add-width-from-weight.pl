#!/usr/bin/env perl

while (<>) {
    if (/(.*\[[^\]]*)weight=([0-9]*)([^\]]*\].*;)/) {
	$weight = $2;
	$width = log ($weight);
	print "$1weight=$weight, style=\"setlinewidth($width)\"$3\n";
    } else {
	print;
    }
}
