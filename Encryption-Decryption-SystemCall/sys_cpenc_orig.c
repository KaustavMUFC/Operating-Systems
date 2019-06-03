#include <linux/linkage.h>
#include <linux/moduleloader.h>
#include <asm/unistd.h>
#include "edstruct.h"
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

asmlinkage extern long (*sysptr)(void *arg);

asmlinkage long cpenc(void *arg)
{
	//dummy syscall: returns 0 for non null, -EINVAL for NULL
	/*printk("cpenc received arg %p\n", arg);
	if (arg == NULL)
		return -EINVAL;
	else
		return 0;*/
    int cp_flg=0;
    //int flag;
    int errno,verify;
	struct _kernelargs *kern_arg;
	struct file *input_file = NULL, *output_file = NULL; //*temp_file = NULL;
	/*struct dentry *tmpDentry = NULL;
	struct inode *tmpInode = NULL;*/




    verify= access_ok (VERIFY_READ,arg, sizeof(arg));
	printk("Verify is %d",verify);
	if (verify==0)
	{
	   printk(KERN_ALERT"\n Invalid Address Space");
	   errno= -EFAULT;
	   //goto BUFFER_END;
	}

	if (arg == NULL)
	{
		printk(KERN_ALERT"\n Kernel side received invalid arguments");
		errno= -EINVAL;
	}
	else
	{
		kern_arg = kmalloc(sizeof(kernelargs),GFP_KERNEL);
		if ( kern_arg == NULL)
		{
			printk(KERN_ALERT"\n Kernel Side Out of Memory");
			errno= -ENOMEM;
		}
	    printk(KERN_ALERT "Kernel has successfully received arguments from Userland %p \n", arg);
	}

   /* cp_flg= copy_from_user(&kern_arg->keylen,&arg->keylen, sizeof(int));
    if (cp_flg>0)
    {
        printk( KERN_ALERT "Kernel Arguments couln't be copied ");
			err = cp_flg;
			//goto BUFFER_END;
    }
    cp_flg= copy_from_user(&kern_arg->flag,&arg->flag, sizeof(int));

     if (cp_flg>0)
    {
        printk( KERN_ALERT "Flag couln't be copied ");
			err = cp_flg;
			//goto BUFFER_END;
    }
*/

    cp_flg= copy_from_user(kern_arg,arg, sizeof(kernelargs));
    if((kern_arg->keybuf == NULL) || (kern_arg->keylen == 0)) {
		printk(KERN_INFO "Invalid Keybuffer or Keylength.\n");
		errno = -EINVAL;
		//goto BUFFER_END;
	}

    if(!(kern_arg->flag ==1 || kern_arg->flag ==2 || kern_arg->flag ==4))
		{
			printk(KERN_ALERT"\n Kernel Side has received invalid flags");
			errno= -EINVAL;
			//goto BUFFER_END;;
		}
    if(kern_arg->keylen!=16)
		{
			printk(KERN_ALERT"\n Kernel Side has received invalid key");
			errno= -EINVAL;
			//goto BUFFER_END;
		}
    if((kern_arg->infile == NULL) || (kern_arg->outfile == NULL)) {
		printk(KERN_INFO "Invalid Infile and Outfile.\n");
		errno = -EINVAL;}
		//goto BUFFER_END;
    if((kern_arg->keylen!=strlen(kern_arg->keybuf)))
    {
       printk(KERN_INFO "Length of Key Buffer and Key Length is not the same.\n");
       errno= -EINVAL;
       //goto BUFFER_END;
    }

    /*Open I/P File and O/P File with Permissions*/

    input_file = filp_open( kern_arg->infile, O_RDONLY, 0 );
        if ( !input_file || IS_ERR( input_file ) )
		{
			errno = (int)PTR_ERR( input_file );
			printk( KERN_ALERT "Couldn't open Input File: %d\n", errno );
			input_file = NULL;
			//goto BUFFER_END;
    	}

    	else if ((!S_ISREG(input_file->f_path.dentry->d_inode->i_mode))) {
                printk(KERN_INFO "Input File is not regular.\n");
		errno = -EIO;
                //goto BUFFER_END;
                }

		output_file = filp_open( kern_arg->outfile, O_WRONLY | O_CREAT,0);
        if ( !output_file || IS_ERR( output_file ) )
		{
			errno = (int)PTR_ERR( output_file );
			printk( KERN_ALERT "Couldn't open Output File: %d\n", errno );
			output_file = NULL;
			//goto BUFFER_END;
        }
        	else if ((!S_ISREG(output_file->f_path.dentry->d_inode->i_mode))) {
                printk(KERN_INFO "Output File is not regular.\n");
		errno = -EIO;
                //goto BUFFER_END;
                }








	return 1;
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
//MODULE_AUTHOR("Kaustav Sarkar");
//MODULE_DESCRIPTION("Sys_Cpenc System Call to Encrypt & Decrypt Files.");
//MODULE_LICENSE("Dual BSD/GPL");


