#include <asm/unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "sys_cpenc.h"
#include <string.h>
#include <ctype.h>
#include <openssl/evp.h>
#include <openssl/md5.h>


#ifndef __NR_cpenc
#error cpenc system call not defined
#endif



/* Help Function to Print Appropriate Help Messages */
void helptext (int hlp)
{
    if (hlp==0)
    {
        printf("Help is Here \n \
               This is to guide you to use the interface in a proper way \n \
               The file should be run in the following format: \n\
               ./xcpenc -p this is my password -e infile outfile \n\
               The following is the information about the valid flags you can use: \n\
               1. -e: This is to encrypt the file \n\
               2. -d: This is to decrypt the file \n\
               3. -c: This is to copy the infile onto the outfile \n\
               4. -p: This is to specify the password. Once this option is entered, you must specify a proper password. \n\
               5. -h: No need to say anything about this, since you already know how and why to use it."
               );

    }

}

//Checks for Invalid Arguments
int check_func(char *password, int argc, int flag)
{   int flg=flag;
	int check=-1;

    while (flg!=10 || flg!=4){
	if (password == NULL) {
		printf ("The password cannot be NULL.");
		goto ERROR;
	}
	else
	if (strlen(password) < 6) {
        printf ("The length of the password should at least be 6 characters.");
		goto ERROR;
	}

	else
	if((flg == -1)) {
		printf ("Missing -e/-d/-c option\n");
		goto ERROR;
	}
	else
	if ((optind+2) > argc || (argc-optind > 2)) {
        if (argc-optind == 0)
			printf ("Missing input and output files\n");

		if (argc-optind == 1)
			printf ("Missing output file\n");

		if (argc-optind > 2)
			printf ("Only 1 Input File and 1 Output File is allowed\n");
		goto ERROR;
	}
	else
        check=1;

	return check;
    }
ERROR:
	return EXIT_FAILURE;
}

// Remove New Line before passing it through MD5 Hash
char *rm_new_line(char *password)
{
    char *ref_password;
    char *pwd= password;

    ref_password = malloc (strlen(pwd) + 1);
    if(ref_password==NULL){
		printf("Malloc Failed.\n");
		}
	int a=0,b=0;
	for (a=0;pwd[a]!='\0';a++)
    {
        if (pwd[a]!='\n')
        {
            ref_password[b]=pwd[a];
            b++;
        }
        ref_password[b]='\0';
    }
return ref_password;
free(ref_password);
}

//The Main Function
int main(int argc, char **argv)
{
	/* Declaration of Flags */
	int opt_p=0,opt_e=0,opt_d=0,opt_c=0,opt_h=0;
	int check;
	int c=0;
	opterr=0;
	/*Dynamic Memory Allocation to Structure */
    struct _kernelargs *kernel_buf = (struct _kernelargs *)malloc(sizeof(struct _kernelargs));
    if (!kernel_buf && (sizeof(kernelargs)+1 > 0))
        {
        printf("Memory allocation failed");
        goto free_arg;
        }
    /* Some other variables */
    char *password=NULL;
    char *ref_password=NULL;
    unsigned char pwd_hash[20];
    kernel_buf->flag = -1;
	/* Using getopt to retrieve the flags from command line */
	while ((c= getopt (argc,argv, "p:edch")) !=-1){
            switch(c)
                    {
                        //Password Flag to get the Password
                        case 'p':
                            if (opt_p==0)
                            {
                                opt_p=1;
                                password=optarg;
                                helptext(-1);
                            }
                            else
                            {
                                printf("-p option can be used atmost once.");
                                return -1;
                                goto free_arg;
                            }
                                break;

                        // Encryption Flag Set
                        case 'e':
                            if(opt_e==0)
                            {opt_e=1;
                            kernel_buf->flag = 1;
                            helptext(-1);}
                            else
                            {
                                printf("-e option can be used atmost once.");
                                return -1;
                                goto free_arg;
                            }
                        break;
                        /* Decryption Flag Set */
                        case 'd':
                          if(opt_d==0)
                            {opt_d=1;
                            kernel_buf->flag = 2;
                            helptext(-1);}
                            else
                            {
                                printf("-d option can be used atmost once.");
                                return -1;
                                goto free_arg;
                            }
                        break;
                        // Copy Flag Set
                        case 'c':
                            if(opt_c==0)
                            {opt_c=1;
                            kernel_buf->flag = 4;
                            helptext(-1);}
                            else
                            {
                                printf("-c option can be used atmost once.");
                                return -1;
                                goto free_arg;
                            }
                            break;
                        //Help Flag
                        case 'h':
                            {opt_h=1;
                            kernel_buf->flag = 10;
                            break;
                            }
                        default:
                            printf ("Incorrect option selected. Give it another try.");
                            return -1;
                            break;

                    }
                }

                    //Some more checks

                    if ((argc<2) || (argc>6)){
                    printf("Wrong Number of Arguments given. Arguments should be atleast 2 and atmost 6 in number depending on option selected.\n");
                    return -1;
                    }
                    if((opt_h==1)&&((opt_e==1)||(opt_d==1)||(opt_c==1)||(opt_p==1)))
                       {
                           printf("If help option is selected, you can't select any other option\n");
                           return -1;
                       }

                    //Checking whether both Encryption and Decryption flags are passed
                    if ((opt_e==1)&&(opt_d==1))
                        {
                            printf("Select either encrypt or decrypt flag but not both.\n");
                            return -1;

                        }
                    // Checking whether if Copy Flag is set, Encryption and Decryption flags and Password are set or not
                    if (((opt_c)==1)&& ((opt_e==1)||(opt_d==1)||(opt_p==1)))
                        {
                            printf("When copy flag is selected, you can't select password flags or encrypt or decrypt flags.\n");
                            return -1;
                        }
                    // When Help Flag is selected,no other flags can be set

                     if ((opt_h==1) && (argc>2))
                    {
                        printf("If u need help, don't pass the infile and outfile\n");
                        return -1;
                    }
                    // Help Function Called
                    if((opt_h==1))
                    {
                        helptext(0);
                        return 0;
                    }

                    //Checking Encryption and Decryption Conditions and also Calling Remove New Line Function
                    if(opt_e==1 || opt_d==1 ||opt_p==1)
                    {check = check_func(password, argc, kernel_buf->flag);
                        if (check==1)
                        {
                        ref_password=rm_new_line(password);
                        }
                        }

                    //Calling MD5 Function
                    if(opt_e==1 || opt_d==1)
                    {
                    int pwd_len= strlen(ref_password);
                    MD5((unsigned char*)ref_password, pwd_len, pwd_hash);
                    }

            //Copying the MD5 Hash to the Kernel Buffer
            strncpy((char *)kernel_buf->keybuf,(char *)pwd_hash,16);
            //Allocating Memory for the Kernel Buffer Parameters
            kernel_buf->infile=(char *) malloc (KARGS_SIZE);
            memset(kernel_buf->infile,'\0',KARGS_SIZE);
            kernel_buf->outfile=(char *) malloc (KARGS_SIZE);
            memset(kernel_buf->outfile,'\0',KARGS_SIZE);
            //Storing the Parameters in the Kernel Buffer
            strncpy(kernel_buf->infile, argv[optind], strlen(argv[optind]));
            strncpy(kernel_buf->outfile, argv[optind+1], strlen(argv[optind+1]));
            kernel_buf->keylen =strlen((char *)kernel_buf->keybuf);

            /* System Call Invocation */
            int res;
            res = syscall(__NR_cpenc, kernel_buf);
            if (res == 0 && check==1)
            printf("syscall returned %d\n", res);
            else
            printf("syscall returned %d (errno=%d)\n", res, errno);

            //Free Allocated Memory
            free_arg:
            free(kernel_buf);

 return 0;
}

