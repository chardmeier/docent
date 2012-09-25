#! /bin/sh

while [ $# -gt 0 ]
do
	case "$1" in
	-Wl,@*)
		file="`echo $1 | sed 's/^-Wl,@//'`"
		cmd="$cmd `cat $file`"
		;;
	*)
		cmd="$cmd \"$1\""
	esac
	shift
done

eval exec $cmd
