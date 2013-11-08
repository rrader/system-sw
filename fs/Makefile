obj-m += ffs.o 

all: mkfs
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

mkfs:
	make -C mkfs.ffs

reload:
	-rmmod ffs
	-rmmod -f ffs
	insmod ffs.ko

test:
	-sudo umount /mnt/
	-sudo umount -f /mnt/
	make all
	sudo make reload
	sudo mount -o loop -t ffs mkfs.ffs/img /mnt
