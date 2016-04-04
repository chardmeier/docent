#!/usr/bin/perl -w


use strict;
use warnings;
#use Getopt::Long "GetOptions";


my %pos = (
'pm' => 0.0, 'nn' => 0.0, 'pp' => 0.0, 'pc' => 0.0, 'vb' => 0.0, 'ab' => 0.0, 'ps' => 0.0, 'pn' => 0.0
);



my $tokens = 0.0;
#johan	pm.nom	johan	

while (<STDIN>) {
	if (/<>/) {

		printf "%5d %5d %5d %5d %5d %5d %5d %5d", $tokens, $pos{'pm'}, $pos{'nn'}, $pos{'pp'}, $pos{'pc'}, $pos{'pn'}, $pos{'ab'}, $pos{'vb'};

		my $NR = ($pos{'nn'}+$pos{'pp'}+$pos{'pc'})/($pos{'pn'}+$pos{'ab'}+$pos{'vb'}+0.1);
		my $PM = $pos{'pm'}/$tokens;
		
		printf " %9.5f %9.5f\n", $NR, $PM;

		%pos = (
			pm => 0.0, nn => 0.0, pp => 0.0, pc => 0.0, vb => 0.0, ab => 0.0, pn => 0.0
			);
		$tokens = 0.0;
	}
	else {  #tagged line
		
		s/ps/pn/g;
		if (/\s(pm|nn|pp|pc|vb|ab|pn)[\.\s]/) {
			$pos{$1}++;
		}
		
		$tokens++;
	}
}
