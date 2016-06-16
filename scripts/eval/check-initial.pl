#!/usr/bin/perl
#-*-perl-*-


my $doc=0;
my %initial=();
my $violations = 0;
my $total=0;
my $sents=0;

while(<>){
    if (/\<\/doc\>/){
	$doc++;
	my ($high) = sort { $b <=> $a } values %initial;
	my ($char) = sort { $initial{$b} <=> $initial{$a} } keys %initial;
	print "document $doc: highest freq of initial char = $high ($char)\n";
	$violations += $sents-$high;
	%initial=();
	$sents=0;
    }
    next unless (/\<seg/);
    s/<.*?>//g;
    if (/^(.)/){
	$initial{$1}++;
    }
    $sents++;
    $total++;
}

print "total number of violations = $violations ($total)\n";
