obj-m += hello3.o

KERNELSOURCE=/var/volatile/tmp/headers/kernel

all:

	make -C $(KERNELSOURCE) M=$(PWD) modules
	
clean:

	make -C $(KERNELSOURCE) M=$(PWD) clean
