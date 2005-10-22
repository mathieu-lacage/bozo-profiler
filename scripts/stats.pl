#!/usr/bin/env perl

use Storable;

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


sub print_stats
{
    my %nodes = %{shift @_};
    my $n_missed = shift @_;
    my $n_single_edges = 0;
    my $n_weighted_edges = 0;
    my $n_leaf = 0;

    for $node_source (keys %nodes) {
	if ((scalar keys %{$nodes{$node_source}{'calls_to'}}) == 0) {
	    $n_leaf++;
	}
	for $node_dest (keys %{$nodes{$node_source}{'calls_to'}}) {
	    $n_weighted_edges++;
	    $n_single_edges += $nodes{$node_source}{'calls_to'}{$node_dest}{'n'};
	}
    }

    my $n_nodes = scalar (keys %nodes);
    print "n nodes: " . $n_nodes . "\n";
    print "n weighted edges: $n_weighted_edges\n";
    print "n single edges: $n_single_edges\n";
    print "n missed: $n_missed\n";
    print "n leaf/n nodes: " . $n_leaf / $n_nodes . "\n";
}


my %g_nodes = ();
my $g_n_missed = 0;
my $g_input = 'foo.dat';
my $g_print_input = 0;
my $g_print_input_dup = 0;
my $g_print_output = 0;
my $g_print_output_dup = 0;
my $g_print_sum = 0;

while (scalar @ARGV) {
    my $arg = shift @ARGV;

    if ($arg =~ /--input/) {
	$g_input = shift @ARGV;
    } elsif ($arg =~ /--print-input$/) {
	$g_print_input = 1;
    } elsif ($arg =~ /--print-input-dup/) {
	$g_print_input_dup = 1;
    } elsif ($arg =~ /--print-output-dup/) {
	$g_print_output_dup = 1;
    } elsif ($arg =~ /--print-output$/) {
	$g_print_output = 1;
    } elsif ($arg =~ /--print-both/) {
	$g_print_output = 1;
	$g_print_input = 1;
    } elsif ($arg =~ /--print-sum/) {
	$g_print_sum = 1;
    } else {
	print "options: \n";
	print "\t--help\n";
	print "\t--input [file]\n";
	print "\t--print-input\n";
	print "\t--print-input-dup\n";
	print "\t--print-output\n";
	print "\t--print-output-dup\n";
	print "\t--print-both\n";
	print "\t--print-sum\n";
	print "\t--print-none\n";
	exit (0);
    }
}


%g_nodes = %{retrieve ($g_input)};
$g_n_missed = $g_nodes{'unknown'};

print_stats (\%g_nodes, $g_n_missed);

if ($g_print_input) {
    print "input\n";
    my %distribution = ();
    %distribution = rank_distribution (\%g_nodes, 'calls_from');
    #print_distribution (\%distribution);
    for $n (keys %distribution) {
	my $node;
	my @nodes = @{$distribution{$n}};
	print $n . " " . scalar @nodes . " ";
	my $output_avg = 0;
	for $node (@nodes) {
	    my $output = scalar keys %{$g_nodes{$node}{'calls_to'}};
	    $output_avg += $output;
	    printf ("%d ", $output);
	}
	$output_avg /= scalar @nodes;
	#print "$output_avg \n";
	print "\n";
    }
}
if ($g_print_input_dup) {
    print "input dup\n";
    my %distribution_dup = ();
    %distribution_dup = rank_distribution_dup (\%g_nodes, 'calls_from');
    print_distribution (\%distribution_dup);
}
if ($g_print_output) {
    print "output\n"; 
    my %distribution = (); 
    %distribution = rank_distribution (\%g_nodes, 'calls_to');
    #print_distribution (\%distribution);
    for $n (keys %distribution) {
	my $node;
	my @nodes = @{$distribution{$n}};
	my $n_nodes = scalar @nodes;
	print $n . " " . $n_nodes . " ";
	my $input_avg = 0;
	for $node (@nodes) {
	    my $input = scalar keys %{$g_nodes{$node}{'calls_from'}};
	    $input_avg += $input;
	    printf ("%d ", $input);
	}
	$input_avg /= $n_nodes;
	#print " $input_avg \n";
	print "\n";
    }
}
if ($g_print_output_dup) {
    print "output dup\n";
    my %distribution_dup = ();
    %distribution_dup = rank_distribution_dup (\%g_nodes, 'calls_to');
    print_distribution (\%distribution_dup);
}
if ($g_print_sum) {
    my %distribution_sum = ();
    my $n_small = 0;
    for $node (keys %g_nodes) {
	my $n_output = scalar keys %{$g_nodes{$node}{'calls_to'}};
	my $n_input = scalar keys %{$g_nodes{$node}{'calls_from'}};
	my $both = $n_input - $n_output;
	push @{$distribution_sum{$both}}, $node;
    }
    #printf ("oo: %d\n", $n_small);
    print_distribution (\%distribution_sum);
}
