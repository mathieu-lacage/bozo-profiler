#!/usr/bin/perl

my $file = shift @ARGV;

use Storable;


my %tree = ();
%nodes = %{retrieve ($file)};


my $index = 0;
for $node (keys %nodes) {
    #printf ("n %d %s\n", $index, $node);
    $nodes{$node}{'index'} = $index;
    $index++;
}

for $node (keys %nodes) {
    for $other_node (keys %{$nodes{$node}{'calls_to'}}) {
	if ($nodes{$node}{'index'} != $nodes{$other_node}{'index'}) {
	    printf ("%d %d 1\n", $nodes{$node}{'index'}, $nodes{$other_node}{'index'});
	}
    }
}

