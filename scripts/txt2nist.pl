#!/usr/bin/perl
#-*-perl-*-
#
# txt2xml.pl - convert plain text files to NIST format used by mteval
#
#  txt2xml.pl [-r|-s|-t] textfile1 textfile2 ... > output.xml
#
# -r ... generate reference file
# -s ... generate source file
# -t .... generate test set file
#
# -m max .... take only the first <max> sentences from each document
#


use Getopt::Std;

my %opts=();
getopts('rstm:',\%opts);

$type = exists $opts{r} ? 'ref' : exists $opts{t} ? 'tst' : 'src';


print "<?xml version=\"1.0\"?>\n<mteval>\n";
if ($type eq 'ref'){
    print "<refset refid=\"ref\" setid=\"test\" trglang=\"any\" srclang=\"any\" sysid=\"sys\">\n";
}
elsif ($type eq 'tst'){
    print "<tstset setid=\"test\" trglang=\"any\" srclang=\"any\" sysid=\"sys\">\n";
}
else{
    print "<srcset setid=\"test\" trglang=\"any\" srclang=\"any\" sysid=\"sys\">\n";
}


my $segcount=0;

unless (@ARGV){
    print_doc(*STDIN,'test');
}
my $txtcount=0;
foreach (@ARGV){
    $txtcount++;
    open $fh,"<$_" || die "cannot open file $_\n";
    print_doc($fh,"test$txtcount");
}

$type eq 'ref' ? print "</refset>" : $type eq 'tst' ? print "</tstset>" : print "</srcset>";
print "\n</mteval>\n";




sub print_doc{
    my ($fh,$id) = @_;
    print "<doc sysid=\"sys\" docid=\"$id\" genre=\"any\" origlang=\"any\">\n";

    my $scount=0;
    while (<$fh>){
	chomp;
	s/\&/\&amp;/gs;
	s/\>/\&gt;/gs;
	s/\</\&lt;/gs;
	$segcount++;
	$scount++;
	print "<seg id=\"$segcount\">$_</seg>\n";
	last if ($opts{m} && ($scount>=$opts{m}));
    }
    print "</doc>\n";
}


