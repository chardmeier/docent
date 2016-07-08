#!/bin/bash

function usage()
{
    cat >&2 <<"EOF"
./build-boost.sh -c CODEDIR [-b BUILDPREFIX]
or from Boost code directory: $DOCENTDIR/build-boost.sh [-b BUILDPREFIX]
Build the Boost libraries.
CODEDIR defaults to the current working directory, BUILDPREFIX is taken
from the $BOOST_ROOT environment variable, if it is already set.
EOF
    exit 130
}

Code="$PWD"
while getopts "b:c:-h" arg; do
    case $arg in
        b)  BOOST_ROOT="$OPTARG" ;;
        c)  Code="$OPTARG" ;;

        -)  break ;;
        h|\?)  usage ;;
        *)  die -10 "Internal error: unhandled argument, please report: $arg"
    esac
done
shift $((OPTIND-1))

if [[ -z "$BOOST_ROOT" ]]; then
    echo >&2 "$0: You must set the BOOST_ROOT variable or use argument '-b' to specify the installation target."
    usage
fi
if [[ ! "$Code" =~ */[Bb]oost* ]]; then
    if [[ ! -f "$Code/Jamroot" ]]; then
        echo >&2 "$0: ERROR: There is no file 'Jamroot' in '$Code', that does not seem to be the Boost code directory."
        exit 1
    fi
    echo >&2 "$0: Warning: Strange, the directory name '$Code' does not contain the string 'boost'."
    echo >&2 "It might not be the Boost code directory. I'll try the build, but be prepared for it to fail."
fi

set -e

mkdir -p  "$BOOST_ROOT/lib"
ln -s lib "$BOOST_ROOT/lib64"

export LIBRARY_PATH=$BOOST_ROOT/lib${LIBRARY_PATH:+:$LIBRARY_PATH}
export CPATH=$BOOST_ROOT/include${CPATH:+:$CPATH}
export LD_LIBRARY_PATH=$BOOST_ROOT/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
cd "$Code/"
./bootstrap.sh --prefix=$BOOST_ROOT
./b2 --prefix=$BOOST_ROOT --libdir=$BOOST_ROOT/lib --layout=tagged link=static,shared threading=multi install
./b2 --prefix=$BOOST_ROOT --libdir=$BOOST_ROOT/lib                 link=static,shared threading=multi install
