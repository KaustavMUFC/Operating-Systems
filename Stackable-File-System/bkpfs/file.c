/*
 * Copyright (c) 1998-2017 Erez Zadok
 * Copyright (c) 2009      Shrikar Archak
 * Copyright (c) 2003-2017 Stony Brook University
 * Copyright (c) 2003-2017 The Research Foundation of SUNY
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "bkpctl.h"
#include "bkpfs.h"
#define BKPFS_XATTR_FILE_VERSION "user.latestversion"
#define BKPFS_XATTR_RR_STATUS "user.rrstatus"
#define BKPFS_MAX_FILE_VERSION 10



struct bkpfs_callback {
        struct dir_context ctx;
        struct dir_context *caller;
        struct super_block *sb;
        int filldir_called;
        int entries_written;
};

int hide_backup(const char *lower_name)
{
        lower_name = strrchr(lower_name, '.');
        printk("lower name found %s\n", lower_name);

        if( lower_name != NULL ){
                if(strcmp(lower_name, ".V1") == 0)
                        return 0;
                else if (strcmp(lower_name, ".V2") == 0)
                        return 0;
                else if(strcmp(lower_name, ".V3") == 0)
                        return 0;
                else if(strcmp(lower_name, ".V4") == 0)
                        return 0;
                else if(strcmp(lower_name, ".V5") == 0)
                        return 0;
                else if(strcmp(lower_name, ".V6") == 0)
                        return 0;
                else if(strcmp(lower_name, ".V7") == 0)
                        return 0;
                else if(strcmp(lower_name, ".V8") == 0)
                        return 0;
                else if(strcmp(lower_name, ".V9") == 0)
                        return 0;
                else if(strcmp(lower_name, ".V10") == 0)
                        return 0;
        }

  return( -1 );
}

/* Inspired from ecryptfs filldir */
static int bkpfs_filldir(struct dir_context *ctx, const char *lower_name, int lower_namelen,loff_t offset, u64 ino, unsigned int d_type)
{
        struct bkpfs_callback *buf = container_of(ctx, struct bkpfs_callback, ctx);

        size_t name_size;
        char *name;
        int rc;
        buf->filldir_called++;
        rc = hide_backup(lower_name);
        printk("Is backup returned %d\n", rc);
        if(rc == 0){
                printk("%s is a backup file, set name to NULL\n", lower_name);

        } else {
                printk("%s is not a backup file\n", lower_name);
                name = kmalloc((lower_namelen + 1), GFP_KERNEL);
                if(!name){
                        printk("could not alloc space for name");
                        goto out;
                }
                memcpy((void *)(name), (void *)lower_name, lower_namelen);
                name_size = lower_namelen;
                buf->caller->pos = buf->ctx.pos;
                rc = !dir_emit(buf->caller, name, name_size, ino, d_type);
        }

        //kfree(name);
        if (rc >= 0)
                buf->entries_written++;
        out:
        return rc;
}


static int bkpfs_readdir(struct file *file, struct dir_context *ctx)
{
        int err;
        struct file *lower_file = NULL;
        struct dentry *dentry = file->f_path.dentry;

        struct inode *inode = file_inode(file);
        struct bkpfs_callback buf = {
                .ctx.actor = bkpfs_filldir,
                .caller = ctx,
                .sb = inode->i_sb,
        };



        lower_file = bkpfs_lower_file(file);

        err = iterate_dir(lower_file, &buf.ctx);
        ctx->pos = buf.ctx.pos;
        file->f_pos = lower_file->f_pos;

        if (err >= 0)           /* copy the atime */
                fsstack_copy_attr_atime(d_inode(dentry),
                                        file_inode(lower_file));
        return err;
}

static ssize_t bkpfs_read(struct file *file, char __user *buf,
                           size_t count, loff_t *ppos)
{
        int err;
        struct file *lower_file;
        struct dentry *dentry = file->f_path.dentry;
        printk("inside bkfs read");

        lower_file = bkpfs_lower_file(file);
        err = vfs_read(lower_file, buf, count, ppos);
        /* update our inode atime upon a successful lower read */
        if (err >= 0)
                fsstack_copy_attr_atime(d_inode(dentry),
                                        file_inode(lower_file));

        return err;
}

// Modified bkps_write which creates a backup file when a file in the BKPFS File System is opened for writing
static ssize_t bkpfs_write(struct file *file, const char __user *buf,
                            size_t count, loff_t *ppos)
{
        int err;
        struct file *temporary = NULL, *lower_file;
        struct path lower_path,lower_parent_path;
        int written_bytes = 0;
        struct dentry *dentry = file->f_path.dentry, *parent, *lower_dir_dentry = NULL,*lower_dentry=NULL;
        int get_version, set_version,get_status, set_status,xversion,rr_status = 0;
        char ver_str[20], backup_name[50]="";
        struct qstr this_lower;
        const char *name_lower;
        struct vfsmount *lower_dir_mnt;
        const struct cred *cred;
        mm_segment_t oldfs;
        cred = current_cred();
        //Checking whether write is getting called with proper parameters
        printk("Inside bkpfs_write");
        lower_file = bkpfs_lower_file(file);
        printk("File pos: %lld\n", file->f_pos);
        printk("file size %lld\n", file->f_inode->i_size);
        printk("lower File size %lld\n", lower_file->f_inode->i_size);

        // Creating a -ve Dentry

        parent = dget_parent(file->f_path.dentry);
        printk("parent is %s\n", parent->d_name.name);
        bkpfs_get_lower_path(parent, &lower_parent_path);
        printk("got lower path for parent");
        lower_dir_dentry = lower_parent_path.dentry;
        printk("Lower dir dentry %s\n", lower_dir_dentry->d_name.name);

        // Setting the Version and Status using Extended Attributes

        get_version  = vfs_getxattr(file->f_path.dentry,BKPFS_XATTR_FILE_VERSION, &xversion, sizeof(int));
        get_status =  vfs_getxattr(file->f_path.dentry,BKPFS_XATTR_RR_STATUS, &rr_status, sizeof(int));
        printk("Get Version returned %d\n", get_version);
        printk("Get Status returned %d\n", get_status);
        printk("RR status returned as %d\n", rr_status);

       // Initializing First Version for a Backup File
        if(get_version == -ENODATA) {
                printk("Setting the version for the first time");
                xversion = 1;
        } else {
                if(xversion == 10){
                        /* Once maximum number of versions(10) are reached, the latest version is set to 1 and the BKPFS_XATTR_RR_STATUS
                        will be set to 1.*/
                        if(get_status == -ENODATA) {
                        //Setting the Round Robin status
                                rr_status = 1;
                                set_status = vfs_setxattr(file->f_path.dentry, BKPFS_XATTR_RR_STATUS,&rr_status, sizeof(int),0);
                                printk("Set Status returned %d\n", set_status);
                        }
                        // Setting the Version Number to 1 again and hence, overriding the file
                        xversion = 1;
                } else
                        xversion = xversion + 1;

        }
        printk("New Xversion %d\n", xversion);
        set_version = vfs_setxattr(file->f_path.dentry, BKPFS_XATTR_FILE_VERSION,&xversion, sizeof(int),0);
        printk("Set Attr returned %d\n", set_version);

        //Initializing the Process of Naming the Backup Version Files by Concatenating the Version from Extended Attributes and the Actual File Name

        snprintf(ver_str, sizeof(ver_str), "%d", xversion);
        printk("Typecasting version to ver_str %s\n", ver_str);
        printk("Final Version is %d\n", xversion);
        strcat(backup_name, ".");
        strcat(backup_name, file->f_path.dentry->d_name.name);
        strcat(backup_name, ".");
        strcat(backup_name, "V");
        strcat(backup_name, ver_str);

        printk("Backup name %s\n", backup_name);
        name_lower = backup_name;
        this_lower.name = name_lower;
        this_lower.len = strlen(name_lower);
        this_lower.hash = full_name_hash(lower_dir_dentry, this_lower.name, this_lower.len);

        //A d_lookup to check whether the backup file name is already present as a dentry or not
            lower_dentry = d_lookup(lower_dir_dentry, &this_lower);
            if(lower_dentry) {
                    printk("Dentry with the provide backup file name already exists!");
            } else {
                lower_dentry=d_alloc(lower_dir_dentry, &this_lower);
                if(!lower_dentry)
                {
                        printk("Lower dentry not allocated!\n");
                }
                else {
                        printk("Lower dentry Allocated!\n");
                        printk("Backup File Dentry created with name %s\n", lower_dentry->d_name.name);
                        d_add(lower_dentry, NULL);
                        //printk("Ref Count of Lower dentry %d\n", lower_dentry->d_lockref);
                }

                //Calling vfs_create to create the inode and hence the actual backup file
                lower_dir_dentry = lock_parent(lower_dentry);
                err = vfs_create(d_inode(lower_dir_dentry), lower_dentry, S_IRWXU|S_IRWXG|S_IRWXO, false);
                if(err) {
                        printk("Error while creating inode for bkp_dentry, err: %d\n", err);
                } else {
                        printk("Able to create inode for bkp_dentry");
                        //printk("Ref Count of Lower dentry is %d\n", lower_dentry->d_lockref );
                }
                unlock_dir(lower_dir_dentry);
        }
        //Doing a vfs_path lookup to check for the dentry

        lower_dir_mnt = lower_parent_path.mnt;
        err = vfs_path_lookup(lower_dir_dentry, lower_dir_mnt, lower_dentry->d_name.name, 0,&lower_path);
        if(!err)
                printk("Positive dentry found");
        else
                printk("Negative dentry found");

        //Opening the Dentry of the Backup File before actually writing to it

        temporary = dentry_open(&lower_path, O_CREAT|O_TRUNC|O_WRONLY, current_cred());
        if(temporary){
                 printk("File opened using dentry open");
                printk("The name of the file opened is %s\n", temporary->f_path.dentry->d_name.name);
        }else
                printk("Couldn't open Dentry");

        // Process of Taking Backup, that is, copying from the original file to the backup file
        temporary->f_pos=0;
        oldfs=get_fs();
        set_fs(KERNEL_DS);
        written_bytes = vfs_write(temporary,buf,count,&temporary->f_pos);
        printk("Written Bytes %d\n", written_bytes);
        set_fs(oldfs);

        // Cleaning Up or Releasing the Paths
        bkpfs_put_lower_path(parent, &lower_parent_path);
        dput(parent);

        err = vfs_write(lower_file, buf, count, ppos);
        /* update our inode times+sizes upon a successful lower write */
        if (err >= 0) {

                printk("file size after vfs_write %lld\n", file->f_inode->i_size);
                fsstack_copy_inode_size(d_inode(dentry),
                                        file_inode(lower_file));
                fsstack_copy_attr_times(d_inode(dentry),
                                        file_inode(lower_file));
        }

        return err;
}




// When user asks to remove all versions, we need to handle it in a different function using a for loop
int all_ver_rmv(struct file *file){
        int err = 0;
        int i = 1;

        // This for loop will check for all the versions of a particular file and delete them if they are found
        for(i=1; i < 11; i++) {


                char backup_name[50]="", ver_str[20];
                const char *name_lower;
                char *op_backup_file = NULL;
                struct dentry *lower_dir_dentry = NULL,*lower_dentry = NULL,*parent;
                struct path lower_parent_path;
                struct qstr this_lower;

                //Same Process of concatenating the original file name along with the version number
                strcat(backup_name, ".");
                strcat(backup_name, file->f_path.dentry->d_name.name);
                strcat(backup_name, ".");
                strcat(backup_name, "V");
                snprintf(ver_str, sizeof(ver_str), "%d", i);
                op_backup_file = ver_str;
                strcat(backup_name, op_backup_file);
                printk("backup file name to remove %s\n", backup_name);
                name_lower = backup_name;

                parent = dget_parent(file->f_path.dentry);
                printk("Parent is %s\n", parent->d_name.name);
                bkpfs_get_lower_path(parent, &lower_parent_path);
                printk("Got parent's lower path \n");

                lower_dir_dentry = lower_parent_path.dentry;
                printk("Lower dir dentry %s\n", lower_dir_dentry->d_name.name);


                this_lower.name = name_lower;
                this_lower.len = strlen(name_lower);
                this_lower.hash = full_name_hash(lower_dir_dentry, this_lower.name, this_lower.len);
                printk("Lookup Initiated \n");
                lower_dentry = d_lookup(lower_dir_dentry, &this_lower);
                printk("Lookup Completed \n");
                if(lower_dentry) {
                        printk("Dentry already exists!");
                        lower_dir_dentry = lock_parent(lower_dentry);

                        err = vfs_unlink(d_inode(lower_dir_dentry), lower_dentry, NULL);
                        printk("vfs_unlink returned %d\n", err);
                        if(err == 0){
                        d_drop(lower_dentry);
                        printk("Successful deletion");
                         err = 0;
                        }
                        else {
                        printk("Could not delete, Error is %d\n", err);
                        }
                        unlock_dir(lower_dir_dentry);
                        } else {
                                printk("Dentry not found, checking for the next one");
                        }
                bkpfs_put_lower_path(parent, &lower_parent_path);
                dput(parent);

        }
        return err;
}


//This Function does the whole process of finding out the lowest version by performing a series of lookups and then returns the oldest version number
int _get_oldest_version(struct file *file){
        int oldest_ver = 0,count = 0, get_status, get_version,xversion = 0, rr_status = 0;
        get_status =  vfs_getxattr(file->f_path.dentry,BKPFS_XATTR_RR_STATUS, &rr_status, sizeof(int));
        printk("Round Robin status is %d\n", rr_status);
        if (rr_status == 0){
                // If the version list is not full, then oldest is 1
                xversion = 1;
        } else if(rr_status == 1){
                // Since Round Robin flag is set, oldest version is set to file version + 1
                get_version  = vfs_getxattr(file->f_path.dentry,BKPFS_XATTR_FILE_VERSION, &xversion, sizeof(int));
                printk("Oldest Version is %d\n", xversion);
                if(xversion == 10)
                        xversion = 1;
                else
                        xversion = xversion + 1;
         }
        while(count < 10) {
                char backup_name[50]="",ver_str[20];
                const char *name_lower, *op_backup_file = NULL;
                struct dentry *lower_dir_dentry = NULL,*lower_dentry = NULL, *parent;
                struct path lower_parent_path;
                struct qstr this_lower;
                strcat(backup_name, ".");
                strcat(backup_name, file->f_path.dentry->d_name.name);
                strcat(backup_name, ".");
                strcat(backup_name, "V");
                snprintf(ver_str, sizeof(ver_str), "%d", xversion);
                op_backup_file = ver_str;
                strcat(backup_name, op_backup_file);
                printk("Oldest Backup found is %s\n", backup_name);
                name_lower = backup_name;
                parent = dget_parent(file->f_path.dentry);
                printk("The Parent is %s\n", parent->d_name.name);
                bkpfs_get_lower_path(parent, &lower_parent_path);
                printk("Found Parent's lower path");
                lower_dir_dentry = lower_parent_path.dentry;
                printk("Lower Directory Dentry %s\n", lower_dir_dentry->d_name.name);

                this_lower.name = name_lower;
                this_lower.len = strlen(name_lower);
                this_lower.hash = full_name_hash(lower_dir_dentry, this_lower.name, this_lower.len);
                printk("Lookup Initiated");
                lower_dentry = d_lookup(lower_dir_dentry, &this_lower);
                if(lower_dentry){
                        oldest_ver = xversion;
                        break;
                } else{
                        printk("Dentry with Version number %d not found, checking the next version\n", xversion);
                        if(xversion == 10)
                                xversion = 1;
                        else
                                xversion = xversion + 1;
                }
                count = count + 1;
        }
        return oldest_ver;

}

//This function removes versions, in all cases except when "all" parameter is passed
unsigned long remove_other_versions(struct file *file, const char *name_lower)
{       unsigned long err = 0;
        struct dentry *parent;
        struct dentry *lower_dir_dentry = NULL, *lower_dentry = NULL;
        struct path lower_parent_path;
        struct qstr this_lower;
        parent = dget_parent(file->f_path.dentry);
        printk("parent is %s\n", parent->d_name.name);
        bkpfs_get_lower_path(parent, &lower_parent_path);
        printk("got lower path for parent");

        lower_dir_dentry = lower_parent_path.dentry;
        printk("Lower dir dentry %s\n", lower_dir_dentry->d_name.name);


        this_lower.name = name_lower;
        this_lower.len = strlen(name_lower);
        this_lower.hash = full_name_hash(lower_dir_dentry, this_lower.name, this_lower.len);
        printk("Starting lookup");
        lower_dentry = d_lookup(lower_dir_dentry, &this_lower);
        printk("Completed lookup");
        if(lower_dentry) {
                printk("dentry with this name already exists!");
                lower_dir_dentry = lock_parent(lower_dentry);

                err = vfs_unlink(d_inode(lower_dir_dentry), lower_dentry, NULL);
                printk("vfs unlink returned %ld\n", err);
                if(err == 0){
                        d_drop(lower_dentry);
                        printk("Successful unlink");
                        err = 0;
                }
                else {
                        printk("Could not complete vfs unlink, err %ld\n", err);
                }
                unlock_dir(lower_dir_dentry);
                bkpfs_put_lower_path(parent, &lower_parent_path);
                dput(parent);

}
 return err;
}

// This function restores the content of the original file to the content of the backup file selected by the user
unsigned long restore_versions(struct file *file,const char *name_lower)
{               unsigned long err=0;
                int bytes = 0,temporary_bytes = 0, read_bytes = 0, written_bytes = 0;
                struct file *temp = NULL;
                char *buffer = NULL;
                struct vfsmount *lower_dir_mnt;
                mm_segment_t oldfs;
                struct path lower_path;
                struct qstr this_lower;
                struct dentry *lower_dir_dentry = NULL,*lower_dentry=NULL,*parent;
                struct path lower_parent_path;

                parent = dget_parent(file->f_path.dentry);
                printk("parent is %s\n", parent->d_name.name);
                bkpfs_get_lower_path(parent, &lower_parent_path);
                printk("got lower path for parent");
                lower_dir_dentry = lower_parent_path.dentry;
                printk("Lower dir dentry %s\n", lower_dir_dentry->d_name.name);


                this_lower.name = name_lower;
                this_lower.len = strlen(name_lower);
                this_lower.hash = full_name_hash(lower_dir_dentry, this_lower.name, this_lower.len);

                //Checking whether the backup file selected by the user is present or not
                lower_dentry = d_lookup(lower_dir_dentry, &this_lower);
                if(lower_dentry) {
                        printk("Found the Dentry of the Backup File to be restored!!\n");
                        buffer = (void *)kmalloc(PAGE_SIZE,GFP_KERNEL);
                        lower_dir_mnt = lower_parent_path.mnt;
                        err = vfs_path_lookup(lower_dir_dentry, lower_dir_mnt, lower_dentry->d_name.name, 0,&lower_path);
                        temp = dentry_open(&lower_path, O_RDONLY, current_cred());
                        if(temp){
                                printk("File opened");
                                printk("File Name of the opened file: %s\n", temp->f_path.dentry->d_name.name);
                        }
                        else {
                                printk("File couldn't be opened \n");
                                err = -EIO;
                                bkpfs_put_lower_path(parent, &lower_parent_path);
                                dput(parent);
                                goto out;
                        }
                        // Starting the restoration process
                        temp->f_pos=0;
                        bytes = temp->f_inode->i_size;

                        oldfs=get_fs();
                        set_fs(KERNEL_DS);
                        while(bytes > 0){
                                temporary_bytes = 0;
                                read_bytes = 0;
                                written_bytes = 0;

                                if(bytes > PAGE_SIZE) {
                                        temporary_bytes = PAGE_SIZE;
                                        bytes = bytes - temporary_bytes;
                                }else {
                                        temporary_bytes = bytes;
                                        bytes = 0;
                                }
                                read_bytes=vfs_read(temp, buffer, temporary_bytes, &temp->f_pos);
                                printk("Total bytes read is %d\n", read_bytes);
                                if(read_bytes < 0) {
                                        printk("Could not read from the backup file \n");
                                        err = read_bytes;
                                        set_fs(oldfs);
                                        bkpfs_put_lower_path(parent, &lower_parent_path);
                                        dput(parent);
                                        goto out;
                                }
                                written_bytes = vfs_write(file,buffer,read_bytes,&file->f_pos);
                                printk("Total bytes written is %d\n", written_bytes);
                                err=0;
                                if(written_bytes < 0){
                                        err = written_bytes;
                                        set_fs(oldfs);
                                        bkpfs_put_lower_path(parent, &lower_parent_path);
                                        dput(parent);
                                        goto out;
                                }
                        }

                set_fs(oldfs);
                } else {
                        printk("Dentry of the Backup not found\n");
                        err = -EFAULT;
                        bkpfs_put_lower_path(parent, &lower_parent_path);
                        dput(parent);
                        goto out;
                }

        out:
            if(buffer)
                kfree(buffer);
            return err;

}


unsigned long view_versions(struct file *file,const char *name_lower, char *arg)
{               unsigned long err=0;
                int bytes = 0,temporary_bytes = 0, read_bytes = 0, written_bytes = 0;
                struct file *temp = NULL;
                char *buffer = NULL;
                struct vfsmount *lower_dir_mnt;
                mm_segment_t oldfs;
                struct path lower_path;
                struct qstr this_lower;
                struct dentry *lower_dir_dentry = NULL,*lower_dentry=NULL,*parent;
                struct path lower_parent_path;

                parent = dget_parent(file->f_path.dentry);
                printk("parent is %s\n", parent->d_name.name);
                bkpfs_get_lower_path(parent, &lower_parent_path);
                printk("got lower path for parent");
                lower_dir_dentry = lower_parent_path.dentry;
                printk("Lower dir dentry %s\n", lower_dir_dentry->d_name.name);


                this_lower.name = name_lower;
                this_lower.len = strlen(name_lower);
                this_lower.hash = full_name_hash(lower_dir_dentry, this_lower.name, this_lower.len);

                //Checking whether the backup file selected by the user is present or not
                lower_dentry = d_lookup(lower_dir_dentry, &this_lower);
                if(lower_dentry) {
                        printk("Found the Dentry of the Backup File to be restored!!\n");
                        buffer = (void *)kmalloc(PAGE_SIZE,GFP_KERNEL);
                        lower_dir_mnt = lower_parent_path.mnt;
                        err = vfs_path_lookup(lower_dir_dentry, lower_dir_mnt, lower_dentry->d_name.name, 0,&lower_path);
                        temp = dentry_open(&lower_path, O_RDONLY, current_cred());
                        if(temp){
                                printk("File opened");
                                printk("File Name of the opened file: %s\n", temp->f_path.dentry->d_name.name);
                        }
                        else {
                                printk("File couldn't be opened \n");
                                err = -EIO;
                                bkpfs_put_lower_path(parent, &lower_parent_path);
                                dput(parent);
                                goto out;
                        }
                        // Starting the restoration process
                        temp->f_pos=0;
                        bytes = temp->f_inode->i_size;

                        oldfs=get_fs();
                        set_fs(KERNEL_DS);
                        while(bytes > 0){
                                temporary_bytes = 0;
                                read_bytes = 0;
                                written_bytes = 0;

                                if(bytes > PAGE_SIZE) {
                                        temporary_bytes = PAGE_SIZE;
                                        bytes = bytes - temporary_bytes;
                                }else {
                                        temporary_bytes = bytes;
                                        bytes = 0;
                                }
                                read_bytes=vfs_read(temp, buffer, temporary_bytes, &temp->f_pos);
                                printk("Total bytes read is %d\n", read_bytes);
                                if(read_bytes < 0) {
                                        printk("Could not read from the backup file \n");
                                        err = read_bytes;
                                        set_fs(oldfs);
                                        bkpfs_put_lower_path(parent, &lower_parent_path);
                                        dput(parent);
                                        goto out;
                                }
                                written_bytes = copy_to_user((char*) arg, buffer, read_bytes);
                                if(written_bytes <0 || written_bytes < read_bytes) {
                                        printk("Could not write to user buffer \n");
                                } else
                                        printk("Written to user buffer \n");

                        }

                set_fs(oldfs);
                } else {
                        printk("Dentry of the Backup not found\n");
                        err = -EFAULT;
                        bkpfs_put_lower_path(parent, &lower_parent_path);
                        dput(parent);
                        goto out;
                }

        out:
            if(buffer)
                kfree(buffer);
            return err;

}








static long bkpfs_unlocked_ioctl(struct file *file, unsigned int cmd,
                                  unsigned long arg)
{
        long err = -ENOTTY;
        struct file *lower_file;
        char *op_backup_file = NULL,*version_num = NULL;
        const char *name_lower;
        char backup_name[50]="", ver_str[20] ,version_list[500]="";
        int xversion,get_version, set_version, i;
        struct qstr this_lower;
        struct dentry *lower_dir_dentry = NULL, *lower_dentry=NULL, *parent;
        struct path lower_parent_path;


        printk("Unlocked Ioctl called");
        //Original File Passed from User Land
        printk("Original File %s\n", file->f_path.dentry->d_name.name);

        //Check whether the arg passed from User Land is ok
        if (access_ok (VERIFY_READ,arg, sizeof(arg)))
        {
	    printk("Verify is ok\n");
        }
        else
        {
        printk(" Access Not Ok \n");
        err= -EACCES;
        goto out;
        }

        //Now we need to find the actual backup file to be operated on
        version_num = (char *)arg;
        printk("Version to Operate on %s\n", version_num);

        //We now need to concatenate the version number to the actual file name to get the backup file to operate on and set its dentry

        strcat(backup_name, ".");
        strcat(backup_name, file->f_path.dentry->d_name.name);
        strcat(backup_name, ".");
        strcat(backup_name, "V");


        //If the newest argument is passed from command line we need to get the current newest version with the help of extended attributes
        if(strcmp(version_num, "newest") == 0) {
                printk("Newest Parameter passed from User Land");
                get_version  = vfs_getxattr(file->f_path.dentry,BKPFS_XATTR_FILE_VERSION, &xversion, sizeof(int));
        printk("Current Version returned by Xattribute %d\n", xversion);
        snprintf(ver_str, sizeof(ver_str), "%d", xversion);
        op_backup_file = ver_str;
        printk("The actual backup file number to operate on is %s\n", op_backup_file);
        }

        // If the oldest argument is passed from command line we need to get the oldest version with the help of a function
        else if(strcmp(version_num, "oldest") == 0){
                int xversion = _get_oldest_version(file);
                snprintf(ver_str, sizeof(ver_str), "%d", xversion);
                op_backup_file = ver_str;
                printk("The actual backup file number to operate on is  %s\n", op_backup_file);

        }

        // In case N is passed where N is the version number
        else {
                op_backup_file = version_num;
        }

         //This will be used in all operations
        strcat(backup_name, op_backup_file);
        printk("The backup file to operate on: %s\n", backup_name);
        name_lower = backup_name;
        printk("Checking name_lower %s\n", name_lower);

            parent = dget_parent(file->f_path.dentry);
            printk("parent is %s\n", parent->d_name.name);
            bkpfs_get_lower_path(parent, &lower_parent_path);
            printk("got lower path for parent");
            lower_dir_dentry = lower_parent_path.dentry;
            printk("Lower dir dentry %s\n", lower_dir_dentry->d_name.name);
            strcat(version_list,"List of the files are:\n");



        switch(cmd){

        //Removing a Backup File
        case BKPFS_REMOVE_VERSIONS:


        printk("Remove Flag Passed from User Land\n");
                //Remove other versions function is called if "all" is not passed
                err=remove_other_versions(file,name_lower);
                if(err==0)
                {
                    printk("Delete Successful\n");
                }

                else
                {
                    printk("Delete not Successful\n");
                }

                //We need to update the version accordingly if the newest version is removed


                if(strcmp(version_num, "newest") == 0){
                        printk("Updating Newest Version");
                        if(xversion == 1)
                                xversion = 10;
                        else
                                xversion = xversion - 1;
                        printk("Newest version now is %d\n", xversion);
                        set_version = vfs_setxattr(file->f_path.dentry, BKPFS_XATTR_FILE_VERSION,&xversion, sizeof(int),0);
                        printk("Set Version returned %d\n", set_version);
                        if(set_version > 0)
                                printk("Newest version updated successfully");
                }
             /*else {
                printk("Dentry not found");
                err = -EFAULT;
           }*/

           // All Versions Remove Function Called if "all" parameter is passed
           if(strcmp(version_num, "all") == 0){
                err = all_ver_rmv(file);
                goto out;
        }

        goto out;

        case BKPFS_LIST_VERSIONS:
            // Running a For Loop to compare whether a particular version of a particular file is present or not
			for(i=0;i<11;i++){
                char backup_name_list[50]="",list_str[20];
                int intvar = i;
                strcat(backup_name_list, ".");
                strcat(backup_name_list, file->f_path.dentry->d_name.name);
				strcat(backup_name_list, ".");
				strcat(backup_name_list, "V");
				snprintf(list_str, sizeof(list_str), "%d", intvar);
                strcat(backup_name_list, list_str);
                name_lower = backup_name_list;
                this_lower.name = name_lower;
                this_lower.len = strlen(name_lower);
                this_lower.hash = full_name_hash(lower_dir_dentry, this_lower.name, this_lower.len);

				//Doing a d_lookup to check whether a backup file is present or not. If present, store it in the version list,else not.
			        lower_dentry = d_lookup(lower_dir_dentry, &this_lower);
		            	if(lower_dentry) {
						strcat(version_list,name_lower);
						strcat(version_list,"\n");
            				}
			}

			bkpfs_put_lower_path(parent, &lower_parent_path);
            dput(parent);
			printk("********LIST**********\n");
			printk("%s\n",version_list);
			err=copy_to_user((char *)arg,&version_list[0],strlen(version_list));




         case BKPFS_RESTORE_VERSION:

                // The whole process of restoration is done by the restore_versions method
                err=restore_versions(file,name_lower);
                goto out;


        case BKPFS_VIEW_VERSION:

            err=view_versions(file,name_lower,(char *)arg);
            goto out;


        }

        lower_file = bkpfs_lower_file(file);

        /* XXX: use vfs_ioctl if/when VFS exports it */
        if (!lower_file || !lower_file->f_op)
                goto out;
        if (lower_file->f_op->unlocked_ioctl)
                err = lower_file->f_op->unlocked_ioctl(lower_file, cmd, arg);

        /* some ioctls can change inode attributes (EXT2_IOC_SETFLAGS) */
        if (!err)
                fsstack_copy_attr_all(file_inode(file),
                                      file_inode(lower_file));
out:
        bkpfs_put_lower_path(parent, &lower_parent_path);
        dput(parent);
        return err;
}

#ifdef CONFIG_COMPAT
static long bkpfs_compat_ioctl(struct file *file, unsigned int cmd,
                                unsigned long arg)
{
        long err = -ENOTTY;
        struct file *lower_file;

        lower_file = bkpfs_lower_file(file);

        /* XXX: use vfs_ioctl if/when VFS exports it */
        if (!lower_file || !lower_file->f_op)
                goto out;
        if (lower_file->f_op->compat_ioctl)
                err = lower_file->f_op->compat_ioctl(lower_file, cmd, arg);

out:
        return err;
}
#endif

static int bkpfs_mmap(struct file *file, struct vm_area_struct *vma)
{
        int err = 0;
        bool willwrite;
        struct file *lower_file;
        const struct vm_operations_struct *saved_vm_ops = NULL;

        /* this might be deferred to mmap's writepage */
        willwrite = ((vma->vm_flags | VM_SHARED | VM_WRITE) == vma->vm_flags);

        /*
         * File systems which do not implement ->writepage may use
         * generic_file_readonly_mmap as their ->mmap op.  If you call
         * generic_file_readonly_mmap with VM_WRITE, you'd get an -EINVAL.
         * But we cannot call the lower ->mmap op, so we can't tell that
         * writeable mappings won't work.  Therefore, our only choice is to
         * check if the lower file system supports the ->writepage, and if
         * not, return EINVAL (the same error that
         * generic_file_readonly_mmap returns in that case).
         */
        lower_file = bkpfs_lower_file(file);
        if (willwrite && !lower_file->f_mapping->a_ops->writepage) {
                err = -EINVAL;
                printk(KERN_ERR "bkpfs: lower file system does not "
                       "support writeable mmap\n");
                goto out;
        }

        /*
         * find and save lower vm_ops.
         *
         * XXX: the VFS should have a cleaner way of finding the lower vm_ops
         */
        if (!BKPFS_F(file)->lower_vm_ops) {
                err = lower_file->f_op->mmap(lower_file, vma);
                if (err) {
                        printk(KERN_ERR "bkpfs: lower mmap failed %d\n", err);
                        goto out;
                }
                saved_vm_ops = vma->vm_ops; /* save: came from lower ->mmap */
        }

        /*
         * Next 3 lines are all I need from generic_file_mmap.  I definitely
         * don't want its test for ->readpage which returns -ENOEXEC.
         */
        file_accessed(file);
        vma->vm_ops = &bkpfs_vm_ops;

        file->f_mapping->a_ops = &bkpfs_aops; /* set our aops */
        if (!BKPFS_F(file)->lower_vm_ops) /* save for our ->fault */
                BKPFS_F(file)->lower_vm_ops = saved_vm_ops;

out:
        return err;
}

static int bkpfs_open(struct inode *inode, struct file *file)
{
        int err = 0;
        struct file *lower_file = NULL;
        struct path lower_path;
        printk("inside bkpfs open");
        /* don't open unhashed/deleted files */
        if (d_unhashed(file->f_path.dentry)) {
                err = -ENOENT;
                goto out_err;
        }

        file->private_data =
                kzalloc(sizeof(struct bkpfs_file_info), GFP_KERNEL);
        if (!BKPFS_F(file)) {
                err = -ENOMEM;
                goto out_err;
        }

        /* open lower object and link bkpfs's file struct to lower's */
        bkpfs_get_lower_path(file->f_path.dentry, &lower_path);
        lower_file = dentry_open(&lower_path, file->f_flags, current_cred());
        path_put(&lower_path);
        if (IS_ERR(lower_file)) {
                err = PTR_ERR(lower_file);
                lower_file = bkpfs_lower_file(file);
                if (lower_file) {
                        bkpfs_set_lower_file(file, NULL);
                        fput(lower_file); /* fput calls dput for lower_dentry */
                }
        } else {
                bkpfs_set_lower_file(file, lower_file);
        }

        if (err)
                kfree(BKPFS_F(file));
        else
                fsstack_copy_attr_all(inode, bkpfs_lower_inode(inode));
out_err:
        return err;
}

static int bkpfs_flush(struct file *file, fl_owner_t id)
{
        int err = 0;
        struct file *lower_file = NULL;

        lower_file = bkpfs_lower_file(file);
        if (lower_file && lower_file->f_op && lower_file->f_op->flush) {
                filemap_write_and_wait(file->f_mapping);
                err = lower_file->f_op->flush(lower_file, id);
        }

        return err;
}

/* release all lower object references & free the file info structure */
static int bkpfs_file_release(struct inode *inode, struct file *file)
{
        struct file *lower_file;

        lower_file = bkpfs_lower_file(file);
        if (lower_file) {
                bkpfs_set_lower_file(file, NULL);
                fput(lower_file);
        }

        kfree(BKPFS_F(file));
        return 0;
}

static int bkpfs_fsync(struct file *file, loff_t start, loff_t end,
                        int datasync)
{
        int err;
        struct file *lower_file;
        struct path lower_path;
        struct dentry *dentry = file->f_path.dentry;

        err = __generic_file_fsync(file, start, end, datasync);
        if (err)
                goto out;
        lower_file = bkpfs_lower_file(file);
        bkpfs_get_lower_path(dentry, &lower_path);
        err = vfs_fsync_range(lower_file, start, end, datasync);
        bkpfs_put_lower_path(dentry, &lower_path);
out:
        return err;
}

static int bkpfs_fasync(int fd, struct file *file, int flag)
{
        int err = 0;
        struct file *lower_file = NULL;

        lower_file = bkpfs_lower_file(file);
        if (lower_file->f_op && lower_file->f_op->fasync)
                err = lower_file->f_op->fasync(fd, lower_file, flag);

        return err;
}

/*
 * Wrapfs cannot use generic_file_llseek as ->llseek, because it would
 * only set the offset of the upper file.  So we have to implement our
 * own method to set both the upper and lower file offsets
 * consistently.
 */
static loff_t bkpfs_file_llseek(struct file *file, loff_t offset, int whence)
{
        int err;
        struct file *lower_file;

        err = generic_file_llseek(file, offset, whence);
        if (err < 0)
                goto out;

        lower_file = bkpfs_lower_file(file);
        err = generic_file_llseek(lower_file, offset, whence);

out:
        return err;
}

/*
 * Wrapfs read_iter, redirect modified iocb to lower read_iter
 */
ssize_t
bkpfs_read_iter(struct kiocb *iocb, struct iov_iter *iter)
{
        int err;
        struct file *file = iocb->ki_filp, *lower_file;

        lower_file = bkpfs_lower_file(file);
        if (!lower_file->f_op->read_iter) {
                err = -EINVAL;
                goto out;
        }

        get_file(lower_file); /* prevent lower_file from being released */
        iocb->ki_filp = lower_file;
        err = lower_file->f_op->read_iter(iocb, iter);
        iocb->ki_filp = file;
        fput(lower_file);
        /* update upper inode atime as needed */
        if (err >= 0 || err == -EIOCBQUEUED)
                fsstack_copy_attr_atime(d_inode(file->f_path.dentry),
                                        file_inode(lower_file));
out:
        return err;
}

/*
 * Wrapfs write_iter, redirect modified iocb to lower write_iter
 */
ssize_t
bkpfs_write_iter(struct kiocb *iocb, struct iov_iter *iter)
{
        int err;
        struct file *file = iocb->ki_filp, *lower_file;

        lower_file = bkpfs_lower_file(file);
        if (!lower_file->f_op->write_iter) {
                err = -EINVAL;
                goto out;
        }

        get_file(lower_file); /* prevent lower_file from being released */
        iocb->ki_filp = lower_file;
        err = lower_file->f_op->write_iter(iocb, iter);
        iocb->ki_filp = file;
        fput(lower_file);
        /* update upper inode times/sizes as needed */
        if (err >= 0 || err == -EIOCBQUEUED) {
                fsstack_copy_inode_size(d_inode(file->f_path.dentry),
                                        file_inode(lower_file));
                fsstack_copy_attr_times(d_inode(file->f_path.dentry),
                                        file_inode(lower_file));
        }
out:
        return err;
}

const struct file_operations bkpfs_main_fops = {
        .llseek         = generic_file_llseek,
        .read           = bkpfs_read,
        .write          = bkpfs_write,
        .unlocked_ioctl = bkpfs_unlocked_ioctl,
#ifdef CONFIG_COMPAT
        .compat_ioctl   = bkpfs_compat_ioctl,
#endif
        .mmap           = bkpfs_mmap,
        .open           = bkpfs_open,
        .flush          = bkpfs_flush,
        .release        = bkpfs_file_release,
        .fsync          = bkpfs_fsync,
        .fasync         = bkpfs_fasync,
        .read_iter      = bkpfs_read_iter,
        .write_iter     = bkpfs_write_iter,
};

/* trimmed directory options */
const struct file_operations bkpfs_dir_fops = {
        .llseek         = bkpfs_file_llseek,
        .read           = generic_read_dir,
        .iterate        = bkpfs_readdir,
        .unlocked_ioctl = bkpfs_unlocked_ioctl,
#ifdef CONFIG_COMPAT
        .compat_ioctl   = bkpfs_compat_ioctl,
#endif
        .open           = bkpfs_open,
        .release        = bkpfs_file_release,
        .flush          = bkpfs_flush,
        .fsync          = bkpfs_fsync,
        .fasync         = bkpfs_fasync,
};
