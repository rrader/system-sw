CC=gcc
CFLAGS=-g
.PHONY: mkfs.ffs
mkfs.ffs: mkfs.o
		$(CC) $(CFLAGS) -o $@ $^

mkfs.o: mkfs.c
		$(CC) -c $(CFLAGS) -o $@ $<

.PHONY: img
img: mkfs.ffs
	-rm -f img
	dd if=/dev/zero of=img bs=1024 count=4096
	./mkfs.ffs img
