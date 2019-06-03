#!/bin/sh
# test whether any invalid flag is accepted or not
set -x
./xcpenc -o "this is my password" in.test.$$ out.test.$$
retval=$?
if test $retval == 0 ; then
	echo xcpenc failed with error: $retval
	exit $retval
else
	echo xcpenc program succeeded
fi

