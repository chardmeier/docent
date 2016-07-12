#!/bin/bash

function usage()
{
    cat >&2 <<"EOF"
./build-docent.sh [OPTIONS]
Build Docent and its dependencies.

OPTIONS:
  -b BOOST_ROOT  location of a non-system Boost installation
  -c CODEDIR     location of Docent source code (default: current dir)
  -j JOBS        build JOBS targets simultaneously
          (default: number of processor cores on your machine)
  -m MODE        'DEBUG' (default) or 'RELEASE'
  -t BUILDDIR    where to put intermediate and compiled files
          (default: CODEDIR/MODE)

  -h  display this help and exit
EOF
    exit 130
}

Code="$PWD"
Mode='DEBUG'
NProc="$(nproc)"
while getopts "b:c:j:m:t:-h" arg; do
    case $arg in
        b)  export BOOST_ROOT="$OPTARG" ;;
        c)  Code="$OPTARG" ;;
        j)  NProc="$OPTARG" ;;
        m)  Mode="$OPTARG" ;;
        t)  Target="$OPTARG" ;;

        -)  break ;;
        h|\?)  usage ;;
        *)  die -10 "Internal error: unhandled argument, please report: $arg"
    esac
done
shift $((OPTIND-1))

Code="$(readlink -f "$Code")"
if [[ -z "$Target" ]]; then
    Target="$Code/$Mode"
fi

set -e

mkdir -p "$Target"
cd       "$Target"

## build
cmake -DCMAKE_BUILD_TYPE=$Mode "$Code"
make -j ${NProc:-1} "$@"
