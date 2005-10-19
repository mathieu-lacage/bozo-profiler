#!/usr/bin/env perl

my %data = ();

while (<>) {
    @array = split ;
    for $element (@array) {
	if ($element =~ /^([0-9]*)/) {
	    my $k = $1;
	    my $comment = $2;
	    $data{$k}++;
	}
    }
}

foreach $k (keys %data) {
    print $k . " " . $data{$k} . "\n";
}
