#!/bin/sh
# test whether password can be passed along with copy flag
set -x
./xcpenc  -p "this is my password" -c in.test.$$ out.test.$$
retval=$?
if test $retval == 0 ; then
	echo xcpenc failed with error: $retval
	exit $retval
else
	echo xcpenc program succeeded
fi
