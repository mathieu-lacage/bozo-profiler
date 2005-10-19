#!/usr/bin/env perl

my @data = ();

while (<>) {
    if (/^([0-9]*) ([0-9\.e\-]*)(.*)$/) {
	my $k = $1;
	my $pk = $2;
	my $comment = $3;
	push @data, {"k" => $k, "pk" => $pk, "comment" => $comment};
    }
}

my @sorted = sort { ${$b}{"k"} <=> ${$a}{"k"} } @data;

my $pk_accumulated = 0;

foreach $d (@sorted) {
    my $k = ${$d}{"k"};
    my $pk = ${$d}{"pk"};
    my $comment = ${$d}{"comment"}; 
    $pk_accumulated += $pk;
    print $k . " " . $pk_accumulated . " " . $comment . "\n";
}
