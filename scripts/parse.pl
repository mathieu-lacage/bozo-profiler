#!/usr/bin/env perl

use Storable;

sub build_function_tree
{
    my %nodes = ();

    while (<>) {
	my $regex1 = ".*\/cvs\/([^\/]*)\/.*:(.*)\$";
	my $regex2 = "([^:]*(:[^:]*)*):([^:]*)\$";
	if (/\"([^\"]*)\" -> \"([^\"]*)\"[\t ]*(\[label=[0-9]+\])?;/) {
	    my $a = $1;
	    my $b = $2;
	    my $source;
	    my $source_fn;
	    my $dest;
	    my $dest_fn;
	    if ($a =~ /$regex1/) {
		$source = $1;
		$source_fn = $2;
	    } elsif ($a =~ /$regex2/) {
		$source = $1;
		$source_fn = $3;
	    }
	    if ($b =~ /$regex1/) {
		$dest = $1;
		$dest_fn = $2;
	    } elsif ($b =~ /$regex2/) {
		$dest = $1;
		$dest_fn = $3;
	    }
	    if ($source_fn && $dest_fn) {
		$nodes{$source_fn}{'module'} = $source;
		$nodes{$dest_fn}{'module'} = $dest;
		$nodes{$source_fn}{'calls_to'}{$dest_fn}{'n'}++;
		$nodes{$dest_fn}{'calls_from'}{$source_fn}{'n'}++;
	    } else {
		print;
		print $source_fn . "\n";
		print $dest_fn . "\n";
		$nodes{'unknown'}++;
	    }
	} else {
		print;
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

