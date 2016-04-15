
# quick and dirty phrase table inversion ....


while (<>){
    chomp;
    @a=split(/ \|\|\| /);
    @s=split(/\s/,$a[2]);
    @w=split(/\s/,$a[3]);
    @f=split(/\s/,$a[4]);

    # swap phrase pair, scores, frequencies
    ($a[0],$a[1]) = ($a[1],$a[0]);
    ($s[0],$s[1],$s[2],$s[3]) = ($s[2],$s[3],$s[0],$s[1]);
    ($f[1],$f[0]) = ($f[0],$f[1]);

    # swap word alignments
    foreach my $i (0..$#w){
	$w[$i] = join('-',reverse split(/\-/,$w[$i]));
    }

    $a[2] = join(' ',@s);
    $a[3] = join(' ',@w);
    $a[4] = join(' ',@f);

    print join(' ||| ',@a);
    print "\n";
}


