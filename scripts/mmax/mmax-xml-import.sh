#! /bin/bash
#
# convert NIST-XML into MMAX
# requires xmlstarlet!
#

if [ $# -ne 2 ]
then
	echo "$0 xml directory" 1>&2
	exit 1
fi

xmlcorpus=$1
dir=$2

# this gives the directory where the script is located
mmax_scriptdir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"


mmax_skeleton=${mmax_scriptdir}/mmax-skeleton
xml=`which xmlstarlet`
#tokeniser="/usit/abel/u1/chm/WMT2013.en-fr/tokeniser/tokenizer.perl -l en"
tokeniser=cat

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
		print "<markables xmlns=\"www.eml.org/NameSpaces/smtsentence\">" >SENTENCES;
	}

	{
		firstword = widx;
		for(i = 1; i <= NF; i++)
			print "<word id=\"word_" widx++ "\">" escape($i) "</word>" >WORDS;
		lastword = widx - 1;

		print "<markable mmax_level=\"smtsentence\" orderid=\"" sidx "\" id=\"markable_" sidx++ "\" span=\"word_" firstword "..word_" lastword "\" />" >SENTENCES;
	}

	END {
		print "</words>" >WORDS;
		print "</markables>" >SENTENCES;
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
		gawk -v WORDS=$dir/Basedata/${name}_words.xml -v SENTENCES=$dir/markables/${name}_smtsentence_level.xml "$transform"

	cat <<EOF >$dir/$name.mmax
<?xml version="1.0" encoding="UTF-8"?>
<mmax_project>
<words>${name}_words.xml</words>
<keyactions></keyactions>
<gestures></gestures>
</mmax_project>
EOF

#	cat <<EOF >$dir/markables/${name}_chunk_level.xml
#	<?xml version="1.0" encoding="UTF-8"?>
#	<!DOCTYPE markables SYSTEM "markables.dtd">
#	<markables xmlns="www.eml.org/NameSpaces/chunk" />
#EOF

#	cat <<EOF >$dir/markables/${name}_coref_level.xml
#	<?xml version="1.0"?>
#	<!DOCTYPE markables SYSTEM "markables.dtd">
#	<markables xmlns="www.eml.org/NameSpaces/coref" />
#EOF

#	cat <<EOF >$dir/markables/${name}_enamex_level.xml
#	<?xml version="1.0" encoding="UTF-8"?>
#	<!DOCTYPE markables SYSTEM "markables.dtd">
#	<markables xmlns="www.eml.org/NameSpaces/enamex" />
#EOF

#	cat <<EOF >$dir/markables/${name}_markable_level.xml
#	<?xml version="1.0" encoding="UTF-8"?>
#	<!DOCTYPE markables SYSTEM "markables.dtd">
#	<markables xmlns="www.eml.org/NameSpaces/markable" />
#EOF

#	cat <<EOF >$dir/markables/${name}_morph_level.xml
#	<?xml version="1.0" encoding="UTF-8"?>
#	<!DOCTYPE markables SYSTEM "markables.dtd">
#	<markables xmlns="www.eml.org/NameSpaces/morph" />
#EOF

#	cat <<EOF >$dir/markables/${name}_parse_level.xml
#	<?xml version="1.0" encoding="UTF-8"?>
#	<!DOCTYPE markables SYSTEM "markables.dtd">
#	<markables xmlns="www.eml.org/NameSpaces/parse" />
#EOF

#	cat <<EOF >$dir/markables/${name}_pos_level.xml
#	<?xml version="1.0" encoding="UTF-8"?>
#	<!DOCTYPE markables SYSTEM "markables.dtd">
#	<markables xmlns="www.eml.org/NameSpaces/pos" />
#EOF

#	cat <<EOF >$dir/markables/${name}_response_level.xml
#	<?xml version="1.0" encoding="UTF-8"?>
#	<!DOCTYPE markables SYSTEM "markables.dtd">
#	<markables xmlns="www.eml.org/NameSpaces/response" />
#EOF

#	cat <<EOF >$dir/markables/${name}_section_level.xml
#	<?xml version="1.0" ?>
#	<!DOCTYPE markables SYSTEM "markables.dtd">
#	<markables xmlns="www.eml.org/NameSpaces/section" />
#EOF
done
