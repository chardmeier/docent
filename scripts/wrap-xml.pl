#!/usr/bin/perl

my $textfile = shift(@ARGV);
if ($textfile=~/\.gz/){
    open F,"gzip -cd <$textfile |" || die "cannot read from $textfile";
}
else{
    open F,"<$textfile" || die "cannot read from $textfile";
}
my @text = <F>;
close F;

while (<>){
    if (/<seg /){
        my $sent = shift(@text);
        $sent=~s/&/&amp;/g;
        $sent=~s/</&lt;/g;
        $sent=~s/>/&gt;/g;
        chomp($sent);
        s/(<seg .*?>)[^>]*(<\/seg>)/$1$sent$2/;
    }
    print ;
}
