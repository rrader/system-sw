umount /mnt
umount /mnt
umount /mnt
umount /mnt

rmmod ffs

cd mkfs.ffs
make img
cd ..
make
insmod ffs.ko
mount -o loop -t ffs ./mkfs.ffs/img /mnt

