#!/usr/bin/env perl

my @data = ();

while (<>) {
    /([0-9]*) ([0-9\.e\-]*)/;
    my $k = $1;
    my $pk = $2;
    push @data, {"k" => $k, "pk" => $pk};
}

my @sorted = sort { ${$b}{"k"} <=> ${$a}{"k"} } @data;

my $pk_accumulated = 0;

foreach $d (@sorted) {
    my $k = ${$d}{"k"};
    my $pk = ${$d}{"pk"};
    $pk_accumulated += $pk;
    print $k . " " . $pk_accumulated . "\n";
}
