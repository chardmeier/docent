#!/usr/bin/perl -w


use strict;
use warnings;
#use Getopt::Long "GetOptions";

#use Lingua::EN::Syllable;

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");

# # for Se/De
# sub syllables {
#     my $w = shift;
#     #print " *$w* ";
#     #$w =~ s/[^aoueiy]+$//;
#     $w =~ s/^[^aoueiy]+//;
#     my @vowelGroups = split(/[^aoueiy]+/,$w);
#     my $syll = scalar(@vowelGroups);
#     #$syll-- if $syll>1 && $w =~ /e$/; #remove silent 'e' at end of words - not applicable!
#     #print " $syll *$w*  *@consGroups*\n";
#     return $syll;
# }

sub syllables {
    my $w = shift;
    #print " *$w* ";
    #$w =~ s/[^aoueiy]+$//;
    $w =~ s/^[^aoueiy]+//;
    my @vowelGroups = split(/[^aoueiy]+/,$w);
    my $syll = scalar(@vowelGroups);
    $syll-- if $syll>1 && $w =~ /e$/; #remove silent 'e' at end of words
    #print " $syll *$w*  *@consGroups*\n";
    return $syll;
}

my ($nwords, $nsyllables, $nsents, $ncwords) = (0.0,0,0);

while (<>) {
    chomp;

    my @words = split(/\s+/);

    foreach my $word (@words) {
	my @cleanWords = split(/\W+/, $word);
	foreach my $cword (@cleanWords) {
	    $nsyllables += syllables($cword);	    
	}
    }
    
    $nwords += @words;
    $nsents++;
}

my $ASL = $nwords/$nsents;
my $ASW = $nsyllables/$nwords;
my $FC = 206.835-(1.015*$ASL) -(84.6*$ASW);

print "Sentences: $nsents \nWords: $nwords \nASL: $ASL \nSyllables: $ASW \nFlesh: $FC \n\n";
