#! /bin/bash
#
# convert conllu format to mmax
#
# USAGE: mmax-conll-import.sh mmaxdir < treebank.conllu
#
#------------------------------------------------------------------
# This script requires additional document markup!
# add lines in the following format to specify document boundaries:
#
#   ###C:begin document <docname>
#   ###C:end document
#
# other lines that start with '#' will be ignored
#------------------------------------------------------------------
#


if [ $# -ne 1 ]
then
	echo "$0 directory" 1>&2
	exit 1
fi
dir=$1

# this gives the directory where the script is located
mmax_scriptdir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"


mmax_skeleton=${mmax_scriptdir}/mmax-skeleton

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

	/^###C:begin document/ {
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
		if(LEMMA) {
			print "</markables>" >LEMMA;
			close(LEMMA);
		}
		if(UPOS) {
			print "</markables>" >UPOS;
			close(UPOS);
		}
		if(XPOS) {
			print "</markables>" >XPOS;
			close(XPOS);
		}
		if(FEATS) {
			print "</markables>" >FEATS;
			close(FEATS);
		}
		if(HEAD) {
			print "</markables>" >HEAD;
			close(HEAD);
		}
		if(DEPREL) {
			print "</markables>" >DEPREL;
			close(DEPREL);
		}
		if(DEPS) {
			print "</markables>" >DEPS;
			close(DEPS);
		}
		if(MISC) {
			print "</markables>" >MISC;
			close(MISC);
		}

		ENTITIES = sprintf(ENTITIES_FORMAT, docnr, docname);
		print "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" >ENTITIES;
		print "<!DOCTYPE markables SYSTEM \"markables.dtd\">" >ENTITIES;
		print "<markables xmlns=\"www.eml.org/NameSpaces/entity\">" >ENTITIES;
		LEMMA = sprintf(LEMMA_FORMAT, docnr, docname);
		print "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" >LEMMA;
		print "<!DOCTYPE markables SYSTEM \"markables.dtd\">" >LEMMA;
		print "<markables xmlns=\"www.eml.org/NameSpaces/lemma\">" >LEMMA;
		UPOS = sprintf(UPOS_FORMAT, docnr, docname);
		print "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" >UPOS;
		print "<!DOCTYPE markables SYSTEM \"markables.dtd\">" >UPOS;
		print "<markables xmlns=\"www.eml.org/NameSpaces/pos\">" >UPOS;
		XPOS = sprintf(XPOS_FORMAT, docnr, docname);
		print "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" >XPOS;
		print "<!DOCTYPE markables SYSTEM \"markables.dtd\">" >XPOS;
		print "<markables xmlns=\"www.eml.org/NameSpaces/xpos\">" >XPOS;
		FEATS = sprintf(FEATS_FORMAT, docnr, docname);
		print "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" >FEATS;
		print "<!DOCTYPE markables SYSTEM \"markables.dtd\">" >FEATS;
		print "<markables xmlns=\"www.eml.org/NameSpaces/feat\">" >FEATS;
		HEAD = sprintf(HEAD_FORMAT, docnr, docname);
		print "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" >HEAD;
		print "<!DOCTYPE markables SYSTEM \"markables.dtd\">" >HEAD;
		print "<markables xmlns=\"www.eml.org/NameSpaces/head\">" >HEAD;
		DEPREL = sprintf(DEPREL_FORMAT, docnr, docname);
		print "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" >DEPREL;
		print "<!DOCTYPE markables SYSTEM \"markables.dtd\">" >DEPREL;
		print "<markables xmlns=\"www.eml.org/NameSpaces/deprel\">" >DEPREL;
		DEPS = sprintf(DEPS_FORMAT, docnr, docname);
		print "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" >DEPS;
		print "<!DOCTYPE markables SYSTEM \"markables.dtd\">" >DEPS;
		print "<markables xmlns=\"www.eml.org/NameSpaces/deps\">" >DEPS;
                MISC = sprintf(MISC_FORMAT, docnr, docname);
		print "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" >MISC;
		print "<!DOCTYPE markables SYSTEM \"markables.dtd\">" >MISC;
		print "<markables xmlns=\"www.eml.org/NameSpaces/misc\">" >MISC;

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

	/^###C:end document/ {
		end_sentence();
		docnr++;
		next;
	}

        ## ignore all other lines starting with #
	/^#/ {
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
		printf "<markable level=\"lemma\" id=\"markable_%d\" span=\"word_%d\" tags=\"%s\"/>\n", widx-1, widx, escape($3) >LEMMA;
		printf "<markable level=\"pos\" id=\"markable_%d\" span=\"word_%d\" tags=\"%s\"/>\n", widx-1, widx, escape($4) >UPOS;
		printf "<markable level=\"xpos\" id=\"markable_%d\" span=\"word_%d\" tags=\"%s\"/>\n", widx-1, widx, escape($5) >XPOS;
		printf "<markable level=\"feat\" id=\"markable_%d\" span=\"word_%d\" tags=\"%s\"/>\n", widx-1, widx, escape($6) >FEATS;
		printf "<markable level=\"head\" id=\"markable_%d\" span=\"word_%d\" tags=\"%s\"/>\n", widx-1, widx, escape($7) >HEAD;
		printf "<markable level=\"deprel\" id=\"markable_%d\" span=\"word_%d\" tags=\"%s\"/>\n", widx-1, widx, escape($8) >DEPREL;
		printf "<markable level=\"deps\" id=\"markable_%d\" span=\"word_%d\" tags=\"%s\"/>\n", widx-1, widx, escape($9) >DEPS;
		printf "<markable level=\"misc\" id=\"markable_%d\" span=\"word_%d\" tags=\"%s\"/>\n", widx-1, widx, escape($10) >MISC;

		widx++;
	}

	END {
		print "</words>" >WORDS;
		print "</markables>" >SENTENCES;
		print "</markables>" >ENTITIES;
		print "</markables>" >LEMMA;
		print "</markables>" >UPOS;
		print "</markables>" >XPOS;
		print "</markables>" >FEATS;
		print "</markables>" >HEAD;
		print "</markables>" >DEPREL;
		print "</markables>" >DEPS;
		print "</markables>" >MISC;
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


cp -r $mmax_skeleton $dir
mkdir -p $dir/Basedata $dir/markables

gawk -v WORDS_FORMAT="$dir/Basedata/%03d_%s_words.xml" \
	-v SENTENCES_FORMAT="$dir/markables/%03d_%s_sentence_level.xml" \
	-v ENTITIES_FORMAT="$dir/markables/%03d_%s_entity_level.xml" \
	-v LEMMA_FORMAT="$dir/markables/%03d_%s_lemma_level.xml" \
	-v UPOS_FORMAT="$dir/markables/%03d_%s_pos_level.xml" \
	-v XPOS_FORMAT="$dir/markables/%03d_%s_xpos_level.xml" \
	-v FEATS_FORMAT="$dir/markables/%03d_%s_feats_level.xml" \
	-v HEAD_FORMAT="$dir/markables/%03d_%s_head_level.xml" \
	-v DEPREL_FORMAT="$dir/markables/%03d_%s_deprel_level.xml" \
	-v DEPS_FORMAT="$dir/markables/%03d_%s_deps_level.xml" \
	-v MISC_FORMAT="$dir/markables/%03d_%s_misc_level.xml" \
	-v ENTITY_FORMAT="$dir/markables/%03d_%s_entity_level.xml" \
	-v MMAX_FORMAT="$dir/%03d_%s.mmax" "$transform"

