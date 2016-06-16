#!/usr/bin/perl -w


use strict;
use warnings;
#use Getopt::Long "GetOptions";


binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");

my ($nwords, $nlongWords, $nsents) = (0.0,0,0);

while (<>) {
    chomp;

    my @words = split(/\s+/);

    foreach my $word (@words) {
	$nlongWords++ if length($word) > 6;
    }
    
    $nwords += @words;
    $nsents++;
}

my $ASL = $nwords/$nsents;
my $perLongWords = 100 * $nlongWords/$nwords;
my $LIX = $ASL + $perLongWords;

print "Sentences: $nsents \nWords: $nwords \nASL: $ASL \nLong words: $perLongWords \nLIX: $LIX \n\n"
