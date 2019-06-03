#include <asm/unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <edstruct.h>
#include <string.h>

#ifndef __NR_cpenc
#error cpenc system call not defined
#endif

int main(int argc, const char *argv[])
{
	/* Declaration of Flags */
	int opt_p=0,opt_e=0,opt_d=0,opt_c=0,opt_h=0;
	int c=0;
	opterr=0;

	/* Using getopt to retrieve the flags from command line */
	while ((c= getopt (argc,argv, "p:edch")) !=-1)
            switch(c)
                    {
                        case 'p':
                            if ((strl))

                        }



	int rc;
	void *dummy = (void *) argv[1];

  	rc = syscall(__NR_cpenc, dummy);
	if (rc == 0)
		printf("syscall returned %d\n", rc);
	else
		printf("syscall returned %d (errno=%d)\n", rc, errno);

	exit(rc);
}
