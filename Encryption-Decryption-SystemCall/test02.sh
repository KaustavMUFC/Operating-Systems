#!/bin/sh
# test whether additional flags can be passed if help option is selected
set -x
./xcpenc -h -p -c in.test.$$ out.test.$$
retval=$?
if test $retval == 0 ; then
	echo xcpenc failed with error: $retval
	exit $retval
else
	echo xcpenc program succeeded
fi

