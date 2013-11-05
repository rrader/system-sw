
obj-m += ffs.o 

all: mkfs
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

mkfs:
	make -C mkfs.ffs

reload:
	-rmmod -f ffs
	insmod ffs.ko
