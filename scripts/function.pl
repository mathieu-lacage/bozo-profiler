#!/usr/bin/env perl

my %functions = ();
my $state = 0;
while (<>) {
    if ($state == 0 && /.* \(DW_TAG_subprogram\)$/) {
	$state ++;
	$name = "";
	$low = 0;
	$high = 0;
    } elsif ($state == 1 && /DW_AT_name.*: ([^:]+)$/) {
	$state ++;
	$name = $1;
    } elsif ($state == 2 && /DW_AT_low_pc.*: (0x)?([0-9a-f]+)/) {
	$state ++;
	$low = hex ($2);
    } elsif ($state == 3 && /DW_AT_high_pc.*: (0x)?([0-9a-f]+)/) {
	$state ++;
	$high = hex ($2);
	$functions{$name} = $high - $low;
    } elsif (/<[0-9]+><[0-9a-f]+>/) {
	$state = 0;
    }
}

for $function (keys %functions) {
    print $functions{$function} . "\n";
}
