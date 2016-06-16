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

my $first = 1;

my ($nTokens, $nTypes) = (0.0,0);
my ($nsyllables, $nsents) = (0.0,0);
my ($nlongWords, $nXlongWords) = (0.0,0);

my %values = ( "ASL" => [],  "ASW" => [], "FC" => [], "LW" => [], "XLW" => [], "LIX" => [], "TT" => [], "OVIX" => [], "token" => [], "type" => [], "syll" => [], "sent" => [], "nLW" => [], "nXLW" => []);

#format of sentences:
#<seg id="1">400 jobs will shifted</seg>

my %types;

while (<STDIN>) {

    if (/<doc/) {
		
		if ($first) {
			$first = 0;
			next;
		}

		$nTypes =keys(%types);
		push(@{$values{"type"}},$nTypes);

		push(@{$values{"ASL"}},$nTokens/$nsents);
		push(@{$values{"ASW"}},$nsyllables/$nTokens);
		push(@{$values{"FC"}},206.835-(1.015*@{$values{"ASL"}}[-1]) -(84.6*@{$values{"ASW"}}[-1]));

		push(@{$values{"LW"}},100 * $nlongWords/$nTokens);
		push(@{$values{"XLW"}},100 * $nXlongWords/$nTokens);
		push(@{$values{"LIX"}},@{$values{"ASL"}}[-1] + @{$values{"LW"}}[-1]);

		push(@{$values{"TT"}},$nTypes/$nTokens);
		push(@{$values{"OVIX"}},log($nTokens)/(log(2.0-(log($nTypes)/log($nTokens)))));

		push(@{$values{"syll"}},$nsyllables); 
		push(@{$values{"sent"}},$nsents); 
		push(@{$values{"nLW"}},$nlongWords);   
		push(@{$values{"nXLW"}},$nXlongWords);   
		push(@{$values{"token"}},$nTokens);  
		
		
		($nTokens) = (0.0);
		($nsyllables, $nsents) = (0.0,0);
		($nlongWords, $nXlongWords) = (0.0,0);
		%types = ();

		next;
    }

    elsif (/<seg id="\d+"> *(.*) *<\/seg>/) {

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



#print "ABSOLUTE NUMBERS:\nSentences: $nsents \nTokens: $nTokens \nTypes: $nTypes\nSyllables: $nsyllables\nLong words: $nlongWords\nExtra long words: $nXlongWords\n\n";

#printf "AUXILLIARY MEASURES:\nlw ratio: %9.3f\nxlw ratio: %8.3f\nASL: %14.3f\nASW: %14.3f\n\n", $perLongWords, $perXLongWords, $ASL, $ASW;

#printf "METRICS:\nLIX: %14.3f\nFlesh: %12.3f\nTT ratio: %9.3f\nOVIX: %13.3f\n\n", $LIX, $FC, $TT, $OVIX;

my $num = 0;
foreach my $metric ( "token" , "type" , "syll" , "sent" , "nLW" , "nXLW" ,  "ASW" , "FC" , "LW" , "TT" , "XLW" , "LIX" , "ASL" , "OVIX"  ) {
    my $all = $values{"$metric"};

    print "\n" if ($num == 6 || $num == 10);
    $num++;

    my $tot = 0.0;

    foreach my $v (@{$all}) {
		$tot += $v;
		#printf STDERR "%.3f ", $v;
    }
    my $average = $tot/@{$all};
    #print STDERR "\n$average\n";
    my $sqtotal = 0.0;
    foreach my $v (@{$all}) {
        $sqtotal += ($average-$v) ** 2;
		#printf STDERR " %.5f",($average-$v) ** 2;
    }
    my $stddev = sqrt($sqtotal / (@{$all}-1));
    #printf STDERR "\n%.5f  %.5f\n",$sqtotal, $stddev ;

    printf "$metric: %20.4f (%4f)\n", $average, $stddev;
}
