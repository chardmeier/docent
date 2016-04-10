#!/usr/bin/perl
#
# add document boundaries to a conll file
# (which is required for mmax-conll-inport.sh)
# document markup comes from NIST XML
#
# USAGE: add-docids-to-conll.pl xmlfile < conllfile
#

my $sgmfile = shift(@ARGV);

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");

open F,"<$sgmfile" || die "cannot read from $sgmfile";
my %doc=();
my $count=0;
while (<F>){
    if (/docid=\"([^\"]+)\"/){
	$doc{$count} = $1;
	next;
    }
    elsif (/\<seg /){
	$count++;
    }
}



$count=0;
print "###C:begin document $doc{0}\n";

while (<>){
    if (/^\s*$/){
	$count++;
	if (exists $doc{$count}){
	    print "###C:end document ",$doc{$count-1},"\n";
	}
	print;
	if (exists $doc{$count}){
	    print "###C:begin document $doc{$count}\n";
	}
    }
    else{
	print;
    }
}
print "###C:end document ",$doc{$count},"\n";
