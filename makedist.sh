#!/bin/sh
#

make distclean

if ! ./configure.windows-x86; then
	exit 1
fi

if ! make distwindows distclean; then
	exit 1
fi

if ! ./configure ; then
	exit 1
fi

if ! test "x$1" = "x-f"; then
	if ! make check; then
		exit 1
	fi
fi

if ! make dist; then
	exit 1
fi

