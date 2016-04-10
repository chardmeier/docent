#!/usr/bin/perl
#
# select target language words linked to specific word classes
#        or words of specific lengths in the source language
# --> for target language modeling over these selected sequences
#----------------------------------------------------------------------
# USAGE: select-words.pl [OPTIONS] srctxt trgtxt algfile idfile xmldir
#
# srctxt:  plain text file with source language text
# trgtxt:  plain text file with target language text
#          (MOSES style: sentence aligned by line number)
# algfile: word alignment file (Moses/SMT style)
# idfile:  sentence/doc IDs (from OPUS)
# xmldir:  home directory of original XML files (OPUS)
#
# OPTIONS:
#
#   -p <pos> ...... label RE to be used
#   -l <length> ... minimum length of the source token
#   -d <doc> ...... tag that specifies document boundaries
#   -s ............ treat each sentence as a document
#   -t <char> ..... delimiter character (default = |)
#   -n <nr> ....... field number (default = 1)


use strict;
use Getopt::Std;

our ($opt_d,$opt_n,$opt_p,$opt_l,$opt_s,$opt_t);
getopts('d:n:p:l:st:');


# my $MinWordLength = $opt_l || 4;
# my $PosTag = $opt_p || 'NOUN';

my $MinWordLength = $opt_l;
my $PosTag   = $opt_p;
my $DocTag   = $opt_d || 'SPEAKER';
my $FieldDel = $opt_t || '\|';
my $FieldNr  = $opt_n || 1;

my ($srctxtfile,$trgtxtfile,$algfile,$idfile,$xmldir) = @ARGV;

if ($srctxtfile=~/\.gz/){
    open ST,"gzip -cd < $srctxtfile |" || die "cannot read from $srctxtfile";
}
else{ open ST,"< $srctxtfile" || die "cannot read from $srctxtfile"; }
if ($trgtxtfile=~/\.gz/){
    open TT,"gzip -cd < $trgtxtfile |" || die "cannot read from $trgtxtfile";
}
else{ open TT,"< $trgtxtfile" || die "cannot read from $trgtxtfile"; }
if ($algfile=~/\.gz/){
    open A,"gzip -cd < $algfile |" || die "cannot read from $algfile";
}
else{ open A,"< $algfile" || die "cannot read from $algfile"; }


my $usexml = 0;
if (-e $idfile){
    open I,"gzip -cd < $idfile |" || die "cannot read from $idfile";
    $usexml = 1;
}

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
    my $idstr = <I> if ($usexml);

    chomp($_,$trgsent,$algstr,$idstr);
    s/^\s*//;$trgsent=~s/^\s*//;

    my @srcwords = split(/\s+/,$_);
    my @trgwords = split(/\s+/,$trgsent);
    my @links    = split(/\s+/,$algstr);

    ## read XML document structure
    my ($srcxml,$trgxml,$srcids,$trgids);
    if ($usexml){
	($srcxml,$trgxml,$srcids,$trgids) = split(/\t+/,$idstr);

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

    my @selected = select_words(\@srcwords,\@trgwords,\%src2trg,
                                min_length => $MinWordLength,
                                "RE:$FieldNr" => $PosTag);
#                                pos_tag => $PosTag);
    my @ids = split(/\s+/,$trgids);

    print join(' ',@selected);
    if ((exists $trgend{$ids[-1]}) || $opt_s){
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

sub select_words{
    my ($words1,$words2,$links,%conditions) = @_;

    my %selected=();
    foreach my $i (0..$#{$words1}){
	next if (defined $conditions{min_length} && 
		 length($$words1[$i]) < $conditions{min_length});

	my @fields = split(/$FieldDel/,$$words1[$i]);
	my $ok = 1;
	foreach (0..$#fields){
	    if (defined $conditions{"RE:$_"}){
		unless ($fields[$_]=~/$conditions{"RE:$_"}/){
		    $ok = 0;
		    last;
		}
	    }
	}
	next unless ($ok);
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
