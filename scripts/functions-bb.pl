#!/usr/bin/env perl

my $g_line = shift @ARGV;

my @functions = ();
my $state = 0;
my $name;
my $low;
my $high;
while (<>) {
    if ($state == 0 && /.* \(DW_TAG_subprogram\)$/) {
	$state ++;
	$name = "";
	$low = 0;
	$high = 0;
    } elsif ($state == 1 && /DW_AT_name.*: ([^:\n]+)$/) {
	$state ++;
	$name = $1;
    } elsif ($state == 2 && /DW_AT_low_pc.*: (0x)?([0-9a-f]+)/) {
	$state ++;
	$low = hex ($2);
    } elsif ($state == 3 && /DW_AT_high_pc.*: (0x)?([0-9a-f]+)/) {
	$state ++;
	$high = hex ($2);
	my $function_ref = {'name' => $name, 'low' => $low, 'high' => $high};
	push @functions, $function_ref;
    } elsif (/<[0-9]+><[0-9a-f]+>/) {
	$state = 0;
    }
}

my @functions_sorted = sort { ${$a}{'low'} <=> ${$b}{'low'} } @functions;

my @bbs = ();

open (LINE, $g_line);
while (<LINE>) {
    if (/ad: 0x([0-9a-f]+)/) {
	my $bb_start = hex ($1);
if ($bb_start == 0) {
    print "fdfds " . $_;
}
	push (@bbs, $bb_start);
    }
}
close (LINE);

my @bbs_sorted = sort { $a <=> $b } @bbs;


for $function_ref (@functions_sorted) {
    printf ("%s ", $function_ref->{'name'});
    while (@bbs_sorted && $bbs_sorted[0] < $function_ref->{'high'}) {
        my $bb = shift @bbs_sorted;
        printf ("0x%x ", $bb);
    }
    printf ("0x%x\n", $function_ref->{'high'});
}


