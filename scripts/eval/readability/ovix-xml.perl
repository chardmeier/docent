#!/usr/bin/perl -w


use strict;
use warnings;
#use Getopt::Long "GetOptions";


binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");

my ($nTokens, $nTypes) = (0.0,0);

#format of sentences:
#<seg id="1">400 jobs will shifted</seg>

my %types;

while (<>) {

    if (/<seg id="\d+"> *(.*) *<\/seg>/) {

	my $text = $1;
	my @words = split(/\s+/, $text);

	foreach my $word (@words) {
	    $types{$word}++;
	}
    
	$nTokens += @words;

    }
}

$nTypes = keys(%types);

my $tt = $nTypes/$nTokens;
my $ovix = (log($nTokens)/(2-(log($nTypes)/log($nTokens))));


print "Number of tokens: $nTokens\nNumber of types: $nTypes\nType/token ratio: $tt\nOVIX: $ovix\n\n";
