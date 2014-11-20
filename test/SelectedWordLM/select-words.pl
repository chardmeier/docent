#!/usr/bin/perl

my $MinWordLength = 4;


my ($srctxtfile,$trgtxtfile,$algfile,$idfile,$xmldir) = @ARGV;


open ST,"gzip -cd < $srctxtfile |" || die "cannot read from $srctxtfile";
open TT,"gzip -cd < $trgtxtfile |" || die "cannot read from $trgtxtfile";

open A,"gzip -cd < $algfile |" || die "cannot read from $algfile";
open I,"gzip -cd < $idfile |" || die "cannot read from $idfile";

my $opensrcxml = undef;
my $opentrgxml = undef;

while (<ST>){
    my $trgsent = <TT>;
    my $algstr = <A>;
    my $idstr = <I>;

    chomp($_,$trgsent,$algstr,$idstr);

    my @srcwords = split(/\s+/$_);
    my @trgwords = split(/\s+/$trgsent);
    my @links    = split(/\s+/,$algstr);

    my ($srcxml,$trgxml,$srcids,$trgids) = split(/\t+/,$idstr);

    unless ($opensrcxml eq $srcxml){
	open SX,"gzip -cd < $xmldir/$srcxml |" || die "cannot read from $xmldir/$srcxml";
	$opensrcxml = $srcxml;
    }
    unless ($opentrgxml eq $trgxml){
	open SX,"gzip -cd < $xmldir/$trgxml |" || die "cannot read from $xmldir/$trgxml";
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

    my @selected = select_words(\@trgwords,\@srcwords,\%trg2src,{min-length = $MinWordLength});

    print join(' ',@selected);
    print "\n" if (end_of_document(*TX,$trgids));

}

sub end_of_documents{
}

sub select_words{
    return ();
}
