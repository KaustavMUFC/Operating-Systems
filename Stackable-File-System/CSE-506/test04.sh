#!/bin/sh
# test delete version functionality with oldest passed

# call write.c to create some versions
gcc write.c
./a.out
./a.out
./a.out
#Now compile bkpctl
gcc bkpctl.c -o bkpctl
#Now call bkpctl to delete the oldest version
./bkpctl -d oldest /mnt/bkpfs/write.txt
retval=$?
if test $retval != 0 ; then
        echo bkpctl failed with error: $retval
        exit $retval
else
        echo bkpctl program succeeded
fi