#!/usr/bin/env perl

$graph_started = 0;

$root = shift @ARGV;

while (<>) {
    if (/([^{]*){(.*)/) {
	print "$1\{\{rank=source;\"$root\";\}$2\n";
    } else {
	print;
    }
}
