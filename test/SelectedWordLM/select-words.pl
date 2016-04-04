#!/usr/bin/perl

use strict;

my $MinWordLength = 4;
my $DocTag = 'SPEAKER';


my ($srctxtfile,$trgtxtfile,$algfile,$idfile,$xmldir) = @ARGV;


open ST,"gzip -cd < $srctxtfile |" || die "cannot read from $srctxtfile";
open TT,"gzip -cd < $trgtxtfile |" || die "cannot read from $trgtxtfile";

open A,"gzip -cd < $algfile |" || die "cannot read from $algfile";
open I,"gzip -cd < $idfile |" || die "cannot read from $idfile";

my $opensrcxml = undef;
my $opentrgxml = undef;

my %srcstart = ();
my %srcend = ();

my %trgstart = ();
my %trgend = ();

my $docCount=0;
while (<ST>){
    my $trgsent = <TT>;
    my $algstr = <A>;
    my $idstr = <I>;

    chomp($_,$trgsent,$algstr,$idstr);

    my @srcwords = split(/\s+/,$_);
    my @trgwords = split(/\s+/,$trgsent);
    my @links    = split(/\s+/,$algstr);

    my ($srcxml,$trgxml,$srcids,$trgids) = split(/\t+/,$idstr);

    unless ($opensrcxml eq $srcxml){
	%srcstart = ();
	%srcend = ();
	get_doc_structure("$xmldir/$srcxml",\%srcstart,\%srcend);
	$opensrcxml = $srcxml;
    }
    unless ($opentrgxml eq $trgxml){
	%trgstart = ();
	%trgend = ();
	get_doc_structure("$xmldir/$srcxml",\%trgstart,\%trgend);
	$opentrgxml = $trgxml;
    }

    my %src2trg=();
    my %trg2src=();

    foreach (@links){
	my ($s,$t) = split(/-/);
	$src2trg{$s}{$t}++;
	$trg2src{$t}{$s}++;
    }

    ## extract all words that are linked to a word that fulfills the condition
    ## (right now: only min-string-length is checked!)

    my @selected = select_words(\@srcwords,\@trgwords,\%src2trg,min_length => $MinWordLength);
    my @ids = split(/\s+/,$trgids);

    print join(' ',@selected);
    if (exists $trgend{$ids[-1]}){
	print "\n";
	$docCount++;
	if (! ($docCount % 100) ){print STDERR '.';}
	if (! ($docCount % 5000) ){print STDERR $docCount,"\n";}
    }
    else{ print ' '; }
}

sub get_doc_structure{
    my ($file,$start,$end) = @_;
    open F,"gzip -cd < $file |" || die "cannot read from $file";
    my $sid = undef;
    while (<F>){
	if (/<s\s+.*id\=\"([^"]*)\"/){
	    if ((not defined $sid) || (exists $$end{$sid})){
		$$start{$sid} = 1;
	    }
	    $sid = $1;
	}
	elsif (/\<$DocTag/){
	    $$end{$sid} = 1;
	}
    }
    close F;
}

sub end_of_documents{
    my ($fh,$ids) = @_;
    my @id = split(/\s+/,$ids);
    my $end
}

sub select_words{
    my ($words1,$words2,$links,%conditions) = @_;

    my %selected=();
    foreach my $i (0..$#{$words1}){
	next if (exists $conditions{min_length} && length($$words1[$i]) < $conditions{min_length});

	for my $j (keys %{$$links{$i}}){
	    $selected{$j} = $$words2[$j];
#	    push(@selected,$$words2[$j]) if ($j <= $#{$words2});
	}
    }
    my @words = ();
    foreach (sort {$a <=> $b} keys %selected){
	push(@words,$selected{$_});
    }
    return @words;
}
