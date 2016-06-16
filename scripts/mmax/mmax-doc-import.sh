#! /bin/bash

chunksize=50

if [ $# -ne 3 ]
then
	echo "$0 docstart prefix directory" 1>&2
	exit 1
fi

docstart=$1
prefix=$2
dirprefix=$3

mmax_skeleton=/usit/abel/u1/chm/mmax-import/mmax-skeleton

if [ -e $dirprefix.001 ]
then
	echo "$dirprefix.001 already exists." 1>&2
	exit 1
fi

start=(`cat $docstart`)

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
	}

	{
		firstword = widx;
		for(i = 1; i <= NF; i++)
			print "<word id=\"word_" widx++ "\">" escape($i) "</word>" >WORDS;
		lastword = widx - 1;

		print "<markable mmax_level=\"sentence\" orderid=\"" sidx "\" id=\"markable_" sidx++ "\" span=\"word_" firstword "..word_" lastword "\" />" >SENTENCES;
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

for ((i=1; i<=${#start[@]}; i++))
do
	if [ $(($i % $chunksize)) -eq 1 ]
	then
		dir=$dirprefix.`printf '%03d' $(($i / $chunksize))`
		cp -r $mmax_skeleton $dir
	fi

	name=${prefix}_`printf '%07d' ${start[$i-1]}`

	{
		lineno=${start[$i-1]}
		echo -n "Document $i starts at line $lineno " 1>&2
		while [ $i -eq ${#start[@]} -o $lineno -lt "0${start[$i]}" ] && read -r
		do
			echo "$REPLY"
			let lineno++
		done
		echo "and ends at line $(($lineno - 1)) " 1>&2
	} |
		gawk -v WORDS=$dir/Basedata/${name}_words.xml -v SENTENCES=$dir/markables/${name}_sentence_level.xml "$transform"

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
#
#	cat <<EOF >$dir/markables/${name}_coref_level.xml
#	<?xml version="1.0"?>
#	<!DOCTYPE markables SYSTEM "markables.dtd">
#	<markables xmlns="www.eml.org/NameSpaces/coref" />
#EOF
#
#	cat <<EOF >$dir/markables/${name}_enamex_level.xml
#	<?xml version="1.0" encoding="UTF-8"?>
#	<!DOCTYPE markables SYSTEM "markables.dtd">
#	<markables xmlns="www.eml.org/NameSpaces/enamex" />
#EOF
#
#	cat <<EOF >$dir/markables/${name}_markable_level.xml
#	<?xml version="1.0" encoding="UTF-8"?>
#	<!DOCTYPE markables SYSTEM "markables.dtd">
#	<markables xmlns="www.eml.org/NameSpaces/markable" />
#EOF
#
#	cat <<EOF >$dir/markables/${name}_morph_level.xml
#	<?xml version="1.0" encoding="UTF-8"?>
#	<!DOCTYPE markables SYSTEM "markables.dtd">
#	<markables xmlns="www.eml.org/NameSpaces/morph" />
#EOF
#
#	cat <<EOF >$dir/markables/${name}_parse_level.xml
#	<?xml version="1.0" encoding="UTF-8"?>
#	<!DOCTYPE markables SYSTEM "markables.dtd">
#	<markables xmlns="www.eml.org/NameSpaces/parse" />
#EOF
#
#	cat <<EOF >$dir/markables/${name}_pos_level.xml
#	<?xml version="1.0" encoding="UTF-8"?>
#	<!DOCTYPE markables SYSTEM "markables.dtd">
#	<markables xmlns="www.eml.org/NameSpaces/pos" />
#EOF
#
#	cat <<EOF >$dir/markables/${name}_response_level.xml
#	<?xml version="1.0" encoding="UTF-8"?>
#	<!DOCTYPE markables SYSTEM "markables.dtd">
#	<markables xmlns="www.eml.org/NameSpaces/response" />
#EOF
#
#	cat <<EOF >$dir/markables/${name}_section_level.xml
#	<?xml version="1.0" ?>
#	<!DOCTYPE markables SYSTEM "markables.dtd">
#	<markables xmlns="www.eml.org/NameSpaces/section" />
#EOF
done
