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
my $g_input = 'foo.dat';
my $g_print_input = 0;
my $g_print_input_dup = 0;
my $g_print_output = 0;
my $g_print_output_dup = 0;

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
    } else {
	print "options: \n";
	print "\t--help\n";
	print "\t--input [file]\n";
	print "\t--print-input\n";
	print "\t--print-input-dup\n";
	print "\t--print-output\n";
	print "\t--print-output-dup\n";
	print "\t--print-both\n";
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
    print_distribution (\%distribution);
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
    print_distribution (\%distribution);
}
if ($g_print_output_dup) {
    print "output dup\n";
    my %distribution_dup = ();
    %distribution_dup = rank_distribution_dup (\%g_nodes, 'calls_to');
    print_distribution (\%distribution_dup);
}
