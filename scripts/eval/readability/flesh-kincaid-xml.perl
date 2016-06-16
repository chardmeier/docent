#!/usr/bin/perl -w


use strict;
use warnings;
#use Getopt::Long "GetOptions";

#use Lingua::EN::Syllable;

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

print STDERR "Running Flesch-Kincaid, language: $LANG\n";

my ($nwords, $nsyllables, $nsents, $ncwords) = (0.0,0,0);

#format of sentences:
#<seg id="1">400 jobs will shifted</seg>


while (<STDIN>) {

    if (/<seg id="\d+"> *(.*) *<\/seg>/) {

	my $text = $1;

	my @words = split(/\s+/, $text);

	foreach my $word (@words) {
	    my @cleanWords = split(/\W+/, $word);
	    foreach my $cword (@cleanWords) {
		$nsyllables += syllables($cword);	    
	    }
	}
	
	$nwords += @words;
	$nsents++;
    }
}

my $ASL = $nwords/$nsents;
my $ASW = $nsyllables/$nwords;
my $FC = 206.835-(1.015*$ASL) -(84.6*$ASW);

print "Sentences: $nsents \nWords: $nwords \nASL: $ASL \nSyllables: $ASW \nFlesh: $FC \n\n";
