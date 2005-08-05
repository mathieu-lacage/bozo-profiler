#!/usr/bin/env perl

sub rank_distribution_dup
{
    my %nodes = %{shift @_};
    my $call = shift @_;
    my %distribution = ();
    my $node;

    for $node (keys %nodes) {
	my $n_dup = 0;
	my $other_node;
	for $other_node (keys %{$nodes{$node}{$call}}) {
	    $n_dup += $nodes{$node}{$call}{$other_node}{'n'};
	}
	push @{$distribution{$n_dup}}, $node;
    }
    return %distribution;
}

sub rank_distribution
{
    my %nodes = %{shift @_};
    my $call = shift @_;
    my %distribution = ();
    my $node;

    for $node (keys %nodes) {
	my $n_single = scalar keys %{$nodes{$node}{$call}};
	push @{$distribution{$n_single}}, $node;
    }

    return %distribution;
}

sub print_distribution
{
    my %distribution = %{shift @_};
    my $n;

    for $n (keys %distribution) {
	my $node;
	my @nodes = @{$distribution{$n}};
	print $n . " " . scalar @nodes . " ";
	for $node (@nodes) {
	    print $node . " ";
	}
	print "\n";
    }
}

sub print_nodes
{
    my %nodes = %{shift @_};

    for $key (keys %nodes) {
	print $key . " ";
    }
    print "\n";    
}

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

sub print_stats
{
    my %nodes = %{shift @_};
    my $n_missed = shift @_;
    my $n_single_edges = 0;
    my $n_weighted_edges = 0;

    for $node_source (keys %nodes) {
	for $node_dest (keys %{$nodes{$node_source}{'calls_to'}}) {
	    $n_weighted_edges++;
	    $n_single_edges += $nodes{$node_source}{'calls_to'}{$node_dest}{'n'};
	}
    }

    print "n nodes: " . scalar (keys %nodes) . "\n";
    print "n weighted edges: $n_weighted_edges\n";
    print "n single edges: $n_single_edges\n";
    print "n missed: $n_missed\n";
}


my %g_nodes = ();
my $g_n_missed = 0;
my $print_input = 0;
my $print_input_dup = 0;
my $print_output = 0;
my $print_output_dup = 0;

while (scalar @ARGV) {
    my $arg = shift @ARGV;

    if ($arg =~ /--print-input-dup/) {
	$print_input_dup = 1;
	next;
    }
    if ($arg =~ /--print-input[^-]*/) {
	$print_input = 1;
	next;
    }
    if ($arg =~ /--print-output-dup/) {
	$print_output_dup = 1;
	next;
    }
    if ($arg =~ /--print-output[^-]/) {
	$print_output = 1;
	next;
    }
    if ($arg =~ /--print-both/) {
	$print_output = 1;
	$print_input = 1;
	next;
    }
    if ($arg =~ /--help/) {
	print "options: \n";
	print "\t--help\n";
	print "\t--print-input\n";
	print "\t--print-input-dup\n";
	print "\t--print-output\n";
	print "\t--print-output-dup\n";
	print "\t--print-both\n";
	print "\t--print-none\n";
	next;
    }

    unshift @ARGV, $arg;
    last;
}


%g_nodes = build_function_tree ();
#%g_nodes = build_module_tree ();
#%g_nodes = build_sub_module_tree ('nautilus');
$g_n_missed = $g_nodes{'unknown'};

print_stats (\%g_nodes, $g_n_missed);

if ($print_input) {
    print "input\n";
    my %distribution = ();
    %distribution = rank_distribution (\%g_nodes, 'calls_from');
    print_distribution (\%distribution);
}
if ($print_input_dup) {
    print "input dup\n";
    my %distribution_dup = ();
    %distribution_dup = rank_distribution_dup (\%g_nodes, 'calls_from');
    print_distribution (\%distribution_dup);
}
if ($print_output) {
    print "output\n"; 
    my %distribution = (); 
    %distribution = rank_distribution (\%g_nodes, 'calls_to');
    print_distribution (\%distribution);
}
if ($print_output_dup) {
    print "output dup\n";
    my %distribution_dup = ();
    %distribution_dup = rank_distribution_dup (\%g_nodes, 'calls_to');
    print_distribution (\%distribution_dup);
}
