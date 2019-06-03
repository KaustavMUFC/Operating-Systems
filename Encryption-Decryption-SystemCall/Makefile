obj-m += sys_cpenc.o

INC=/lib/modules/$(shell uname -r)/build/arch/x86/include

all: tcpenc cpenc

tcpenc: tcpenc.c
	gcc -Wall -Werror -lssl -lcrypto -I$(INC)/generated/uapi -I$(INC)/uapi tcpenc.c -o xcpenc
cpenc:
	make -Wall -Werror -lssl -lcrypto -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules 

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f tcpenc
