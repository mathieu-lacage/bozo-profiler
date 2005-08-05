#!/usr/bin/env perl

use strict;

my $print_input = 0;
my $arg = shift @ARGV;

if ($arg =~ /--print-input/) {
    $print_input = 1;
} elsif ($arg =~ /--print-output/) {
    $print_input = 0;
} else {
    print "options: \n";
    print "\t--help\n";
    print "\t--print-input\n";
    print "\t--print-outputput\n";
    exit (1);
}
my %source_hash = ();
my %dest_hash = ();
my %global_hash = ();

while (<>) {
    /\".*:([^:]*)\" -> \".*:([^:]*)\"[ ;]/;
    my $source_fn = $1;
    my $dest_fn = $2;
    if ($source_fn) {
	$source_hash{$source_fn}++;
	$global_hash{$source_fn} = 1;
    }
    if ($dest_fn) {
	$dest_hash{$dest_fn}++;
	$global_hash{$dest_fn} = 1;
    }
}


my %output_distribution = ();
my $source;

for $source (keys %source_hash) {
    my $n;
    my $i;
    $n = $source_hash{$source};
    $i = $output_distribution{$n};
    $i++;
    $output_distribution{$n} = $i;
}


my %input_distribution = ();
my $dest;

for $dest (keys %dest_hash) {
    my $n;
    my $i;
    $n = $dest_hash{$dest};
    $i = $input_distribution{$n};
    $i++;
    $input_distribution{$n} = $i;
}


my $n_nodes = keys %global_hash;
my $key;

if ($print_input) {
    
    for $key (keys %input_distribution) {
	my $proba;
	$proba = $input_distribution{$key}/$n_nodes;
	print "$key $proba\n";
    }

} else {
    
    for $key (keys %output_distribution) {
	my $proba;
	$proba = $output_distribution{$key}/$n_nodes;
	print "$key $proba\n";
    }
}

