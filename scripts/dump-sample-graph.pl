#!/usr/bin/perl


use Storable;

sub build_tree
{
    my %nodes = ();

    while (<>) {
	my $regex1 = ".*\/cvs\/([^\/]*)\/.*:(.*)\$";
	my $regex2 = "([^:]*(:[^:]*)*):([^:]*)\$";
	if (/\"([^\"]*)\" -> \"([^\"]*)/) {
	    my $source = $1;
	    my $dest = $2;
	    if ($source && $dest) {
		$nodes{$source}{'calls_to'}{$dest}{'n'}++;
		$nodes{$dest}{'calls_from'}{$source}{'n'}++;
	    } else {
		print;
		print $source . "\n";
		print $dest . "\n";
		$nodes{'unknown'}++;
	    }
	} else {
	    print;
	    $nodes{'unknown'}++;
	}
    }

    return %nodes;
}


my %tree = build_tree ();
store (\%tree, 'tree.dat');

