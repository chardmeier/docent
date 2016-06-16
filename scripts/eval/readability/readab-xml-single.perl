#!/usr/bin/perl -w


use strict;
use warnings;
#use Getopt::Long "GetOptions";


binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");



my $LANG = "en";

sub syllables {
    my $w = shift;
    $w =~ s/^[^aoueiy]+//;
    my @vowelGroups = split(/[^aoueiy]+/,$w);
    my $syll = scalar(@vowelGroups);
    $syll-- if $LANG eq "en" && $syll>1 && $w =~ /[^aouiy]e$/; #remove silent 'e' at end of English words
    return $syll;
}

$LANG = $ARGV[0] if @ARGV >=1;

print STDERR "Calculating various readability metrics, language: $LANG\n\n";


my ($nTokens, $nTypes) = (0.0,0);
my ($nsyllables, $nsents) = (0.0,0);
my ($nlongWords, $nXlongWords) = (0.0,0);

#format of sentences:
#<seg id="1">400 jobs will shifted</seg>

my %types;

while (<STDIN>) {

    if (/<seg id="\d+"> *(.*) *<\/seg>/) {

	my $text = $1;
	my @words = split(/\s+/, $text);

	foreach my $word (@words) {
	    $nlongWords++ if length($word) > 6;
	    $nXlongWords++ if length($word) >= 14;
	    $types{$word}++;
	    my @cleanWords = split(/\W+/, $word); #to account for inter-word punctuation like hyphens
	    foreach my $cword (@cleanWords) {
		$nsyllables += syllables($cword);	    
	    }
	}
    
	$nTokens += @words;
	$nsents++;
    }
}

$nTypes = keys(%types);



my $ASL = $nTokens/$nsents;
my $ASW = $nsyllables/$nTokens;
my $FC = 206.835-(1.015*$ASL) -(84.6*$ASW);

my $perLongWords = 100 * $nlongWords/$nTokens;
my $perXLongWords = 100 * $nXlongWords/$nTokens;
my $LIX = $ASL + $perLongWords;

my $TT = $nTypes/$nTokens;
my $OVIX = log($nTokens)/(log(2.0-(log($nTypes)/log($nTokens))));


print "ABSOLUTE NUMBERS:\nSentences: $nsents \nTokens: $nTokens \nTypes: $nTypes\nSyllables: $nsyllables\nLong words: $nlongWords\nExtra long words: $nXlongWords\n\n";

printf "AUXILLIARY MEASURES:\nlw ratio: %9.3f\nxlw ratio: %8.3f\nASL: %14.3f\nASW: %14.3f\n\n", $perLongWords, $perXLongWords, $ASL, $ASW;

printf "METRICS:\nLIX: %14.3f\nFlesh: %12.3f\nTT ratio: %9.3f\nOVIX: %13.3f\n\n", $LIX, $FC, $TT, $OVIX;

