#!/usr/bin/perl -w


use strict;
use warnings;
#use Getopt::Long "GetOptions";


my %pos = (
pm => 0.0, nn => 0.0, pp => 0.0, pc => 0.0, vb => 0.0, ab => 0.0, ps => 0.0, pn => 0.0
    );



my $tokens = 0.0;
#johan	pm.nom	johan	

while (<STDIN>) {
    s/ps/pn/g;
    if (/\s(pm|nn|pp|pc|vb|ab|ps|pn)[\.\s]/) {
	$pos{$1}++;
    }
    $tokens++;
}



my $NR = (1.0*($pos{"nn"}+$pos{"pp"}+$pos{"pc"}))/($pos{"pn"}+$pos{"ab"}+$pos{"vb"});
my $PM = $pos{"pm"}/$tokens;

print "Tokens: $tokens\nPM: $pos{'pm'}, NN:$pos{'nn'}, PP: $pos{'pp'}, PC: $pos{'pc'}, PN: $pos{'pn'}, AB: $pos{'ab'}, VB: $pos{'vb'}\n\n";
printf "NR: %14.3f\nPM: %14.3f\n\n", $NR, $PM;

