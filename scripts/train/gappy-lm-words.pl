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
#   -w <nr> ....... pseudo documents containing <nr> sentences
#   -S <nr> ....... source factor to be printed (default = 0)
#   -T <nr> ....... target factor to be printed (default = 0)
#   -U ............ skip unaligned tokens
#   -M ............ don't merge multi-word alignments
#   -X <SrcCond> .. source language conditions
#   -Y <TrgCond> .. target language conditions
#
# conditions = disjunction of conjunctive conditions!
#              field=value&field=value|field=value|field=value&field=value ...

use strict;
use Getopt::Std;

our ($opt_b,$opt_d,$opt_n,$opt_p,$opt_l,$opt_s,$opt_t,$opt_w, $opt_S, $opt_T,$opt_U,$opt_M,$opt_X,$opt_Y);
getopts('bd:n:p:l:st:w:S:T:UMX:Y:');


# my $MinWordLength = $opt_l || 4;
# my $PosTag = $opt_p || 'NOUN';

my $MinWordLength = $opt_l;
my $PosTag   = $opt_p;
my $DocTag   = $opt_d || 'SPEAKER';
my $FieldDel = $opt_t || '\|';
my $FieldNr  = $opt_n || 1;
my $srcFactor = $opt_S || 0;
my $trgFactor = $opt_T || 0;

my @SrcConditions = ();
my @TrgConditions = ();

# source language conditions
if ($opt_X){
    my @disj = split(/\|/,$opt_X);
    foreach (@disj){
	my $idx = @SrcConditions;
	my @conj = split(/\&/);
	foreach (@conj){
	    my ($key,$val) = split(/\=/);
	    $SrcConditions[$idx]{"RE:$key"} = $val;
	}
    }
}
elsif ($opt_p || $opt_l){
    %{$SrcConditions[0]} = (min_length => $MinWordLength,
			    "RE:$FieldNr" => $PosTag );
}

# target language conditions
if ($opt_Y){
    my @disj = split(/\|/,$opt_Y);
    foreach (@disj){
	my $idx = @TrgConditions;
	my @conj = split(/\&/);
	foreach (@conj){
	    my ($key,$val) = split(/\=/);
	    $TrgConditions[$idx]{"RE:$key"} = $val;
	}
    }
}


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
my $sentCount=0;
while (<ST>){
    my $trgsent = <TT>;
    my $algstr = <A>;
    my $idstr = <I> if ($usexml);

    chomp($_,$trgsent,$algstr,$idstr);
    s/^\s*//;$trgsent=~s/^\s*//;

    my @srcwords = split(/\s+/,$_);
    my @trgwords = split(/\s+/,$trgsent);
    my @links    = split(/\s+/,$algstr);
    $sentCount++;

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

    my @selected = select_words(\@srcwords,\@trgwords,\%src2trg,
				\@SrcConditions,\@TrgConditions);

    my @ids = split(/\s+/,$trgids);

    print join(' ',@selected);
    if ((exists $trgend{$ids[-1]}) || $opt_s || 
	($opt_w && ($sentCount > $opt_w))){
	print "\n";
	$docCount++;
	if (! ($docCount % 100) ){print STDERR '.';}
	if (! ($docCount % 5000) ){print STDERR $docCount,"\n";}
	$sentCount=0;
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


sub match_conditions{
    my ($fields,$conditions) = @_;
    if (defined $$conditions{min_length}){ 
	return 0 if (length($$fields[0]) < $$conditions{min_length});
    }
    foreach (0..$#{$fields}){
	if (defined $$conditions{"RE:$_"}){
	    unless ($$fields[$_]=~/$$conditions{"RE:$_"}/){
		return 0;
	    }
	}
    }
    return 1;
}

sub match_word{
    my ($fields,$conditions) = @_;
    foreach my $cond (@{$conditions}){
	return 1 if (match_conditions($fields,$cond));
    }
    return 0;
}

sub select_trg_words{
    my ($words,$conditions) = @_;
    my @selected = ();
    foreach my $i (0..$#{$words}){
	my @fields = split(/$FieldDel/,$$words[$i]);
	next unless (match_word(\@fields,$conditions));
	push(@selected,$fields[$trgFactor]);
    }
    return @selected;
}


sub select_words{
    my ($words1,$words2,$links,$SrcConditions,$TrgConditions) = @_;

    unless ($opt_b || @{$SrcConditions}){
	return select_trg_words($words2,$TrgConditions);
    }

    my $checkTrg = 0;
    $checkTrg = 1 if (keys %{$TrgConditions});

    my %selected=();
    foreach my $i (0..$#{$words1}){
	my @fields = split(/$FieldDel/,$$words1[$i]);
	next unless (match_word(\@fields,$SrcConditions));

	# my $ok = 1;
	# foreach (0..$#fields){
	#     if (defined $$SrcConditions{"RE:$_"}){
	# 	unless ($fields[$_]=~/$$SrcConditions{"RE:$_"}/){
	# 	    $ok = 0;
	# 	    last;
	# 	}
	#     }
	# }
	# next unless ($ok);

	if ($opt_M){
	    for my $j (keys %{$$links{$i}}){
		my @trgFields = split(/$FieldDel/,$$words2[$j]);
		if ($checkTrg){
		    next unless (match_word(\@trgFields,$TrgConditions));
		}
		if ($opt_b){
		    $selected{$j} = $fields[$srcFactor].'=TRG+'.$trgFields[$trgFactor];
		}
		else{
		    $selected{$j} = 'TRG+'.$trgFields[$trgFactor];
		}
	    }
	}
	else{
	    ## NEW: only one token per selected source word!
	    ## (before: one token per aligned target word, no token if empty alignment)
	    my $trgStr = 'TRG';
	    my $trgPos = undef;
	    my $linkCount=0;
	    for my $j (keys %{$$links{$i}}){
		$linkCount++;
		my @trgFields = split(/$FieldDel/,$$words2[$j]);
		if ($checkTrg){
		    next unless (match_word(\@trgFields,$TrgConditions));
		}
		$trgStr .= '+'.$trgFields[$trgFactor];
		if ((not defined $trgPos) || ($j < $trgPos)){
		    $trgPos = $j;
		}
	    }
	    $trgPos = $i unless (defined $trgPos);
	    unless ($opt_U && ($linkCount == 0)){     # skip unaligned if opt_U
		if ($opt_b){
		    $selected{$trgPos} = $fields[$srcFactor].'='.$trgStr;
		}
		else{
		    $selected{$trgPos} = $trgStr;
		}
	    }
	}
    }
    my @words = ();
    foreach (sort {$a <=> $b} keys %selected){
	push(@words,$selected{$_});
    }
    return @words;
}
