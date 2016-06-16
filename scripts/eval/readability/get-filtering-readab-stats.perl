#!/usr/bin/perl -w


use strict;
use warnings;
#use Getopt::Long "GetOptions";


binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");


my $min=8;
my $max=100;

my $lixWeight = 1;
my $ovixWeight = 1;
my $pnWeight = 50;

my $num=1;

my @docs;

sub my_sort {
	my $ae = $lixWeight*@{$a}[2] + $ovixWeight*@{$a}[3] + $pnWeight*@{$a}[5];
	my $be = @{$b}[2] + @{$b}[3] + 50*@{$b}[5];
	return $ae <=> $be;
}


#reading pasted readb+readabPOS files, without stats at end
while (<>) {

	s/^ *//;
	
	my @s = split(/\s+/);

	if ($s[0] >= $min && $s[0] <= $max) {
		#doc num, doc length, lix, ovix, ASL, NR
		my @stats = ($num, $s[0], $s[12],$s[14],$s[13],$s[23]);

		push(@docs,\@stats);

		#print "$num $s[0] $s[12] $s[14] $s[13] $s[23]\n";
	}
	
	$num++;
}

my $totSize = 0;
my $doc = 0;
foreach my $stat (sort my_sort @docs) {
	print join(" ", @{$stat});
	$totSize += @{$stat}[1];
	$doc++;
	print " $doc $totSize\n";
}
