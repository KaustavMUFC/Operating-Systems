#!/bin/sh
# test list version functionality

cd /usr/src/hw2-ksarkar/CSE-506/

# call write.c to create some versions
gcc write.c
./a.out
./a.out
./a.out
#Now compile bkpctl
gcc bkpctl.c -o bkpctl
#Now call bkpctl to list the versions
./bkpctl -l /mnt/bkpfs/write.txt
retval=$?
if test $retval == 0 ; then
        echo bkpctl failed with error: $retval
        exit $retval
else
        echo bkpctl program succeeded
fi
