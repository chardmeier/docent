#!/usr/bin/perl -w

use strict;
use warnings;


## Converts a number of Docent files, names given on stdin, into a Moses style nbest file, written on stdout

die ("You have to give at least 1 Docent translation file\nUsage: docent2mosesNbest docentConfigFile docentTransFile1 docentTransFile2 ... docentTransFileN") if @ARGV < 2;

my @modelStrings;

open(CFILE, shift);
my $index = -1;
my $mIndex = 0;
my $lastModel = "";
while (<CFILE>) {
     if (/<weight model="([^"]+)/) {
		if ($1 eq $lastModel) {
			push (@modelStrings, "");
		}
		else {
			push (@modelStrings, "$1: ");
			$mIndex = 0;
			$lastModel = $1;
		}
		$index++;
		$mIndex++;
    }
}

die "no models read, possibly malformed config file" if !@modelStrings;


my %nbest;

foreach my $file (@ARGV) {
    
    open(FILE, $file);
    
    my $mIndex = 0;
    my $mNum = 0;
    my $docNum = 0;
    my $first = 1;
    my $docString;
    my $trans = "";

    while (<FILE>) {
		if (/<seg id="\d+"> *(.*) *<\/seg/) {
			$trans .= "$1 \\n ";
		}
		elsif (/<\!-- DOC ([\d\.-]+) - \[(.*)\]/) {
			my $total = $1;
			my @weights = split(/, /, $2);

			if ($docString) {
				$docString =~ s/\#\#TRANS\#\#/$trans/;
				if (!defined $nbest{$docNum}) {
					$nbest{$docNum} = $docString;
				}
				else {
					$nbest{$docNum} = $docString.$nbest{$docNum};
				}
				$trans = "";
			}
			
			$docString = "$docNum ||| ##TRANS## ||| ";
			
			foreach my $w (@weights) {
				$docString .= $modelStrings[$mNum];
				$docString .= "$w ";
				$mNum++;
			}

			$docString .= "||| $total\n";

			$docNum++;
			$mIndex = 0;
			$mNum = 0;
			$first = 1;
		}
    }
    # save last document
    if ($docString) {
		$docString =~ s/\#\#TRANS\#\#/$trans/;
		if (!defined $nbest{$docNum}) {
			$nbest{$docNum} = $docString;
		}
		else {
			$nbest{$docNum} = $docString.$nbest{$docNum};
		}
    }
    close FILE;
}



foreach my $num (sort {$a<=>$b} keys %nbest) {
    print $nbest{$num};
}



# MOSES format example: 0 ||| herr cashman ! jag är mer än glad över att höra goda nyheter eftersom jag var på officiellt besök i irland förra veckan , och , naturligtvis , var vi alla väntar på detta meddelande .  ||| d: 0 lm: -121.747 w: -36 tm: -23.2388 -44.337 -18.9698 -46.7882 19.9979 ||| -15.3356

#DOCENT FORMAT EXAMPLE:
#<doc NAME="Skinner" docid="60-31">
#<!-- DOC -901.814 - [-58.808, -159.628, -45.6944, -5849.73, -298, -33, -4, 0, -720, -2, -3439.78, -499.219, -1096.08, -350.943, -1008.24, 313.967] -->
#<!-- SEG -9.1258 - [0, 0, 0, 0, -5, -1, 0, 0, -16, 0, -45.5353, -5.79235, -22.1427, -5.07301, -14.849, 4.99948] -->
#<seg id="1">herr talman ! riskkapital är nödvändig för att skapa sysselsättning , eftersom mellan 80 och 90 procent av medlen från riskkapital går mot att anställa människor .</seg>
