#!/bin/sh
# test whether both encrypt and decrypt option can be selected
set -x
./xcpenc -e -d in.test.$$ out.test.$$
retval=$?
if test $retval == 0 ; then
	echo xcpenc failed with error: $retval
	exit $retval
else
	echo xcpenc program succeeded
fi

