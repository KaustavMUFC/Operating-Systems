#!/bin/sh
# test whether password is at least 6 characters or else no encryption/decryption takes place
set -x
./xcpenc -p "this" -e in.test.$$ out.test.$$
retval=$?
if test $retval != 0 ; then
	echo xcpenc failed with error: $retval
	exit $retval
else
	echo xcpenc program succeeded
fi

