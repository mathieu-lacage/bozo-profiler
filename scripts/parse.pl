#!/usr/bin/env perl

use Storable;

sub build_function_tree
{
    %nodes = ();

    while (<>) {
	/.*\/cvs\/([^\/]*)\/.*:(.*)\" -> .*\/cvs\/([^\/]*)\/.*:(.*)\"/;
	my $source = $1;
	my $source_fn = $2;
	my $dest = $3;
	my $dest_fn = $4;
	if ($source_fn && $dest_fn) {
	    $nodes{$source_fn}{'module'} = $source;
	    $nodes{$dest_fn}{'module'} = $dest;
	    $nodes{$source_fn}{'calls_to'}{$dest_fn}{'n'}++;
	    $nodes{$dest_fn}{'calls_from'}{$source_fn}{'n'}++;
	} else {
	    $nodes{'unknown'}++;
	}
    }

    return %nodes;
}

sub build_module_tree
{
    %nodes = ();

    while (<>) {
	if (/.*\/cvs\/([^\/]*)\/.*:(.*)\" -> .*\/cvs\/([^\/]*)\/.*:(.*)\"/) {
	    my $source = $1;
	    my $source_fn = $2;
	    my $dest = $3;
	    my $dest_fn = $4;
	    if ($source_ && $dest) {
		$nodes{$source}{'calls_to'}{$dest}{'n'}++;
		$nodes{$dest}{'calls_from'}{$source}{'n'}++;
	    } elsif ($source) {
		$nodes{$source}{'calls_to'}{'unknown'}{'n'}++;
	    } elsif ($dest) {
		$nodes{$dest}{'calls_from'}{'unknown'}{'n'}++;
	    }
	}
    }

    return %nodes;
}

sub build_sub_module_tree
{
    $name = shift @_;
    %nodes = ();

    while (<>) {
	if (/.*\/cvs\/([^\/]*)\/.*:(.*)\" -> .*\/cvs\/([^\/]*)\/.*:(.*)\"/) {
	    my $source = $1;
	    my $source_fn = $2;
	    my $dest = $3;
	    my $dest_fn = $4;
	    if ($source eq $name && $dest eq $name) {
		$nodes{$source_fn}{'module'} = $source;
		$nodes{$dest_fn}{'module'} = $dest;
		$nodes{$source_fn}{'calls_to'}{$dest_fn}{'n'}++;
		$nodes{$dest_fn}{'calls_from'}{$source_fn}{'n'}++;
	    }
	}
    }

    return %nodes;
}



while (scalar @ARGV) {
    my $arg = shift @ARGV;

    if ($arg =~ /--build-functions/) {
	my %function_tree = build_function_tree ();
	store (\%function_tree, 'function.dat');
    } elsif ($arg =~ /--build-modules/) {
	my %module_tree = build_module_tree ();
	store (\%module_tree, 'module.dat');
    } elsif ($arg =~ /--build-submodules/) {
	$module = shift @ARGV;
	my %module_tree = build_sub_module_tree ($module);
	store (\%module_tree, 'submodule.dat');
    } else {
	print "options: \n";
	print "\t--help\n";
	print "\t--build-functions\n";
	print "\t--build-modules\n";
	print "\t--build-submodules\n";
	exit (0);
    }
}

