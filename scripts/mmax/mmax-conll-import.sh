#! /bin/bash

if [ $# -ne 1 ]
then
	echo "$0 directory" 1>&2
	exit 1
fi
dir=$1

mmax_skeleton=/opt/nobackup/ch/FBK/coherence/mmax-import/mmax-skeleton

if [ -e $dir ]
then
	echo "$dir already exists." 1>&2
	exit 1
fi

transform='
	BEGIN {
		docnr = 0;
		firstword = -1;
	}

	/^#begin document/ {
		docname = $3;

		if(WORDS) {
			print "</words>" >WORDS;
			close(WORDS);
		}
		WORDS = sprintf(WORDS_FORMAT, docnr, docname);
		print "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" >WORDS;
		print "<!DOCTYPE words SYSTEM \"words.dtd\">" >WORDS;
		print "<words>" >WORDS;

		if(SENTENCES) {
			print "</markables>" >SENTENCES;
			close(SENTENCES);
		}
		SENTENCES = sprintf(SENTENCES_FORMAT, docnr, docname);
		print "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" >SENTENCES;
		print "<!DOCTYPE markables SYSTEM \"markables.dtd\">" >SENTENCES;
		print "<markables xmlns=\"www.eml.org/NameSpaces/sentence\">" >SENTENCES;

		if(ENTITIES) {
			print "</markables>" >ENTITIES;
			close(ENTITIES);
		}
		ENTITIES = sprintf(ENTITIES_FORMAT, docnr, docname);
		print "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" >ENTITIES;
		print "<!DOCTYPE markables SYSTEM \"markables.dtd\">" >ENTITIES;
		print "<markables xmlns=\"www.eml.org/NameSpaces/entity\">" >ENTITIES;

		wordsfile = WORDS;
		sub(/.*\//, "", wordsfile);

		MMAX = sprintf(MMAX_FORMAT, docnr, docname);
		print "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" >MMAX;
		print "<mmax_project>" >MMAX;
		printf "<words>%s</words>", wordsfile >MMAX;
		print "<keyactions></keyactions>" >MMAX;
		print "<gestures></gestures>" >MMAX;
		print "</mmax_project>" >MMAX;
		close(MMAX);

		widx = 1;
		sidx = 0;

		next;
	}

	/^#end document/ {
		end_sentence();
		docnr++;
		next;
	}

	/^$/ {
		end_sentence();
		next;
	}

	{
		in_sentence();
		printf "<word id=\"word_%d\">%s</word> <!-- %s -->\n", widx, escape($2), $NF >WORDS;
		printf "<markable level=\"entity\" id=\"markable_%d\" span=\"word_%d\" tags=\"%s\"/>\n", widx-1, widx, $NF >ENTITIES;
		widx++;
	}

	END {
		print "</words>" >WORDS;
		print "</markables>" >SENTENCES;
		print "</markables>" >ENTITIES;
	}

	function in_sentence() {
		if(firstword < 0)
			firstword = widx;
	}

	function end_sentence() {
		if(firstword < 0)
			return;
		lastword = widx - 1;
		print "<markable mmax_level=\"sentence\" orderid=\"" sidx "\" id=\"markable_" sidx++ "\" span=\"word_" firstword "..word_" lastword "\" />" >SENTENCES;
		firstword = -1;
	}

	function escape(s) {
		gsub(/&/, "\\&amp;", s);
		gsub(/</, "\\&lt;", s);
		gsub(/>/, "\\&gt;", s);
		return s;
	}'

mkdir -p $dir/Basedata $dir/markables

gawk -v WORDS_FORMAT="$dir/Basedata/%03d_%s_words.xml" \
	-v SENTENCES_FORMAT="$dir/markables/%03d_%s_sentence_level.xml" \
	-v ENTITIES_FORMAT="$dir/markables/%03d_%s_entity_level.xml" \
	-v MMAX_FORMAT="$dir/%03d_%s.mmax" "$transform"

