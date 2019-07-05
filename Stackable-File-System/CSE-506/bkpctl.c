
#include <asm/unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <openssl/md5.h>
#include <fcntl.h>
#include "bkpctl.h"

//The Main Function
int main(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind, optopt;
    int fd=0,rc=0,flagl=0,flagd=0,flagv=0,flagr=0,error=0; // Declaration of Flags
	char *arg_type,*file_name,*file_versions;

	arg_type = (char*)malloc(4096);


	// Allocating Memory to get the list of backup files for an original file
	file_versions = (char *)malloc(500);
	if (!file_versions )
        {
        printf("Memory allocation failed");
        goto out;
        }

       // Using getopt to retrieve the flags from command line
        while(1){
                int c ;
                c= getopt(argc, argv, "ld:v:r:");
                if (c == -1)
                        break;
                switch(c){
                        // List Versions Flag Set
                        case 'l':
                                flagl =1;
                                break;
                        // List Versions Flag Set
                        case 'd':
                      		arg_type = optarg;
                               	flagd=1;
                                break;
                        // List Versions Flag Set
                        case 'v':
                              	arg_type = optarg;
                               	flagv=1;
                                break;
                        // List Versions Flag Set
                        case 'r':
                              	arg_type = optarg;
                               	flagr=1;
                                break;

                        case ':':
                                printf("Option -%c requires argument\n",optopt);
                                error+=1;
                                break;
			}
        	}


    // Ensuring that the user doesn't pass more than one type of action from the command line
	if(flagl){
                if(flagd || flagv || flagr){
			printf("Versions can either be listed, viewed, restored or removed!!!\n");
			exit(2);
                }
        }

	if(flagd){
                if(flagl || flagv || flagr){
                        printf("Versions can either be listed, viewed, restored or removed!!!\n");
                        exit(2);
                }
        }

	if(flagv){
                if(flagl || flagd || flagr){
                        printf("Versions can either be listed, added or removed!!!\n");
                        exit(2);
                }
        }

    if(flagr){
                if(flagl || flagd || flagv){
                        printf("Versions can either be listed, added or removed!!!\n");
                        exit(2);
                }
        }
        if(error) {
                printf("Invalid arguments are provided\n");
                exit(2);
        }

      /*  if(optind!= argc -1){
                printf("Missing arguments\n");
                exit(0);
        }*/

    // Checking for the minimum and maximum number of arguments that can be passed from the user program
    if ((argc<3) || (argc>4)){
                    printf("Wrong Number of Arguments given. Arguments should be atleast 2 and atmost 4 in number depending on option selected.\n");
                    return -1;
                    }



    //Storing the File Name
	file_name = argv[optind++];

	//Opening the File
	fd = open(file_name, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if(fd < 1){
		printf("File cannot be opened!!!\n");
		return -1;
	}
	else
    {
        printf("File opened \n");
    }

	/* IOCTL for listing versions*/
	if(flagl==1){
		rc = ioctl(fd,BKPFS_LIST_VERSIONS,file_versions);
		if(rc > 0){
			printf("Unable to list versions\n");
			goto out;;
		}
		printf("********VERSION LIST**********\n %s", file_versions);
	}
	/*IOCTL for removing versions*/
	else if(flagd==1){
                rc = ioctl(fd,BKPFS_REMOVE_VERSIONS,arg_type);
                if(rc < 0){
                        printf("Unable to remove version(s)\n");
                	goto out;
		}
		else{
            printf("Removed %s version(s)\n",arg_type);
		}
        }
	/*IOCTL for viewing versions*/
	else if(flagv==1){
                rc = ioctl(fd,BKPFS_VIEW_VERSION,arg_type);
                if(rc < 0){
                        printf("Unable to view version(s)\n");
                	goto out;
		}
		else{
            printf("Content of the Version requested: \n %s \n",arg_type);
        }
	}
    /*IOCTL for restoring versions*/
    else if(flagr==1){
                rc = ioctl(fd,BKPFS_RESTORE_VERSION,arg_type);
                if(rc < 0){
                        printf("Unable to restore version\n");
                	goto out;
		}

		else{
            printf("Restored %s version\n",arg_type);
        }
    }

// Cleanup and Returning Val
out:
    if (file_versions )
    {
        free(file_versions);
    }


	return rc;
}
