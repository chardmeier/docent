#!/usr/bin/env bash

function usage()
{
    cat >&2 <<"EOF"
./build-docent.sh [OPTIONS] [TARGETS]
Build Docent and its dependencies.

OPTIONS:
  -b BOOST_ROOT  location of a non-system Boost installation
  -c CODEDIR     location of Docent source code (default: current dir)
  -D CMAKE_VAR   add a CMake cache entry (mind the space after '-D'!)
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
CMakeOptions=()
Mode='DEBUG'
NProc="$(nproc)"
while getopts "b:c:D:j:m:t:-h" arg; do
    case $arg in
        b)  CMakeOpts+=(-DBOOST_ROOT="$OPTARG") ;;
        c)  Code="$OPTARG" ;;
        D)  CMakeOptions+=(-D"$OPTARG") ;;
        j)  NProc="$OPTARG" ;;
        m)  Mode="$OPTARG" ;;
        t)  Target="$OPTARG" ;;

        -)  break ;;
        h|\?)  usage ;;
        *)  echo >&2 "Internal error: unhandled argument, please report: $arg"
            exit 10
    esac
done
shift $((OPTIND-1))

CMakeOptions+=(-DCMAKE_BUILD_TYPE="$Mode")
Code="$(readlink -f "$Code")"
if [[ -z "$Target" ]]; then
    Target="$Code/$Mode"
fi

set -e

mkdir -p "$Target"
cd       "$Target"

## build
cmake "${CMakeOptions[@]}" "$Code"
make -j ${NProc:-1} "$@"
