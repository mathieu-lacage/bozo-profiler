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
    print "\t--print-output\n";
    exit (1);
}
my %source_hash = ();
my %source_hash_time = ();
my %dest_hash = ();
my %dest_hash_time = ();
my %global_hash = ();
my $line = 0;

while (<>) {
    /\".*:([^:]*)\" -> \".*:([^:]*)\"[ ;]/;
    my $source_fn = $1;
    my $dest_fn = $2;
    if ($source_fn) {
	$source_hash{$source_fn}++;
	$source_hash_time{$source_fn}+=$line;
	$global_hash{$source_fn} = 1;
    }
    if ($dest_fn) {
	$dest_hash{$dest_fn}++;
	$dest_hash_time{$dest_fn}+=$line;
	$global_hash{$dest_fn} = 1;
    }
    $line++;
}


my %output_distribution_time = ();
my %output_distribution = ();
my $source;

for $source (keys %source_hash) {
    my $n;
    my $i;
    $n = $source_hash{$source};
    $i = $output_distribution{$n};
    $i++;
    $output_distribution{$n} = $i;
    $output_distribution_time{$n} += $source_hash_time{$source};
}


my %input_distribution_time = ();
my %input_distribution = ();
my $dest;

for $dest (keys %dest_hash) {
    my $n;
    my $i;
    $n = $dest_hash{$dest};
    $i = $input_distribution{$n};
    $i++;
    $input_distribution{$n} = $i;
    $input_distribution_time{$n} += $dest_hash_time{$dest};
}


my $n_nodes = keys %global_hash;
my $key;

if ($print_input) {
    
    for $key (keys %input_distribution) {
	my $proba;
	my $n = $input_distribution{$key};
	$proba = $n/$n_nodes;
	my $time_avg = $input_distribution_time{$key} / ($key * $n);
	print "$key $proba $time_avg\n";
    }

} else {
    
    for $key (keys %output_distribution) {
	my $proba;
	my $n = $output_distribution{$key};
	$proba = $n/$n_nodes;
	my $time_avg = $output_distribution_time{$key} / ($key * $n);
	print "$key $proba $time_avg\n";
    }
}

