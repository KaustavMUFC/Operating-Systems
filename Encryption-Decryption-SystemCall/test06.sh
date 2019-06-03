#!/bin/sh
# test whether multiple encryption flags can be passed
set -x
./xcpenc -e -e in.test.$$ out.test.$$
retval=$?
if test $retval == 0 ; then
	echo xcpenc failed with error: $retval
	exit $retval
else
	echo xcpenc program succeeded
fi

