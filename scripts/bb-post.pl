#!/usr/bin/env perl

my $g_n_bb = 0;
my $g_bb_size = 0;

while (scalar @ARGV) {
    my $arg = shift @ARGV;

    if ($arg =~ /--print-n-bb-per-function/) {
	$g_n_bb = 1;
    } elsif ($arg =~ /--print-size-per-bb/) {
	$g_bb_size = 1;
    }
}

while (<>) {
    if (/([^0]+)((0x[0-9a-f]+ ?)+)$/) {
	my @bbs = split (/ /, $2);
	if ($g_n_bb) {
	    printf ("%u\n", (scalar @bbs) - 1);
	}
	if ($g_bb_size) {
	    my $first = 1;
	    my $prev;
	    for $bb (@bbs) {
		if ($first) {
		    $first = 0;
		    $prev = hex ($bb);
		} else {
		    my $current = hex($bb);
		    printf ("%u\n", $current - $prev);
		    $prev = $current;
		}
	    }
	}
    }
}
