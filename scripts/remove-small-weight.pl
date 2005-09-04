#!/usr/bin/env perl

$min_weight = shift @ARGV;

while (<>) {
    if (/(.*\[[^\]]*)weight=([0-9]*)([^\]]*\].*;)/) {
	$weight = $2;
	if ($weight >= $min_weight) {
	    print;
	} else {
	    print "$1weight=$weight, constraint=false$3\n";
	}
    } else {
	print;
    }
}
