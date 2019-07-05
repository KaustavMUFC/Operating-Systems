#!/bin/sh
set -x
# WARNING: this script doesn't check for errors, so you have to enhance it in case any of the commands
# below fail.

umount  /mnt/bkpfs
rmmod bkpfs.ko
insmod fs/bkpfs/bkpfs.ko
mount -t bkpfs /test/bkp/ /mnt/bkpfsrm -rf fs/amfs/log.txt
# lsmod