#ifndef BKPCTL_H
#define BKPCTL_H

#define BKPFS_LIST_VERSIONS _IOR(0x15,0,char*)
#define BKPFS_REMOVE_VERSIONS _IOWR(0x20,1,char*)
#define BKPFS_VIEW_VERSION _IOR(0x14,2,char*)
#define BKPFS_RESTORE_VERSION _IOWR(0x21,3,char*)
#endif
