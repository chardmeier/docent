#!/usr/bin/perl
#-*-perl-*-


my $doc=0;
my %initial=();
# my $violations = 0;
my $total=0;
my $sents=0;

while(<>){
    if (/\<\/doc\>/){
	$doc++;
	my ($high) = sort { $b <=> $a } values %initial;
	my ($char) = sort { $initial{$b} <=> $initial{$a} } keys %initial;
# 	$violations += $sents-$high;
	my $entropy = 0;
	foreach my $f (values %initial){
	    $entropy -= $f/$total * log($f/$total) / log(2);
	}
	print "document $doc: highest freq of initial char = $high ($char), entropy = $entropy\n";
	%initial=();
	$sents=0;
    }
    next unless (/\<seg/);
    s/<.*?>//g;
    my @words = split(/\s+/);
    foreach my $w (@words){
	if ($w=~/^([a-zA-Z])/){
	    $initial{$1}++;
	}
	$sents++;
	$total++;
    }
}

# print "total number of violations = $violations ($total)\n";
