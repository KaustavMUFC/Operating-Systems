/* sys_cpenc.h--->Header File containing the Structure Definition for sys_cpenc */

/* The Structure Definition*/

#define KARGS_SIZE 255

typedef struct _kernelargs{
    char *infile;
    char *outfile;
    unsigned char keybuf[16];
    int keylen;
    int flag;

} kernelargs;

