#!/usr/bin/env perl


%source_hash = {};

while (<>) {
    /\"(.*)\" -> \"(.*)\"[ ;]/;
    $source = $1;
    $dest = $2;
    if ($source =~ s/(.*):.*/$1/ && $dest =~ s/(.*):.*/$1/) {
	if ($source !~ /$dest/) {
	    $n = $source_hash{$source}{$dest};
	    $n++;
	    $source_hash{$source}{$dest} = $n;
	}
    }
}

print "digraph G {\n";

for $source (keys %source_hash) {
    for $dest (keys %{$source_hash{$source}}) {
	print "\"$source\" -> \"$dest\" [weight=$source_hash{$source}{$dest}]\n";
    }
}


print "}\n";


