#!/usr/bin/perl
#-*-perl-*-

use FindBin qw($Bin);

my $testset_src = shift(@ARGV);
my $testset_ref = shift(@ARGV);

my @output = @ARGV;

die "cannot find $testset_src\n" unless (-e $testset_src);
die "cannot find $testset_ref\n" unless (-e $testset_ref);

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

    @{$parity{$steps}} = count_final($o);
}

foreach (sort {$a <=> $b} keys %bleu){
    print "$_\t$bleu{$_}\t";
    print join("\t",@{$parity{$_}});
    print "\n";
}




sub count_final{
    my $file = shift;

    my $doc=0;
    my %final=();
    my $violations = 0;
    my $total=0;
    my $sents=0;

    my @ret=();

    open F,"<$file" || die "cannot read from $file\n";
    
    while(<F>){
	if (/\<\/doc\>/){
	    $doc++;
	    my ($high) = sort { $b <=> $a } values %final;
	    my ($char) = sort { $final{$b} <=> $final{$a} } keys %final;
	    push(@ret,$char,$high);
	    $violations += $sents-$high;
	    %final=();
	    $sents=0;
	}
	next unless (/\<seg/);
	s/<.*?>//g;
	my @words = split(/\s+/);
	if (@words){ 
	    if (length($words[-1])>1){
		$final{$words[-1]}++; 
	    }
	}
	$sents++;
	$total++;
    }

    unshift(@ret,$violations,$total);
    return @ret;
}
