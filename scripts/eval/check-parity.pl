#!/usr/bin/perl
#-*-perl-*-


my $doc=0;
my ($odd,$even) = (0,0);
my $violations = 0;
my $total=0;

while(<>){
    if (/\<\/doc\>/){
	$doc++;
	print "document $doc: even=$even\todd=$odd\n";
	$violations += $odd>$even ? $even : $odd;
	($odd,$even) = (0,0);
    }
    next unless (/\<seg/);
    s/<.*?>//g;
    @w = split(/\s+/);
    if ($#w % 2){$even++}
    else{$odd++;}
    $total++;
}


print "total number of parity violations: $violations ($total)\n";

