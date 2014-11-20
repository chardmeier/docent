#!env perl
#-*-perl-*-

while(<>){
    chomp;
    print join(' ',reverse(split(/\s+/))),"\n";
}
