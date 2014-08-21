#!/usr/bin/perl

my $length=4;

while(<>){
  next unless (/\<seg/);
  s/<.*?>//g;
  @w = split(/\s+/);
  # print $#w,"\n";
  $diff+=abs($#w+1-$length);
  $nr++;
}

print "average diff=",$diff/$nr,"\n";

