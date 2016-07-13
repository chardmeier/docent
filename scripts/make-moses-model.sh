#!/bin/bash

function usage()
{
    cat >&2 <<"EOF"
./make-moses-model.sh [OPTIONS] BASENAME [SRCLANG] [TGTLANG]
Create a Moses model in the current directory, with special focus on
the phrase table for use in Docent. No tuning is carried out; please start
that yourself if you want to use this model "productively" with Moses.

Subdirectories 'corpus', 'lm', 'tm', truecaser', and 'training'
will be created (and clobbered if existing, so be careful not to run
somwhere where you already have a model).

OPTIONS:
  -f NUM  first step to carry out
  -l NUM  last step to carry out, where
          1: tokenise corpus
		  2: truecase corpus
		  3: generate language model
		  4: binarise language model
		  5: clean corpus (remove segments > 80 words)
		  6: train translation model
		  7: move translation model out of training directory
		  8: binarise phrase table
  -j NUM  number of cores to parallelise on; default: $NCores

  -d DIR  search path to CreateProbingPT;
          default: $Docent
  -m DIR  search path to Moses' binaries;
          default: $Moses
  -s DIR  search path to Moses' scripts;
          default: $Scripts
  -t DIR  search path to tools (mgzia/GIZA++, mkcls);
          default: $Tools

  -h  display this help and exit
EOF
    exit 130
}

Docent=~/src/docent/DEBUG
Moses=~/src/mosesdecoder/bin
Scripts=~/src/mosesdecoder/scripts
Tools=~/src/mosesdecoder/tools

NCores=$(nproc)

First=1
Last=0
while getopts "d:f:j:l:m:s:t:-h" arg; do
    case $arg in
        f)  First=$OPTARG ;;
		j)  NCores=$OPTARG ;;
        l)  Last=$OPTARG ;;

        d)  Docent="$OPTARG" ;;
        m)  Moses="$OPTARG" ;;
        s)  Scripts="$OPTARG" ;;
        t)  Tools="$OPTARG" ;;

        -)  break ;;
        h|\?)  usage ;;
        *)  echo >&2 "Internal error: unhandled argument, please report: $arg"
            exit 10
    esac
done
shift $((OPTIND-1))

export Basename="$1"
export Sl=${2:-de}
export Tl=${3:-en}
if [[ ! -f corpus/$Basename$Sl ]]; then
    if [[ -f corpus/$Basename.$Sl ]]; then
        Basename+=.
    else
        echo >&2 "$0: Can't find '$Basename$Sl' or '$Basename.$Sl' in 'corpus/'"
        exit 1
    fi
fi
if [[ ! -f corpus/$Basename$Tl ]]; then
    echo >&2 "$0: Can't find '$Basename$Tl'"
    exit 1
fi

set -e

(( Step = 0 ))

(( Step++ )) ## 1
if (( Step >= First )); then
	## tokenise
	for l in $Sl $Tl; do
		echo "---- $l ----"
		if [[ $l != zh ]]; then
			if [[ -f corpus/$Basename$l.tok ]]; then
				ln -fs $Basename$l.tok corpus/tokenised.$l
			else
				$Scripts/tokenizer/tokenizer.perl -l $l < corpus/$Basename$l > corpus/tokenised.$l
			fi
		fi
	done
fi
if (( Last && Step >= Last )); then exit 0; fi

(( Step++ )) ## 2
if (( Step >= First )); then
	## truecase
	for l in $Sl $Tl; do
		echo "---- $l ----"
		if [[ -f corpus/$Basename$l.seg ]]; then
			ln -fs $Basename$l.seg corpus/true.$l
		elif [[ $l == zh ]]; then
			ln -fs $Basename$l corpus/true.$l
		else
			mkdir -p truecaser
			$Scripts/recaser/train-truecaser.perl --model truecaser/$l --corpus corpus/tokenised.$l
			$Scripts/recaser/truecase.perl        --model truecaser/$l <        corpus/tokenised.$l > corpus/true.$l
		fi
	done
fi
if (( Last && Step >= Last )); then exit 0; fi

(( Step++ )) ## 3
if (( Step >= First )); then
	#### language model
	mkdir -p lm
	$Moses/lmplz -o 3 < corpus/true.$Tl > lm/$Tl.arpa
fi
if (( Last && Step >= Last )); then exit 0; fi

(( Step++ )) ## 4
if (( Step >= First )); then
	## binarise LM
	$Moses/build_binary lm/$Tl.arpa lm/$Tl.blm
fi
if (( Last && Step >= Last )); then exit 0; fi

(( Step++ )) ## 5
if (( Step >= First )); then
	#### translation model
	## clean corpus to entries of max 80 words
	$Scripts/training/clean-corpus-n.perl corpus/true $Sl $Tl corpus/clean 1 80 corpus/clean_lines-retained
fi
if (( Last && Step >= Last )); then exit 0; fi

(( Step++ )) ## 6
if (( Step >= First )); then
	## train TM
	echo >&2 "Training model on ${NCores:-1 (default)} cores..."
	time nice $Scripts/training/train-model.perl \
		-external-bin-dir $Tools \
		-alignment grow-diag-final-and \
		-corpus $PWD/corpus/clean \
		-root-dir train \
		-f $Sl -e $Tl \
		-lm 0:3:$PWD/lm/$Tl.blm:8 \
		-cores ${NCores:=1}
fi
if (( Last && Step >= Last )); then exit 0; fi

(( Step++ )) ## 7
if (( Step >= First )); then
	if [[ -d tm ]]; then
		if [[ -d tm~ ]]; then
			rm -r tm~
		fi
		mv tm tm~
	fi
	mv train/model tm
	rmdir train || true
	sed -i~ -r 's_/train/model/_/tm/_' tm/moses.ini
fi
if (( Last && Step >= Last )); then exit 0; fi

(( Step++ )) ## 8
if (( Step >= First )); then
	$Docent/CreateProbingPT \
		tm/phrase-table.gz \
		tm/phrase-table \
		4

	sed -i~ -r 's_PhraseDictionary\w+_ProbingPT_' tm/moses.ini
fi
if (( Last && Step >= Last )); then exit 0; fi
