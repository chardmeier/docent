#! /bin/bash
#
# convert NIST-XML into MMAX with POS level annotation
# requires xmlstarlet!
# requires hunpos-tag (in your path)
#

if [ $# -ne 3 ]
then
	echo "$0 xml directory hunpos-model" 1>&2
	exit 1
fi

xmlcorpus=$1
dir=$2
tagmodel=$3

# this gives the directory where the script is located
mmax_scriptdir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"


mmax_skeleton=${mmax_scriptdir}/mmax-skeleton
xml=`which xmlstarlet`
if [ ! -e "$xml" ]
then
    xml=`which xml`
fi
if [ ! -e "$xml" ]
then
    echo "cannot find xml starlet"
    exit 1
fi
hunpos=`which hunpos-tag`
if [ ! -e "$hunpos" ]
then
    echo "cannot find hunpos-tag!"
    exit 1
fi


#tokeniser="/usit/abel/u1/chm/WMT2013.en-fr/tokeniser/tokenizer.perl -l en"
tokeniser=cat
tagger="$hunpos $tagmodel"

if [ -e $dir ]
then
	echo "$dir already exists." 1>&2
	exit 1
fi

cp -r "$mmax_skeleton" "$dir"

remove_last_if_empty='
	FNR > 1 {
		print lastline;
	}
	{
		lastline = $0;
	}
	END {
		if(lastline != "")
			print lastline;
	}'

docids=(`$xml sel -t -m "//doc" -v "@docid" -n $xmlcorpus | gawk "$remove_last_if_empty"`)

transform='
	BEGIN {
		widx = 1;
		sidx = 0;

		print "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" >WORDS;
		print "<!DOCTYPE words SYSTEM \"words.dtd\">" >WORDS;
		print "<words>" >WORDS;

		print "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" >SENTENCES;
		print "<!DOCTYPE markables SYSTEM \"markables.dtd\">" >SENTENCES;
		print "<markables xmlns=\"www.eml.org/NameSpaces/sentence\">" >SENTENCES;

		print "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" >POS;
		print "<!DOCTYPE markables SYSTEM \"markables.dtd\">" >POS;
		print "<markables xmlns=\"www.eml.org/NameSpaces/pos\">" >POS;
	}

	{
		firstword = widx;
		for(i = 1; i <= NF; i++){
                        split($i,a,"/");                        
			print "<word id=\"word_" widx "\">" escape(a[1]) "</word>" >WORDS;
			print "<markable id=\"markable_" widx "\" span=\"word_" widx++ "\" tag=\"" a[2] "\" mmax_level=\"pos\" />"  >POS;
                }
		lastword = widx - 1;

		print "<markable mmax_level=\"sentence\" orderid=\"" sidx "\" id=\"markable_" sidx++ "\" span=\"word_" firstword "..word_" lastword "\" />" >SENTENCES;
	}

	END {
		print "</words>" >WORDS;
		print "</markables>" >SENTENCES;
		print "</markables>" >POS;
	}

	function escape(s) {
		gsub(/&/, "\\&amp;", s);
		gsub(/</, "\\&lt;", s);
		gsub(/>/, "\\&gt;", s);
		return s;
	}'

for ((i=0; i<${#docids[@]}; i++))
do
	id=${docids[$i]}
	name="`printf '%03d' $i`_`echo $id | sed 's/[/\\]/_/g'`"
	echo "$id => $name" 1>&2

	$xml sel -t -m "//doc[@docid='$id']//seg" -v 'normalize-space(.)' -n $xmlcorpus |
		gawk "$remove_last_if_empty" |
		$xml unesc |
		$tokeniser 2>/dev/null |
                sed "s/$/\n/" | tr ' ' "\n" | $tagger | tr "\t" '/' | tr "\n" ' ' | sed "s/  /\n/g" |
		gawk -v WORDS=$dir/Basedata/${name}_words.xml -v SENTENCES=$dir/markables/${name}_sentence_level.xml -v POS=$dir/markables/${name}_pos_level.xml "$transform"

	cat <<EOF >$dir/$name.mmax
<?xml version="1.0" encoding="UTF-8"?>
<mmax_project>
<words>${name}_words.xml</words>
<keyactions></keyactions>
<gestures></gestures>
</mmax_project>
EOF
done
