#!/usr/bin/env bash

function die()
{
    local status=$1 l
    shift
    echo >&2 "$0: $1"
    shift
    for line; do echo >&2 "$line"; done
    exit $status
}


function usage()
{
    cat >&2 <<"EOF"
mteval-moses.sh [OPTIONS] [FILE]
Pre-translate an MTeval source set into a test set, using Moses.
This is a support script for Docent.

OPTIONS:
  -m ARGS  arguments to be passed to Moses (to pass several arguments,
           repeat, or put everything into a quoted string)
  -S DIR   search path to the Docent scripts directory
  -t  report segmentation, as needed for initial translations for Docent
  -u  update - write translation back into the input file

  -h  display this help and exit
EOF
    exit 130
}


MosesOpts=()
while getopts "m:S:tu-h" arg; do
    case $arg in
        m)  MosesOpts+=($OPTARG) ;;
        S)  Scripts="${OPTARG%%/}/" ;;
        t)  MosesOpts+=('-t') ;;
        u)  Update='-u' ;;

        -)  break ;;
        h|\?)  usage ;;
        *)  die 10 "Internal error: unhandled argument, please report: $arg"
    esac
done
shift $((OPTIND-1))

if [[ -n $Scripts ]]; then
    if [[ ! -f ${Scripts}mteval2txt.pl || ! -f ${Scripts}mteval-insert-segs.pl ]]; then
        die 1 "Make sure that the directory given by option '-s' contains both" \
            "mteval2txt.pl and mteval-insert-segs.pl"
    fi
else
    for s in mteval2txt.pl mteval-insert-segs.pl; do
        if [[ -z $(which $s) ]]; then
            die 1 "$s: not found on path," \
                "please point to the Docent scripts with option '-S'"
        fi
    done
fi

Input="${1:--}"
if [[ $Input = - ]]; then
    Input="$(mktemp --tmpdir mteval-moses.XXXXX.xml)" || die $? "error creating tempfile"
    cat - > "$Input" || die $? "error writing to tempfile"
    DeleteTmpInput=1
    Output="${2:--}"
else
    Output="${2:-}"
    if [[ -z $Output && -n $Update ]]; then
        ## only if output entirely unspecified -- override with '-'
        Output="$Input"
    fi
fi

mteval2txt.pl -o - "$Input" |
moses "${MosesOpts[@]}" |
mteval-insert-segs.pl "$Input" $Update -o "$Output"

Status=(${PIPE_STATUS[@]})

if (( $DeleteTmpInput )) && [[ $Input = *mteval-moses.*.xml ]]; then
    rm -f "$Input"
fi

(( ${Status[0]} == 0 )) || die ${Status[0]} "extraction error: ${Status[0]}"
(( ${Status[1]} == 0 )) || die ${Status[1]} "Moses error: ${Status[1]}"
(( ${Status[2]} == 0 )) || die ${Status[2]} "reinsertion error: ${Status[2]}"
exit 0
