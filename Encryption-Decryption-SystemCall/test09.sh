#!/bin/sh
# test whether if input file and output file can be passed with help option
set -x
./xcpenc -h in.test.$$ out.test.$$
retval=$?
if test $retval == 0 ; then
	echo xcpenc failed with error: $retval
	exit $retval
else
	echo xcpenc program succeeded
fi

