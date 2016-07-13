#!/usr/bin/env perl
package mteval_insert_segs;
use strict;
use warnings;
use open qw(:std :utf8);

our $VERSION = 0.01;

use Getopt::Long qw(:config no_ignore_case bundling);


my %Opts = (
    output   => '-',
    segsfile => '-',
    trglang  => 'en',
);
ARGV: while (@ARGV) {
    GetOptions( \%Opts
        , 'output|o=s'
        , 'srcfile|s=s'
        , 'trglang|t=s'

        , 'update|u'

        , 'help|h|?'  => sub { usage() }
        , 'version|V' => sub { print "mteval-insert-segs.pl v$VERSION\n"; exit }
    ) or usage();
    while ((scalar @ARGV) && ($ARGV[0] !~ m{^-.+}x)) {
        if      (! $Opts{srcfile}) {
            $Opts{srcfile} = shift;
        } elsif ('-' eq $Opts{segsfile} && -t) {
            $Opts{segsfile} = shift;
        } else {
            usage( "excess files on the command line" );
        }
    }
}

eval { require XML::LibXML };
die "$0: missing Perl dependency: XML::LibXML\n" if $@;

usage( "source file not specified" )  unless exists $Opts{srcfile};
die "$0: file '$Opts{srcfile}' not found, or empty\n"  unless -s $Opts{srcfile} && ! -d $Opts{srcfile};
my $input = eval { XML::LibXML->load_xml( location => $Opts{srcfile} ) }
    or die "$0: error parsing '$Opts{srcfile}': $@\n";
my $tstset;
{   my @srcsets = $input->findnodes( '//srcset' );
    if      (0 == scalar @srcsets) {
        die "$0: no 'srcset' node found in '$Opts{srcfile}'\n";
    } elsif (1 < scalar @srcsets) {
        die "$0: too many 'srcset' nodes in '$Opts{srcfile}', only one is allowed\n";
    }
    if  ($Opts{update} && scalar $input->findnodes( '//tstset' )) {
        die "$0: ambiguous: '$Opts{srcfile}' already contains a 'tstset'\n";
    }
    $tstset = $srcsets[0]->cloneNode(1);
    $tstset->setNodeName( 'tstset' );
    foreach my $attr (qw(trglang)) {
        $tstset->setAttribute( $attr => $Opts{ $attr } )  if exists $Opts{ $attr };
    }
}

my $segsfh;
if ('-' eq $Opts{segsfile}) {
    $segsfh = \*STDIN;
} else {
    open $segsfh, '<', $Opts{segsfile}
        or die "$0: file '$Opts{segsfile}' could not be opened: $!\n";
}

my $xml;
if ($Opts{update}) {
    $xml = $input;
    $xml->documentElement->appendChild( $tstset );
} else {
    $xml = XML::LibXML::Document->new( '1.0', 'utf-8' );
    $xml->setDocumentElement(
        my $root = XML::LibXML::Element->new( 'mteval' )
    );
    $root->appendChild( $tstset );
}

foreach my $seg ($tstset->findnodes( './/seg' )) {
    my $line = <$segsfh>;
    die "$0: too few segments in '$Opts{segsfile}' for mteval set '$Opts{srcfile}\n"
        unless defined $line;
    chomp $line;

    $seg->removeChildNodes;
    $seg->appendText( $line );
}
die "$0: too many segments in '$Opts{segsfile}' for mteval set '$Opts{srcfile}\n"
    if <$segsfh>;
close $segsfh;

my $fh;
if ($Opts{output} eq '-') {
    $fh = \*STDOUT;
} else {
    open $fh, '>', $Opts{output}
        or die "$0: unable to write to '$Opts{output}': $!\n";
}
binmode $fh;
print {$fh} $xml->toString();
close $fh;

exit;


sub usage
{
    my $msg = shift;
    print "$msg\n" if defined $msg;

    print <<"EOT";
Usage: mteval-insert-segs.pl [OPTIONS] -s SOURCE.xml SEGSMENTSFILE
   or: mteval-insert-segs.pl [OPTIONS] -s SOURCE.xml < SEGMENTS

NOTE: This script relies on the libxml2 system library and the Perl package
XML::LibXML.

Mandatory arguments to long options are mandatory for short options too.
  -o, --output FILE  name of the file to which the XML result is written
  -u, --update       write

  -h, --help     display this help and exit
  -V, --version  output version information and exit
EOT

    exit( defined $msg ? 1 : 0 );
}
