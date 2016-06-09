#!/usr/bin/env perl
package mteval2txt;
use strict;
use warnings;
use open qw(:std :utf8);

our $VERSION = 0.01;

use Getopt::Long qw(:config no_ignore_case bundling);


my %Opts = (
    prefix => '',
);
my @Files;
ARGV: while (@ARGV) {
    GetOptions( \%Opts
        , 'overwrite|f'
        , 'output|flat|a|o=s'
        , 'prefix|p=s'

        , 'help|h|?' => \&usage
        , 'version|V' => sub { print "mteval2txt.pl v$VERSION\n"; exit }
    ) or usage();
    while ((scalar @ARGV) && ($ARGV[0] !~ m{^-.+}x)) {
        push @Files, shift;
    }
}
usage( "specify filenames or pipe in an XML" ) unless @Files || ! -t 0;

$Opts{prefix} .= '.'  if (length $Opts{prefix}) && ($Opts{prefix} !~ m{ [/_.~-] $}x);

eval { require XML::LibXML };
die "$0: missing Perl dependency: XML::LibXML\n" if $@;

my $outfh;
if ($Opts{output}) {
    usage( "These options are mutually exclusive: 'output'('flat')/'prefix'" ) if $Opts{prefix};
    die "$0: target file '$Opts{output}' exists and overwriting has not been allowed\n"
        if (! $Opts{overwrite}) && -e $Opts{output};
    open $outfh, '>', $Opts{output}
        or die "$0: could not write file '$Opts{output}': $!\n";
}
foreach my $infile (@Files) {
    unless (-s $infile && ! -d $infile) {
        warn "$0: file '$infile' empty or not found, skipping...";
        next;
    }
    my $input;
    if ($infile =~ m{\.xml}xi) {
        $input = eval { XML::LibXML->load_xml( location => $infile ) };
    } elsif (open my $fh, '<', $infile or die "$0: error reading 'infile': $!\n") {  ## SGML
        local $/;
        my $sgml = <$fh>;
        close $fh;
        $sgml =~ s{&}{&amp;}xg;
        $input = { XML::LibXML->load_xml( string => $sgml ) };
    }
    warn "$0: error parsing '$infile': $@" if $@;
    unless ($input) {
        warn "$0: file '$infile' could not be opened for reading, skipping...";
        next;
    }

    foreach my $doc ($input->findnodes( '//doc' )) {
        my $docid = $doc->getAttribute( 'docid' );
        unless ($Opts{output}) {
            my $file = $docid;
            $file =~ s{/}{_}xg;
            $file = "$Opts{prefix}$file.txt";
            if ((! $Opts{overwrite}) && -e $file) {
                warn "$0: target file '$file' exists for docid '$docid', skipping...";
                next;
            }
            open $outfh, '>', $file  or do {
                warn "$0: could not write file '$file' for docid '$docid', skipping: $!";
                next;
            };
            print STDERR "writing to '$file'...\n";
        }

        my $id = 0;
        my $ordered = 1;
        foreach my $seg ($doc->findnodes( './/seg' )) {
            my $segid = $seg->getAttribute( 'id' );
            if ($ordered && ($segid != ++$id)) {
                warn "Warning, segments do not appear in order at id $segid in docid '$docid'";
                $ordered = 0;
            }
            my $line = $seg->to_literal;
            $line =~ s{^ \s+}{}x;
            $line =~ s{\s+ $}{}x;
            $line =~ s{\n}{  }xg;
            print {$outfh}  "$line\n";
        }
        close $outfh  unless $Opts{output};
    }
}
close $outfh  if $Opts{output};

exit;


sub usage
{
    my $msg = shift;
    print "$msg\n" if defined $msg;

    print <<"EOT";
Usage: mteval2txt.pl [OPTIONS] FILE [...]

NOTE: This script relies on the libxml2 system library and the Perl package
XML::LibXML.

Mandatory arguments to long options are mandatory for short options too.
  -f, --overwrite     overwrite existing output files
  -o, --output FILE,  write all segments to one flat file
    -a, --flat FILE     (mutually exclusive with '-p')
  -p, --prefix STR    prepend STR to the path of output files
                        (mutually exclusive with '-o')

  -h, --help     display this help and exit
  -V, --version  output version information and exit
EOT
    exit( defined $msg ? 1 : 0 );
}
