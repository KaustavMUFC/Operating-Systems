#include <linux/linkage.h>
#include <linux/moduleloader.h>
#include <asm/unistd.h>
#include "sys_cpenc.h"
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <generated/autoconf.h>
#include <asm/unistd.h>
#include <linux/err.h>
#include <linux/scatterlist.h>
#include <linux/stat.h>
#include <linux/namei.h>
#include <linux/hash.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/key-type.h>
#include <linux/ceph/decode.h>
#include <crypto/md5.h>
#include <crypto/aes.h>
#include <keys/ceph-type.h>
#include <asm/uaccess.h>
#include <linux/crypto.h>
#include   <crypto/hash.h>
#include <linux/string.h>
#include <crypto/skcipher.h>
#include <linux/random.h>




//Putting All Data Structures together for the Encryption Decryption Mechanism *Source-https://www.kernel.org/doc/html/v4.18/crypto/api-samples.html*
struct skcipher_def {
    struct scatterlist sg;
    struct crypto_skcipher *tfm;
    struct skcipher_request *req;
    struct crypto_wait wait;
};


/*Performing the Cipher Operation. Adapted from*Source-https://www.kernel.org/doc/html/v4.18/crypto/api-samples.html*
 Optimized according to needs.*/
static unsigned int skcipher_encdec(struct skcipher_def *sk,
                     int enc)
{
    int rc;

    if (enc==1)
        rc = crypto_wait_req(crypto_skcipher_encrypt(sk->req), &sk->wait);
    else if (enc==2)
        rc = crypto_wait_req(crypto_skcipher_decrypt(sk->req), &sk->wait);
    else
        pr_info("Encrypt/Decrypt not selected");

    if (rc)
            pr_info("skcipher encrypt returned with result %d\n", rc);

    return rc;
}

/*Initialize and Trigger the Cipher Operation.Adapted from *Source-https://www.kernel.org/doc/html/v4.18/crypto/api-samples.html*
and modified accordingly*/
int sys_enc_dec(int ed_flag,char *readbuf,int read_bytes,unsigned char *kbuffer)
{   struct skcipher_def sk;
    struct crypto_skcipher *skcipher = NULL;
    struct skcipher_request *req = NULL;
    char *ivdata ;
    int task=ed_flag;
    char *rbuf=readbuf;
    int read=read_bytes;
    unsigned char *keybuf;
    int err;
    keybuf=kbuffer;
    skcipher = crypto_alloc_skcipher("ctr(aes)", 0, 0);
    if (IS_ERR(skcipher)) {
        printk("could not allocate skcipher handle\n");
        return PTR_ERR(skcipher);
    }
    else
    {
        printk("allocated skcipher handle\n");
    }

    req = skcipher_request_alloc(skcipher, GFP_KERNEL);
    if (!req) {
        printk("could not allocate skcipher request\n");
        err = -ENOMEM;
        goto out;
    }
    else
    {
        printk("Allocated skcipher request\n");
    }

    if (crypto_skcipher_setkey(skcipher,keybuf, 16)) {
        printk("key could not be set\n");
        err = -EAGAIN;
        goto out;
    }
    else
    {
        printk("Key has been set \n");
    }

    skcipher_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG,
                      crypto_req_done,
                      &sk.wait);

    ivdata = kmalloc(16, GFP_KERNEL);
    if (!ivdata) {
        printk("could not allocate ivdata\n");
        goto out;
    }
    else
    {
     printk("Allocated ivdata");
    }

    memcpy(ivdata,"cephsageyudagreg",16);

    sk.tfm = skcipher;
    sk.req = req;

    sg_init_one(&sk.sg, rbuf, read);
    skcipher_request_set_crypt(req, &sk.sg, &sk.sg, read, ivdata);
    crypto_init_wait(&sk.wait);

    /* encrypt/decrypt data */
    printk("The flag is %d",task);
    err = skcipher_encdec(&sk, task);
    if (err)
    {
    if(task==1)
    printk("Encryption triggered successfully\n");
    else if (task==2)
    printk("Decryption triggered successfully\n");
    else
    printk("Invalid action requested");
    }

out:
    if (skcipher)
        crypto_free_skcipher(skcipher);
    if (req)
        skcipher_request_free(req);
    if (ivdata)
        kfree(ivdata);

    return err;

    }

asmlinkage extern long (*sysptr)(void *arg);
asmlinkage long cpenc(void *arg)
{
	//dummy syscall: returns 0 for non null, -EINVAL for NULL
/*printk("cpenc received arg %p\n", arg);
	if (arg == NULL)
		return -EINVAL;
	else
		return 0;*/
    int err=0;
    char *rbuf, *extension=".tmp";
    struct dentry *temp_file_dentry, *output_file_dentry;
    int read,write;
    mm_segment_t oldfs;
    char *temp_file;
    struct crypto_shash *alg;
    struct shash_desc *shash;
    int check_hash;
    struct filename *input_file,*output_file;//,*keybuffer;
    //unsigned char * keybuffer;
    struct file *open_ip,*open_op, *open_tp;
    char *hash;
    int partial_write=0;
    kernelargs *kern_arg;
    //Checking if arguments have been successfully received in the Kernel side or not
	if (arg == NULL)
	{
		printk("\n Kernel side received invalid arguments\n");
		err= -EINVAL;
	}
	else
	{

		printk("\n Kernel side has received valid arguments %p",arg);

	}

	//Verifying Area of the Arguments Received
    if (access_ok (VERIFY_READ,arg, sizeof(kernelargs)))
	{
	    printk("Verify is ok\n");
	}
    else
    {
    printk(" Access Not Ok \n");
    err= -EACCES;
    goto free;
    }

    //Allocating Memory for the Private Copy of the Struct
    kern_arg= kmalloc(sizeof(struct _kernelargs),GFP_KERNEL);
	if(!kern_arg)
	{
	    printk("Out of Memory\n");
		err = -ENOMEM;
		goto free;
	}
	else
    {
       printk("Memory Allocated\n");
    }

    //Copy from User Performed for the Arguments Received
	if(copy_from_user(kern_arg,(void __user *)arg,sizeof(struct _kernelargs))==0)

    {
        printk("Copy from User successful\n");
    }
    else
    {
       printk("Copy didn't work \n");
       err= -EACCES;
    }

    /* Basic Checks */

    if(!(kern_arg->flag ==1 || kern_arg->flag ==2 || kern_arg->flag ==4))
		{
			printk(KERN_ALERT"Kernel Side has received invalid flags%d\n",kern_arg->flag);
			err= -EINVAL;
			goto free;
		}
    else
    {
        printk(KERN_ALERT"\n Kernel Side has received valid flags %d\n",kern_arg->flag);
    }
    if((kern_arg->keylen<=0) &&(kern_arg->flag!=4))
		{
        printk(KERN_ALERT"\n Kernel Side has received invalid key\n");
			err= -EINVAL;
			goto free;
		}

    else
    {
       printk(KERN_ALERT"\n Kernel Side has received valid key");
    }
    if((kern_arg->infile == NULL) || (kern_arg->outfile == NULL) || (kern_arg->keybuf==NULL)) {
		printk(KERN_INFO "Invalid Infile and Outfile.\n");
		err = -EINVAL;
		goto free;}
    else
    {
        printk(KERN_INFO "Valid Infile and Outfile.\n");
    }


    //Calling Getname Function to Allocate Memory for the Input File as well as Perform some checks automatically
    input_file=getname((__user char *)kern_arg->infile);

    if (input_file!=NULL)
    {
        printk("Infile Path successfully copied %s\n",input_file->name);
    }

    output_file=kmalloc(sizeof(struct filename*),GFP_KERNEL);

    // //Calling Getname Function to Allocate Memory for the Output File as well as Perform some checks automatically
    output_file=getname((__user char *)kern_arg->outfile);

    if (output_file!=NULL)
    {
        printk("Outfile Path successfully copied %s\n",output_file->name);
    }


    hash=kmalloc(16,GFP_KERNEL);
    if(!hash)
    {
	    printk("Out of Memory\n");
		err = -ENOMEM;
		goto free;
	}
	else
    {
       printk("Memory Allocated\n");
    }
    //MD5 Hashing inside the Kernel *Source: https://www.kernel.org/doc/html/v4.18/crypto/api-digest.html*. Modified Accordingly.
    alg= crypto_alloc_shash("md5",0,0);
    shash=kmalloc(sizeof(struct shash_desc)+crypto_shash_descsize(alg), GFP_KERNEL);
    shash->tfm = alg;

    crypto_shash_digest(shash,kern_arg->keybuf,kern_arg->keylen, hash);

    printk("The hash is: %s",hash);


    //Allocate Read Buffer

    rbuf=kmalloc(PAGE_SIZE,GFP_KERNEL);

    if(!rbuf)
	{
	    printk("Out of Memory\n");
		err = -ENOMEM;
		goto free;
	}
	else
    {
       printk("Memory Allocated\n");

    }
    memset(rbuf,0,PAGE_SIZE);

    //Opening Input File

        oldfs = get_fs();
        set_fs(get_ds());
        open_ip = filp_open(input_file->name, O_RDONLY, 0 );
        set_fs(oldfs);
        if ( !open_ip || IS_ERR( open_ip ) )
		{
			err = (int)PTR_ERR( open_ip );
			printk( KERN_ALERT "Couldn't open Input File: %d\n", err );
			goto free_rbuf;
    	}

        else if(!S_ISREG(open_ip->f_path.dentry->d_inode->i_mode))
        {
		err = -EBADF;
		goto ip_close;
        }

    	else
        {
            printk("Input File Opened \n");
        }

    //Opening Output File

        oldfs = get_fs();
        set_fs(get_ds());
        open_op = filp_open(output_file->name, O_WRONLY|O_CREAT, 0 );
        set_fs(oldfs);
        if ( !open_op || IS_ERR( open_op ) )
            partial_write=1;

        if ( !open_op || IS_ERR( open_op ) )
		{
			err = (int)PTR_ERR( open_op );
			printk( KERN_ALERT "Couldn't open Output File: %d\n", err );
			goto ip_close;
    	}

    	else if(!S_ISREG(open_op->f_path.dentry->d_inode->i_mode))
        {
		err = -EBADF;
		goto op_close;
        }

        else if((open_ip->f_path.dentry->d_inode->i_sb == open_op->f_path.dentry->d_inode->i_sb) &&
		(open_ip->f_path.dentry->d_inode->i_ino ==  open_op->f_path.dentry->d_inode->i_ino))
        {
		err = -EINVAL;
		goto op_close;
        }

    	else
        {
            printk("Output File Opened \n");
        }

        //Now lets create a temporary file and assign a name to it, that is, <output_file_name>.tmp

        temp_file=kmalloc(strlen(output_file->name)+5,GFP_KERNEL);


        if(!temp_file)
        {
	    printk("Out of Memory\n");
		err = -ENOMEM;
		goto op_close;
        }
        else
        {
        printk("Memory Allocated for Temporary File\n");

        }

        memset(temp_file, 0, strlen(output_file->name)+5);
        memcpy(temp_file, output_file->name, strlen(output_file->name));
        memcpy(temp_file+strlen(output_file->name), extension, 4);
        temp_file[strlen(output_file->name)+4] = '\0';

        printk("Temporary File Name is: %s" , temp_file );


        //Opening Temporary File


        oldfs = get_fs();
        set_fs(get_ds());
        open_tp = filp_open(temp_file, O_WRONLY|O_CREAT, open_ip->f_mode );
        set_fs(oldfs);
        if ( !open_tp || IS_ERR( open_tp ) )
		{
			err = (int)PTR_ERR( open_tp );
			printk( KERN_ALERT "Couldn't open Temporary File: %d\n", err );
			goto free_temp;
    	}

    	else if(!S_ISREG(open_tp->f_path.dentry->d_inode->i_mode))
        {
		err = -EBADF;
		goto tp_close;
        }

    	else
        {
            printk("Temporary File Opened \n");
        }

        //Now need to set File permissions of Temporary File same as the Input File

        open_tp->f_path.dentry->d_inode->i_mode = open_ip->f_path.dentry->d_inode->i_mode;
        open_tp->f_path.dentry->d_inode->i_opflags = open_ip->f_path.dentry->d_inode->i_opflags;
        open_tp->f_path.dentry->d_inode->i_uid = open_ip->f_path.dentry->d_inode->i_uid;
        open_tp->f_path.dentry->d_inode->i_gid = open_ip->f_path.dentry->d_inode->i_gid;
        open_tp->f_path.dentry->d_inode->i_flags = open_ip->f_path.dentry->d_inode->i_flags;

        //Setting the dentrys as well

        output_file_dentry = open_op->f_path.dentry;
        temp_file_dentry = open_tp->f_path.dentry;

        //Setting the Offsets

        open_ip->f_pos = 0;
        open_tp->f_pos = 0;

    //Encryption Process

    if (kern_arg->flag==1)

    {
    oldfs = get_fs();
    set_fs(get_ds());

    write=  vfs_write(open_tp, hash, 16, &open_tp->f_pos);

    if (write>0)
    {
        printk("Preamble written \n");
    }
    else
    {
        printk("Preamble not written\n");
        goto unlink;
    }

    while((read = vfs_read(open_ip, rbuf, PAGE_SIZE, &open_ip->f_pos))>0)

    {
    printk("The read is: %d",read);
    sys_enc_dec(1,rbuf,read,kern_arg->keybuf);
    write = vfs_write(open_tp, rbuf, read, &open_tp->f_pos);
    if (write<read)
    {
        printk("Could not write the encrypted content\n");
        goto unlink;
    }
    else
    {
        printk("Encrypted Content written\n");
    }
    }
    set_fs(oldfs);

    }

    //Decryption Process
    else if(kern_arg->flag==2)

    {
    oldfs = get_fs();
    set_fs(get_ds());

    read= vfs_read(open_ip, rbuf, 16, &open_ip->f_pos);

    if (read>0)
    {
        printk("Read the Preamble for correctness %d \n",read);
    }
    else
    {
        printk("Could not read preamble \n");
        goto unlink;
    }

    check_hash=memcmp(rbuf,hash,16);

    if(check_hash==0)
    while((read = vfs_read(open_ip, rbuf, PAGE_SIZE, &open_ip->f_pos))>0)
    {
    printk("The read is: %d",read);
    sys_enc_dec(2,rbuf,read,kern_arg->keybuf);
    write = vfs_write(open_tp, rbuf, read, &open_tp->f_pos);
    if (write<read)
    {
        printk("Could not write the decrypted content\n");
        goto unlink;
    }
    else
    {
        printk("Decrypted Content written %d\n",write);

    }
    }
    set_fs(oldfs);
    if(check_hash!=0)
    {
        printk("Password is incorrect. Please provide correct password to verify yourself");
        err=-EACCES;
        partial_write=1;
        goto unlink;
    }
    }


    //Copy Process
    else if(kern_arg->flag==4)
    {
    oldfs = get_fs();
    set_fs(get_ds());

     while((read = vfs_read(open_ip, rbuf, PAGE_SIZE, &open_ip->f_pos))>0)
    {
    printk("The read is: %d",read);
    write = vfs_write(open_tp, rbuf, read, &open_tp->f_pos);
    if (write<read)
    {
        printk("Could not write the content\n");
        goto unlink;
    }
    else
    {
        printk("Content written %d\n",write);
    }
    }
    set_fs(oldfs);
    }

    //Renaming the temporary file to output file
    vfs_rename(temp_file_dentry->d_parent->d_inode, temp_file_dentry, output_file_dentry->d_parent->d_inode, output_file_dentry, NULL, 0);

    //Partial Write Removal
    unlink:
            if(err<0)
			vfs_unlink(temp_file_dentry->d_parent->d_inode, temp_file_dentry, NULL); //Deleting temporary file.
			if(partial_write==1)
			vfs_unlink(output_file_dentry->d_parent->d_inode, output_file_dentry, NULL); //Deleting partial output file.

    //Closing and Freeing Files
    tp_close:
		filp_close(open_tp, NULL);
	free_temp:
		kfree(temp_file);
	op_close:
		filp_close(open_op, NULL);
	ip_close:
		filp_close(open_ip, NULL);
	free_rbuf:
		kfree(rbuf);
free:
    if (kern_arg!=NULL)
    {
        kfree(kern_arg);
        printk("Freed Memory\n");
    }

    if(input_file)
        putname(input_file);
    if(output_file)
        putname(output_file);
return err;
}


static int __init init_sys_cpenc(void)
{
	printk("installed new sys_cpenc module\n");
	if (sysptr == NULL)
		sysptr = cpenc;
	return 0;
}
static void  __exit exit_sys_cpenc(void)
{
	if (sysptr != NULL)
		sysptr = NULL;
	printk("removed sys_cpenc module\n");
}

module_init(init_sys_cpenc);
module_exit(exit_sys_cpenc);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kaustav Sarkar (ksarkar.cs@stonybrook.edu");
MODULE_DESCRIPTION("Sys_Cpenc System Call to Encrypt & Decrypt Files.");



