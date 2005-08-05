#!/usr/bin/env perl

$file = shift @ARGV;
$png_file = $file . ".png";

$script  = "";
$script .= "f(x) = x ** (-a)\n";
$script .= "fit f(x) \"" . $file . "\" via a\n";
$script .= "set logscale xy\n";
$script .= "set terminal png\n";
$script .= "set output \"" . $png_file . "\"\n";
$script .= "set label \"a=%g\",a at graph 0.8, graph 0.9\n";
$script .= "plot f(x), \"" . $file . "\"\n";

system ("echo '$script' | gnuplot 2>/dev/null");
