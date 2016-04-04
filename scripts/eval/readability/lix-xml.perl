#!/usr/bin/perl -w


use strict;
use warnings;
#use Getopt::Long "GetOptions";


binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");

my ($nwords, $nlongWords, $nXlongWords, $nsents) = (0.0,0,0,0);

#format of sentences:
#<seg id="1">400 jobs will shifted</seg>


while (<>) {

    if (/<seg id="\d+"> *(.*) *<\/seg>/) {

	my $text = $1;
	my @words = split(/\s+/, $text);

	foreach my $word (@words) {
	    $nlongWords++ if length($word) > 6;
	    $nXlongWords++ if length($word) > 14;
	}
    
	$nwords += @words;
	$nsents++;
    }
}

my $ASL = $nwords/$nsents;
my $perLongWords = 100 * $nlongWords/$nwords;
my $perXLongWords = 100 * $nXlongWords/$nwords;
my $LIX = $ASL + $perLongWords;

print "Sentences: $nsents \nWords: $nwords \nLong words: $nlongWords\nExtra long words: $nXlongWords\nlw ratio: $perLongWords \nxlw ratio: $perXLongWords \nASL: $ASL \nLIX: $LIX \n\n"
