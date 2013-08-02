#!/usr/bin/perl
#-*-perl-*-

use strict;


my %ModelScore=();
my %operations=();
my %accepted=();


my $doc=1;
my $steps=0;

while (<>){
    if (/^\*\s([0-9]+)\s([0-9]+)\s(-[0-9\.]+)\s$/){
	$ModelScore{$2}{$1} = $3;
    }
    elsif (/^Document\s([0-9]+), approaching\s([0-9]+)\ssteps./){
	($doc,$steps) = ($1,$2);
    }
    elsif(/^([0-9]+)\s([0-9]+)\s([A-Za-z]+)/){
	$operations{$steps}{$doc}{$3} = $1;
	$accepted{$steps}{$doc}{$3} = $2;
    }
}

foreach my $s (sort {$a <=> $b} keys %ModelScore){
    print $s,"\t";
    foreach my $d (sort {$a <=> $b} keys %{$ModelScore{$s}}){
	print $ModelScore{$s}{$d},"\t";
    }
    # foreach my $d (sort {$a <=> $b} keys %{$ModelScore{$s}}){
    # 	foreach my $o (sort keys %{$operations{$s}{$d}}){
    # 	    print $operations{$s}{$d}{$o},"\t";
    # 	    print $accepted{$s}{$d}{$o},"\t";
    # 	}
    # }
    print "\n";
}



my $count=0;

foreach my $s (sort {$a <=> $b} keys %ModelScore){
    open F, ">operations.$count";
    print F "$s\t";
    foreach my $o (sort keys %{$operations{$s}{0}}){
	print F $o,"\t";
    }
    print F "\n";
    foreach my $d (sort {$a <=> $b} keys %{$ModelScore{$s}}){
	print F "doc",$d+1,"\t";
	foreach my $o (sort keys %{$operations{$s}{$d}}){
	    print F $accepted{$s}{$d}{$o}/($operations{$s}{$d}{$o}+$accepted{$s}{$d}{$o}),"\t";
	}
	print F "\n";
    }
    $count++;
}
