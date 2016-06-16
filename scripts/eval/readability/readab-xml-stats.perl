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

#print STDERR "Calculating various readability metrics, language: $LANG\n\n";

my $first = 1;

my ($nTokens, $nTypes) = (0.0,0);
my ($nsyllables, $nsents) = (0.0,0);
my ($nlongWords, $nXlongWords) = (0.0,0);

my %values = ( "ASL" => [],  "ASW" => [], "FC" => [], "LW" => [], "XLW" => [], "LIX" => [], "TT" => [], "OVIX" => [], "token" => [], "type" => [], "syll" => [], "sent" => [], "nLW" => [], "nXLW" => []);

#format of sentences:
#<seg id="1">400 jobs will shifted</seg>

my $size;

my %types;

while (<STDIN>) {

    if (/<doc docid=\".*-(\d+)/) {
		
		my $newSize = $1;
		if ($first) {
			$first = 0;
			$size = $newSize;
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
		push(@{$values{"OVIX"}},log($nTokens)/(log(2.0-(log($nTypes)/log($nTokens+1)))));

		push(@{$values{"syll"}},$nsyllables); 
		push(@{$values{"sent"}},$nsents); 
		push(@{$values{"nLW"}},$nlongWords);   
		push(@{$values{"nXLW"}},$nXlongWords);   
		push(@{$values{"token"}},$nTokens);

		#if ($size >=5 && $size <= 100) {
			printf "%3d",$size;
			my $num = 0;
			foreach my $metric ( "token" , "type" , "syll" , "sent" , "nLW" , "nXLW" ,  "ASW" , "FC" , "LW" , "TT" , "XLW" , "LIX" , "ASL" , "OVIX"  ) {
			my $m = @{$values{"$metric"}}[-1];
			if ($num<=5) {
				printf " %4d",$m;
			}
			else {
				printf " %9.5f", $m;
			}
			$num++;
			}
			print "\n";
		#}
		
		($nTokens) = (0.0);
		($nsyllables, $nsents) = (0.0,0);
		($nlongWords, $nXlongWords) = (0.0,0);
		%types = ();

		$size = $newSize;
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


#printing final doc
			printf "%3d",$size;
			my $num = 0;
			foreach my $metric ( "token" , "type" , "syll" , "sent" , "nLW" , "nXLW" ,  "ASW" , "FC" , "LW" , "TT" , "XLW" , "LIX" , "ASL" , "OVIX"  ) {
			my $m = @{$values{"$metric"}}[-1];
			if ($num<=5) {
				printf " %4d",$m;
			}
			else {
				printf " %9.5f", $m;
			}
			$num++;
			}
			print "\n";



print "\n\n";
my $n = 0;
foreach my $metric ( "token" , "type" , "syll" , "sent" , "nLW" , "nXLW" ,  "ASW" , "FC" , "LW" , "TT" , "XLW" , "LIX" , "ASL" , "OVIX"  ) {
    my $all = $values{"$metric"};

    print "\n" if ($n == 6 || $n == 10);
    $n++;

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
