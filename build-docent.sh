#!/bin/bash

function usage()
{
    cat >&2 <<"EOF"
./build-docent.sh [-c CODEDIR] [-t BUILDDIR]
Build Docent and its dependencies.
CODEDIR defaults to the current working directory,
BUILDDIR, to $CODEDIR/DEBUG
EOF
    exit 130
}

Code="$PWD"
while getopts "c:t:-h" arg; do
    case $arg in
        c)  Code="$OPTARG" ;;
        t)  Target="$OPTARG" ;;

        -)  break ;;
        h|\?)  usage ;;
        *)  die -10 "Internal error: unhandled argument, please report: $arg"
    esac
done
shift $((OPTIND-1))

if [[ -z "$Target" ]]; then
    Target="$Code/DEBUG"
fi

if [[ -z "$BOOST_ROOT" ]]; then
    echo >&2 "You must set the BOOST_ROOT variable to where you have installed Boost."
    exit 1
fi

export LIBRARY_PATH=$BOOST_ROOT/lib${LIBRARY_PATH:+:$LIBRARY_PATH}
export LD_LIBRARY_PATH=$BOOST_ROOT/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
export CPATH=$BOOST_ROOT/include${CPATH:+:$CPATH}

set -e

## patch Arabica spurious-rebuild bug
cd "$Code/external/arabica"
patch -p1 ../arabica-norebuild.patch
cd -

mkdir -p "$Target"
cd       "$Target"

## build
cmake -DCMAKE_BUILD_TYPE=DEBUG "$Code"
make "$@"
