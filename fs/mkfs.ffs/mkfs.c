#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include "../const.h"

int file_size(char *filename, unsigned long *size)
{
    struct stat fstat;
    int fd;
    if (stat(filename, &fstat) < 0) {
        return -1;
    }
    if (S_ISREG(fstat.st_mode)) {
        *size = fstat.st_size;
        printf("file size: %u\n", *size); 
    } else if (S_ISBLK(fstat.st_mode)) {
    //     *blk_size = 1024;
    //     fd = open(filename, O_RDONLY);
    //     ioctl(fd, BLKGETSIZE, nr_blks);
    //     *nr_blks = *nr_blks >> 1;
        printf("block devices are not supported"); 
        return -2;
    } else
        return -1;
    return 0;
}

int main(int argc, char* argv[]) {
    char *filename;
    int fd;
    unsigned long size;

    printf("\nmkfs.ffs\n============================\n");

    if (argc < 2) {
        fprintf(stderr, "%s filename\n", argv[0]);
        return -1;
    }

    filename = argv[1];
    if (file_size(filename, &size) < 0) {
        fprintf(stderr, "%s should be file or block device\n", filename);
        return -1;
    }

    fd = open(filename, O_RDWR);
    struct ffs_metainfo_sector info;
    info.magic = FFS_MAGIC;
    info.block_count = size/FFS_BLOCK_SIZE;
    info.block_size = FFS_BLOCK_SIZE;
    printf("magic: %X\n", info.magic);
    printf("block_count: %d\n", info.block_count);
    printf("block_size: %d\n", info.block_size);

    lseek(fd, FFS_BLOCK_SIZE, SEEK_SET);
    write(fd, &info, sizeof(info));
    printf("OK\n");
    return 0;
}
