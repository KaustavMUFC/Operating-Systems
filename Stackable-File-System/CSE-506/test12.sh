#!/bin/sh
# test whether backup files are hidden or not

# go to mount directory
cd ../../../../mnt/bkpfs
#Now compile bkpctl
#Now list the files in that directory
ls -a
retval=$?
if test $retval != 0 ; then
        echo bkpctl failed with error: $retval
        exit $retval
else
        echo bkpctl program succeeded
fi