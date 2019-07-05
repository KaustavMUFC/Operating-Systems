#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(void)
{
        int fp, ret;
        char buf[] = "hello world this is Manchester United\n";

        fp = open("/mnt/bkpfs/write.txt", O_WRONLY | O_CREAT, 0644);
        if (fp < 0) {
                perror("open failed");
                return 1;
        }
        ret = write(fp, buf, strlen(buf));
        if (ret < 0) {
                perror("write failed");
                return 1;
        }
        printf("wrote %d bytes\n", ret);
        close(fp);
        return 0;
}
