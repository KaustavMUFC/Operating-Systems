#!/bin/sh
# test full encrypt and decrypt functionality
set -x
echo "dummy test" > in.test.$$
/bin/rm -f out.test.$$
./xcpenc -p "this is my password" -e in.test.$$ out.test.$$
retval=$?
if test $retval != 0 ; then
        echo xcpenc encryption failed with error: $retval
        exit $retval
else
        echo xcpenc encrypt program succeeded
fi
./xcpenc -p "this is my password" -d out.test.$$ dec.test.$$
if test $retval != 0 ; then
        echo xcpenc decryption failed with error: $retval
        exit $retval
else
        echo xcpenc decrypt program succeeded
fi

# now verify that the input file and decrypted file are the same
if cmp in.test.$$ dec.test.$$ ; then
        echo "xcpenc: input and decrypted files contents are the same"
        exit 0
else
        echo "xcpenc: input and decrypted files contents DIFFER"
        exit 1
fi
/bin/rm -f out.test.$$
/bin/rm -f in.test.$$
/bin/rm -f dec.test.$$