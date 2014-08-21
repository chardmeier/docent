#!/usr/bin/env perl

open F,">rhymes.txt";

while (<>){
    if (/<title>(.*)<\/title>/){
	$lex = $1;
	$lang = 'en';
    }
    if (/===Pronunciation===/){
	while (<>){
	    last unless (/^\*/);
	    s/&quot;/"/gs;
	    if (/\{\{IPA\|.*lang=([a-z]+)/){
		$lang = $1;
	    }
	    if (/\{\{IPA\|.*(\/[^\}]*\/)/){
		print "$lang\t$lex\t$1\n";
	    }
	    if (/\{\{rhymes\|.*lang=([a-z]+)/){
		$lang = $1;
	    }
	    if (/\{\{rhymes\|([^\}\|]*)[\}\|]/){
		print F "$lang\t$lex\t$1\n";
	    }
	}
    }
}

close F;
