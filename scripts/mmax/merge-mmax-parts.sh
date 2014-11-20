#! /bin/bash

if [ $# -lt 2 ]
then
	echo "$0 outdir indirs ..." 1>&2
	exit 1
fi

outdir=$1
shift

mmax_skeleton=/hltsrv3/hardmeier/coherence/mmax-import/mmax-skeleton

if [ -e $outdir ]
then
	echo "$outdir already exists." 1>&2
	exit 1
fi

cp -r $1 $outdir

transform_words='
	BEGIN {
		widx = 1;
		fidx = 1;

		started = 0;
	}

	$1 == "D" {
		if(started)
			next;
		else
			$3 = "SYSTEM";
	}

	$1 == "(words" {
		if(!started)
			print;
		started = 1;
		next;
	}

	$1 == "Aid" {
		$2 = "word_" widx;
	}

	$1 == ")word" {
		widx++;
	}

	$1 == ")words" {
		woffset = widx - 1;
		offsets[fidx++] = woffset;
		next;
	}

	{
		print;
	}

	END {
		print ")words";
		for(i = 1; i < fidx; i++)
			print offsets[i] > "/dev/stderr";
	}'

transform_markables='
	BEGIN {
		midx = 0;
		fidx = 0;
		fidx_in = 0;
		woffset = 0;

		started = 0;
	}

	ARGIND == 1 {
		offset[fidx_in++] = $1;
		next;
	}

	started && ($1 == "D" || $1 == "Axmlns") {
		next;
	}

	$1 == "D" {
		$3 = "SYSTEM";
	}

	$1 == "(markables" {
		if(!started)
			print;
		started = 1;
		next;
	}

	$1 == "Aid" {
		$2 = "markable_" midx;
	}

	$1 == "Aspan" {
		span = $2;
		gsub(/word_/, "WORD_", span);
		while(match(span, /WORD_([0-9]+)/, m)) {
			w = m[1];
			sub(/WORD_/, "", w);
			w = "word_" (w + woffset);
			sub(m[0], w, span);
		}
		$2 = span;
	}

	$1 == "Aorderid" {
		$2 = midx;
	}

	$1 == ")markable" {
		midx++;
	}

	$1 == ")markables" {
		woffset = offset[fidx++];
		next;
	}

	{
		print;
	}

	END {
		print ")markables";
	}'

tmp=transform.tmp

{ for i in "$@"; do xml pyx $i/Basedata/*_words.xml; done; } |
	gawk "$transform_words" 2>$tmp |
	xml depyx >$outdir/Basedata/`basename $1/Basedata/*_words.xml`

for i in $1/markables/*.xml
do
	level=`basename $i | sed 's/.*\(_[a-z]\+_level\.xml\)/\1/'`
	echo $level
	{ for j in "$@"; do xml pyx $j/markables/*$level; done; } |
		gawk "$transform_markables" $tmp - |
		xml depyx >$outdir/markables/`basename $i`
done
