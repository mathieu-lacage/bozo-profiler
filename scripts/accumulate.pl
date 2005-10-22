#!/usr/bin/env perl

my @data = ();
my $min_pk = 0xffffffff;
my $min_k = 0xffffffff;
my $max_pk = 0;
my $max_k = 0;

while (<>) {
    if (/^([0-9]*) ([0-9\.e\-]*)(.*)$/) {
	my $k = $1;
	my $pk = $2;
	my $comment = $3;
	if ($k > $max_k) {
	    $max_k = $k;
	} elsif ($k < $min_k) {
	    $min_k = $k;
	}
	if ($pk > $max_pk) {
	    $max_pk = $pk;
	} elsif ($pk < $min_pk) {
	    $min_pk = $pk;
	}
	push @data, {"k" => $k, "pk" => $pk, "comment" => $comment};
    }
}

my @sorted = sort { ${$b}{"k"} <=> ${$a}{"k"} } @data;

my $max_pk_accumulated = 0;
foreach $d (@sorted) {
    my $pk = ${$d}{"pk"};
    $max_pk_accumulated += $pk;
}
my $pk_accumulated = 0;
foreach $d (@sorted) {
    my $k = ${$d}{"k"};
    my $pk = ${$d}{"pk"};
    my $comment = ${$d}{"comment"}; 
    $pk_accumulated += $pk;
    print $k. " " . $pk_accumulated . " " . $comment . "\n";
}
