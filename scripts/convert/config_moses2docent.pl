#!/usr/bin/perl
#
# convert a moses configuration file to docent format
# TODO:
# - command line arguments to control docent-specific parameters
#

print '<?xml version="1.0" ?>
<docent>
<random>185952804</random>
<state-generator>
	<initial-state type="monotonic"/>
	<operation type="change-phrase-translation" weight=".8"/>
	<operation type="swap-phrases" weight=".1">
		<p name="swap-distance-decay">.5</p>
	</operation>
	<operation type="resegment" weight=".1">
		<p name="phrase-resegmentation-decay">.1</p>
	</operation>
</state-generator>
<search algorithm="simulated-annealing">
	<p name="max-steps">100000</p>
	<p name="schedule">hill-climbing</p>
	<p name="hill-climbing:max-rejected">10000</p>
</search>
<models>
<model type="geometric-distortion-model" id="Distortion0">
<p name="distortion-limit">20</p>
</model>
';

my %models = (Distortion0 => 2);
my $weightSection = 0;

while (<>){

    unless ($weightSection){
	if (/^PhraseDictionary.* name=(\S+) num-features=(\S+) path=(\S+) /){
	    $models{$1} = $2;
	    print '<model type="phrase-table" id="',$1,'">',"\n";
	    print '<p name="file">',$3,'</p>',"\n";
	    print '<p name="nscores">',$2,'</p>',"\n";
	    if (-e $3.'.binphr.srctree.wa'){
		print '<p name="load-alignments">true</p>',"\n";
	    }
	    print '</model>',"\n";
	}
	elsif (/^WordPenalty/){
	    print '<model type="word-penalty" id="WordPenalty0"/>',"\n";
	    $models{WordPenalty0} = 1;
	}
	elsif (/^PhrasePenalty/){
	    print '<model type="phrase-penalty" id="PhrasePenalty0"/>',"\n";
	    $models{PhrasePenalty0} = 1;
	}
	elsif (/^UnknownWordPenalty/){
	    print '<model type="oov-penalty" id="UnknownWordPenalty0"/>',"\n";
	    $models{UnknownWordPenalty0} = 1;
	}
	elsif (/^KENLM.* name=(\S+) .*path=(\S+) /){
	    $models{$1} = 1;
	    print '<model type="ngram-model" id="',$1,'">',"\n";
	    print '<p name="lm-file">',$2,'</p>',"\n";
	    print '</model>',"\n";
	}
	elsif (/\[weight\]/){
	    print '</models>',"\n";
	    print '<weights>',"\n";
	    $weightSection = 1;
	}
    }

    if ($weightSection){
	if (/^(\S+)\=\s*([0-9\-].*)$/){
	    my $model = $1;
	    my @scores = split(/\s+/,$2);
	    if (exists $models{$model}){
		if ($model eq 'Distortion0'){
		    push (@scores,'1e30');
		}
		if ($#scores){
		    foreach (0..$#scores){
			print '<weight model="',$model,'" score="',$_,'">';
			print $scores[$_],'</weight>',"\n";
		    }
		}
		else{
		    print '<weight model="',$model,'">',$scores[0],'</weight>',"\n";
		}
	    }
	}
    }
}

print '</weights>',"\n" if ($weightSection);
print '</docent>',"\n";
