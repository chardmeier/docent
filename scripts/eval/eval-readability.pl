#!/usr/bin/perl
#-*-perl-*-

use FindBin qw($Bin);


# my $lcurve_log = shift(@ARGV);

my $testset_src = shift(@ARGV);
my $testset_ref = shift(@ARGV);

my @output = @ARGV;

die "cannot find test set (src) $testset_src\n" unless (-e $testset_src);
die "cannot find reference set $testset_ref\n" unless (-e $testset_ref);

my %bleu = ();
my %parity = ();

my $steps = 0;
foreach my $o (@output){
    $steps++;
    if ($o=~/[^0-9]([0-9]+).xml/){
	$steps=$1;
    }
    print STDERR "evaluate after $steps steps ...\n";

    my $bleu_out = `$Bin/mteval-v13.pl -s $testset_src -r $testset_ref -t $o`;
    if ($bleu_out=~/BLEU score = ([0-1].[0-9]+)\s/s){
	$bleu{$steps} = $1;
    }

    # @{$parity{$steps}} = type_token_ratio($o);
    @{$parity{$steps}} = readability($o);
}

foreach (sort {$a <=> $b} keys %bleu){
    print "$_\t$bleu{$_}\t";
    print join("\t",@{$parity{$_}});
    print "\n";
}


sub type_token_ratio{
    my $file=shift;
    my $doc=0;

    my %words = ();
    my $nrTokens = 0;
    my $nrTypes = 0;
    my $total = 0;

    open F,"<$file" || die "cannot read from $file\n";
    my @ret=();

    while(<F>){
	if (/\<\/doc\>/){
	    $doc++;
	    my $types = scalar keys %words;
	    if ($types){ push(@ret,$nrTokens / $types); }
	    else{ push(@ret,0); }
	    $nrTypes += scalar keys %words;
	    $total += $nrTokens;
	    %words = ();
	    $nrTokens = 0;
	}
	next unless (/\<seg/);
	s/<.*?>//g;
	@w = split(/\s+/);
	foreach (@w){ $words{$_}++; }
	$nrTokens += scalar @w;
    }

    $nrTypes += scalar keys %words;
    unshift(@ret,$total/$nrTypes);
    return @ret;
}


sub readability{


    my $file=shift;

    open F,"<$file" || die "cannot read from $file\n";
    my @ret=();

    binmode(F, ":utf8");
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

    # $LANG = $ARGV[0] if @ARGV >=1;
    # print STDERR "Calculating various readability metrics, language: $LANG\n\n";

    my $first = 1;

    my ($nTokens, $nTypes) = (0.0,0);
    my ($nsyllables, $nsents) = (0.0,0);
    my ($nlongWords, $nXlongWords) = (0.0,0);

    my %values = ( "ASL" => [],  "ASW" => [], "FC" => [], "LW" => [], "XLW" => [], "LIX" => [], "TT" => [], "OVIX" => [], "token" => [], "type" => [], "syll" => [], "sent" => [], "nLW" => [], "nXLW" => []);

#format of sentences:
#<seg id="1">400 jobs will shifted</seg>

    my %types;
    
    while (<F>) {
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

    my @ret=();

#    foreach my $metric ( "token" , "type" , "syll" , "sent" , "nLW" , "nXLW" ,  "ASW" , "FC" , "LW" , "TT" , "XLW" , "LIX" , "ASL" , "OVIX"  ) {
    foreach my $metric ( "nLW" , "nXLW" ,  "ASW" , "FC" , "LW" , "TT" , "XLW" , "LIX" , "ASL" , "OVIX"  ) {
	my $all = $values{"$metric"};
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
	push(@ret, $average);
	# push(@ret, $stddev);
    }
#    foreach my $metric ( "nLW" , "nXLW" ,  "ASW" , "FC" , "LW" , "TT" , "XLW" , "LIX" , "ASL" , "OVIX"  ) {
#	push(@ret,@{$values{$metric}});
 #   }
    return @ret;
}
