#!/bin/bash

function usage()
{
    cat >&2 <<"EOF"
./build-boost.sh [OPTIONS]-c CODEDIR [-b BUILDPREFIX] [-j JOBS]
or from Boost code directory: $DOCENTDIR/build-boost.sh []
Build the Boost libraries.

OPTIONS:
  -b BUILDPREFIX  prefix where to build Boost (will contain directories 'include'
          and 'lib') (default: environment variable BOOST_ROOT if available)
  -c CODEDIR      location of the unpacked Boost source code
          (default: current directory)
  -j JOBS         build JOBS targets simultaneously
          (default: number of processor cores on your machine)

  -h  display this help and exit
EOF
    exit 130
}

Code="$PWD"
NProc=$(nproc)
while getopts "b:c:j:-h" arg; do
    case $arg in
        b)  BOOST_ROOT="$OPTARG" ;;
        c)  Code="$OPTARG" ;;
        j)  NProc="$OPTARG" ;;

        -)  break ;;
        h|\?)  usage ;;
        *)  echo >&2 "Internal error: unhandled argument, please report: $arg"; exit 10
    esac
done
shift $((OPTIND-1))

if [[ -z "$BOOST_ROOT" ]]; then
    echo >&2 "$0: You must set the BOOST_ROOT variable or use argument '-b' to specify the installation target."
    usage
fi
if [[ "$Code" != */[Bb]oost* ]]; then
    if [[ ! -f "$Code/Jamroot" ]]; then
        echo >&2 "$0: ERROR: There is no file 'Jamroot' in '$Code', that does not seem to be the Boost code directory."
        exit 1
    fi
    echo >&2 "$0: Warning: Strange, the directory name '$Code' does not contain the string 'boost'."
    echo >&2 "It might not be the Boost code directory. I'll try the build, but be prepared for it to fail."
fi

set -e

B2Args=(
    --prefix="$BOOST_ROOT"
    --libdir="$BOOST_ROOT/lib"
    -j ${NProc:-1}
    link=static,shared threading=multi
    install
)

mkdir -p "$BOOST_ROOT/lib"
[[ -d "$BOOST_ROOT/lib64" ]] || ln -s lib "$BOOST_ROOT/lib64"

cd "$Code/"
./bootstrap.sh --prefix="$BOOST_ROOT"
./b2 --layout=tagged "${B2Args[@]}"
./b2                 "${B2Args[@]}"
