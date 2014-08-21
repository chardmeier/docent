#!/usr/bin/perl

while(<>){
  next unless (/\<seg/);
  s/<.*?>//g;
  @w = split(/\s+/);
  if ($#w % 2){$even++}
  else{$odd++;}
}

print "even=$even\nodd=$odd\n";

