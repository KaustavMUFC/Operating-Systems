#!/bin/sh
# test whether more than 6 arguments can be passed or not
set -x
./xcpenc -p "this is my password" -c  in.test.$$ out.test.$$ yes ok right good
retval=$?
if test $retval == 0 ; then
	echo xcpenc failed with error: $retval
	exit $retval
else
	echo xcpenc program succeeded
fi

	