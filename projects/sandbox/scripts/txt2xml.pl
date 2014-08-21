#!/usr/bin/perl

print "<mteval>\n";
print "<srcset setid=\"test\" srclang=\"any\">\n";
print "<doc docid=\"test\" genre=\"any\" origlang=\"any\">\n";

my $count=0;
while (<>){
    chomp;
    $count++;
    print "<seg id=\"$count\">$_</seg>\n";
}

print "</doc>\n</srcset>\n</mteval>\n";
