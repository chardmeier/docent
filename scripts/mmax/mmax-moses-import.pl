#!/usr/bin/perl
#
# convert (factored) Moses files into MMAX format
# (creates markables for all factors)
#
# USAGE: mmax-moses-import.pl [-x xmlfile] destdir [factors] < mosesfile
#
# - xmlfile is an optional file in NIST XML with document boundaries
# - destdir is the output directory (will be created)
# - mosesfile is the text with one sentence per line (factors possible)
# - fators is a space-separated list of factor names (after word)
#


use Getopt::Std;
use strict;

use FindBin qw($Bin);

our ($opt_x);
getopts('x:');

binmode(STDIN, ":utf8");
binmode(STDERR, ":utf8");

my $destdir = shift(@ARGV);
die "$destdir exists already!" if (-e $destdir);
system("cp -r $Bin/mmax-skeleton $destdir");

my @factors = @ARGV;
@ARGV = ();
unshift(@factors,'word');

my %doc = ( '0' => '000_document.mmax' );
if ($opt_x){
    get_document_boundaries($opt_x,\%doc);
}


# print basic mmax file

foreach (keys %doc){
    open F,">$destdir/$doc{$_}.mmax" 
	|| die "cannot write to $destdir/$doc{$_}.mmax";
    print F '<?xml version="1.0" encoding="UTF-8"?>
<mmax_project>
<words>';
    print F &escape($doc{$_});
    print F '_words.xml</words>
<keyactions></keyactions>
<gestures></gestures>
</mmax_project>';
    close F;
}



my $count=0;
my $bh = undef;
my $sh = undef;
my %fh = ();
my $wid = 0;
my $sid = 0;
my %mid = ();

while (<>){
    chomp;
    my @tokens = split(/\s+/);

    ## check for factors
    unless (@factors){
	$factors[0] = 'word';
	my @f = split(/\|/,$tokens[0]);
	if (@f){
	    foreach (1..$#f){
		push(@factors,'factor'.$_);
	    }
	}
    }

    #--------------------------------------------------------------
    # open new documents
    #--------------------------------------------------------------
    if (exists $doc{$count}){
	$sid = 0;
	$wid = 0;
	%mid = ();
	if (defined $bh){
	    print $bh "</words>\n";
	    close $bh;
	}
	if (defined $sh){
	    print $sh "</markables>\n";
	    close $sh;
	}
	foreach my $f (values %fh){
	    print $f "</markables>\n";
	    close $f;
	}

	open $bh,">$destdir/Basedata/$doc{$count}_words.xml" 
	    || die "cannot write to $destdir/Basedata/$doc{$count}_words.xml";
	binmode($bh, ":utf8");
	print $bh '<?xml version="1.0" encoding="UTF-8" ?>
<!DOCTYPE words SYSTEM "words.dtd">
<words>
';
	open $sh,">$destdir/markables/$doc{$count}_sentence_level.xml" 
	    || die "cannot write to $destdir/markables/$doc{$count}_sentence_level.xml";
	print $sh '<?xml version="1.0" encoding="UTF-8" ?>
<!DOCTYPE markables SYSTEM "markables.dtd">
<markables xmlns="www.eml.org/NameSpaces/sentence">
';
	foreach (1..$#factors){
	    my $f = $factors[$_];
	    open $fh{$f},">$destdir/markables/$doc{$count}_${f}_level.xml" 
		|| die "cannot open factor markable";
	    my $h = $fh{$f};
	    binmode($h, ":utf8");
	    print $h '<?xml version="1.0" encoding="UTF-8" ?>
<!DOCTYPE markables SYSTEM "markables.dtd">
<markables xmlns="www.eml.org/NameSpaces/';
	    print $h $f,'">',"\n";

	    unless (-e "$destdir/Schemes/${f}_scheme.xml"){
		open S,">$destdir/Schemes/${f}_scheme.xml";
		print S '<?xml version="1.0" encoding="ISO-8859-1"?>
<annotationscheme>
        <attribute id="level_',$f,'" name="tag" text="factor"  type="freetext">
                <value id="value_1000" name="tag"/>
        </attribute>
</annotationscheme>';
		close S;
	    }
	    unless (-e "$destdir/Customizations/${f}_customization.xml"){
		open S,">$destdir/Customizations/${f}_customization.xml";
		print S '<?xml version="1.0" encoding="UTF-8"?>
<customization>
</customization>
';
		close S;
	    }
	}
    }

    #--------------------------------------------------------------
    ## print sentence markable
    #--------------------------------------------------------------
    my $start = $wid+1;
    my $end = $wid+@tokens;
    print $sh '<markable mmax_level="sentence" orderid="',$sid;
    print $sh '" id="markable_',$sid;
    print $sh '" span="word_',$start,'..word_',$end,'" />',"\n";


    #--------------------------------------------------------------
    # save tokens and factors (markables)
    #--------------------------------------------------------------
    foreach (@tokens){
	$wid++;
	my @parts = split(/\|/);
	print $bh '<word id="word_',$wid,'">';
	print $bh &escape($parts[0]);
	print $bh '</word>',"\n";

	foreach (1..$#factors){
	    my $f = $factors[$_];
	    my $h = $fh{$f};
	    $mid{$f}++;
	    print $h '<markable id="markable_',$mid{$f};
	    print $h '" span="word_',$wid;
	    print $h '" tag="',&escape($parts[$_]);
	    print $h '" mmax_level="',$f,'" />',"\n";
	}
    }
    $sid++;
    $count++;
}



if (defined $bh){
    print $bh "</words>\n";
    close $bh;
}
if (defined $sh){
    print $sh "</markables>\n";
    close $sh;
}



#--------------------------------------------------------------


sub escape{
    my $str = shift;
    $str =~s/\&/\&amp\;/g;
    $str =~s/\>/\&gt\;/g;
    $str =~s/\</\&lt\;/g;
    return $str;
}

sub get_document_boundaries{
    my ($xmlfile,$doc) = @_; 
    open F,"<$xmlfile" || die "cannot read from $xmlfile";
    my $count=0;
    while (<F>){
	if (/docid=\"([^\"]+)\"/){
	    my $docname = sprintf "%03d_%s",$count,$1;
	    $$doc{$count} = $docname;
	    next;
	}
	elsif (/\<seg /){
	    $count++;
	}
    }
    close(F);
}


