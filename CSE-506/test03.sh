#!/bin/sh
# test delete version functionality with newest passed

# call write.c to create some versions
gcc write.c
./a.out
./a.out
./a.out
#Now compile bkpctl
gcc bkpctl.c -o bkpctl
#Now call bkpctl to delete the newest version
./bkpctl -d newest /mnt/bkpfs/write.txt
retval=$?
if test $retval != 0 ; then
        echo bkpctl failed with error: $retval
        exit $retval
else
        echo bkpctl program succeeded
fi