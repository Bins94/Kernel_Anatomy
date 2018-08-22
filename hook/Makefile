obj-m += kfunch_ftrace.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

test:	
	gcc test.c -o test

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
