#!/usr/bin/perl

my $file = shift @ARGV;

use Storable;


my %tree = ();
%nodes = %{retrieve ($file)};


my $index = 0;
for $node (keys %nodes) {
    printf ("n %d %s\n", $index, $node);
    $nodes{$node}{'index'} = $index;
    $index++;
}

for $node (keys %nodes) {
    for $other_node (keys %{$nodes{$node}{'calls_to'}}) {
	printf ("e %d %d\n", $nodes{$node}{'index'}, $nodes{$other_node}{'index'});
    }
}

