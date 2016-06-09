#!/usr/bin/env perl
package txt2mteval;
use strict;
use warnings;
use open qw(:std :utf8);

our $VERSION = 0.01;

use Getopt::Long qw(:config no_ignore_case bundling);


my %Opts = (
    mode    => 'srcset',
    output  => '-',
    setid   => 'default',
    srclang => 'any',
);
my @Files;
ARGV: while (@ARGV) {
    GetOptions( \%Opts
        , 'output|o=s'

        , 'refset|r=s'
        , 'srcset|s=s'
        , 'tstset|t=s'

        , 'setid|i=s'
        , 'id|refid|sysid=s'

        , 'help|h|?' => \&usage
        , 'version|V' => sub { print "txt2mteval.pl v$VERSION\n"; exit }
    ) or usage();
    while ((scalar @ARGV) && ($ARGV[0] !~ m{^-.+}x)) {
        push @Files, shift;
    }
}

{   my $count = 0;
    if (exists $Opts{srcset}) {
        ++$count;
        $Opts{srclang} = $Opts{srcset} || 'any';
        undef $Opts{srcset};
    } else {
        foreach my $arg (qw(refset tstset)) {
            next unless exists $Opts{ $arg };
            ++$count;
            $Opts{mode} = $arg;
            $Opts{$arg} ||= 'any-any';
            (@Opts{ qw(srclang trglang) }) = $Opts{$arg} =~ m{^ (.+?) -+ (.+) $}x;
            undef $Opts{ $arg };
        }
    }
    usage( "These arguments are mutually exclusive: 'refset'/'srcset'/'tstset'" ) if $count > 1;
}
usage( "specify at least one filename" ) unless @Files;

eval { require XML::LibXML };
die "$0: missing Perl dependency: XML::LibXML\n" if $@;

my $xml = XML::LibXML::Document->new( '1.0', 'utf-8' );
$xml->setDocumentElement(
    my $root = XML::LibXML::Element->new( 'mteval' )
);
my $set = $root->appendChild( XML::LibXML::Element->new( $Opts{mode} ) );
foreach my $attr (qw(setid srclang trglang)) {
    $set->setAttribute( $attr => $Opts{ $attr } )  if exists $Opts{ $attr };
}

my %Docs;
foreach my $file (@Files) {
    my $docid = $file;
    if ((! -r $file) && ($file =~ m{^ (\S+) = (.+) $}x)) {
        $docid = $1;
        $file  = $2;
    }
    unless (-s $file && ! -d $file) {
        warn "$0: file '$file' not found or empty, skipping...";
        next;
    }
    die "$0: duplicate document ID '$docid' with file '$file'!\n"  if exists $Docs{ $docid };
    $Docs{ $docid } = $file;

    my $doc = $set->appendChild( XML::LibXML::Element->new( 'doc' ) );
    $doc->setAttribute( docid => $docid );
    open my $fh, '<', $file  or do {
        warn "$0: file '$file' could not be opened, skipping: $!";
        next;
    };
    my $id = 0;
    while (my $line = <$fh>) {
        chomp $line;
        next unless length $line;
        my $seg = $doc->appendChild( XML::LibXML::Element->new( 'seg' ) );
        $seg->setAttribute( id => ++$id );
        $seg->appendText( $line );
    }
    close $fh;
}

my $fh;
if ($Opts{output} eq '-') {
    $fh = \*STDOUT;
} else {
    open $fh, '>', $Opts{output}
        or die "$0: unable to write to '$Opts{output}': $!\n";
}
binmode $fh;
print {$fh} $xml->toString(2);
close $fh;

exit;


sub usage
{
    my $msg = shift;
    print "$msg\n" if defined $msg;

    print <<"EOT";
Usage: txt2mteval.pl [OPTIONS] FILE [...]
  or:  txt2mteval.pl [OPTIONS] -o OUTPUT.xml FILE [...]

NOTE: This script relies on the libxml2 system library and the Perl package
XML::LibXML.

Mandatory arguments to long options are mandatory for short options too. The
arguments LANGS must have the form 'SRCLANG-TRGLANG' (either may be 'any').
  -o, --output FILE   name of the file to which the XML result is written

  -r, --refset LANGS  produce a 'refset' MT-result document, setting the
      attributes 'srclang' and 'trglang' according to LANG
  -s, --srcset LANG   produce a 'srcset' input document with the attribute
      'srclang' set to LANG (default, with LANG='any')
  -t, --tstset LANGS  produce a 'tstset' MT-result document, setting the
      attributes 'srclang' and 'trglang' according to LANG

  -i, --setid ID      set the attribute 'setid' of the {ref,src,tst}set node
      (default: 'default')

  -h, --help     display this help and exit
  -V, --version  output version information and exit
EOT

    exit( defined $msg ? 1 : 0 );
}
