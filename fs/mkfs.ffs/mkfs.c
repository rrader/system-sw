#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <limits.h>
#include <string.h>
#include "../const.h"

#define BIT_MASK(x) \
    (((x) >= sizeof(unsigned) * CHAR_BIT) ? \
        (unsigned) -1 : (1U << (x)) - 1)

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
    printf("block_count: %d\n", info.block_count);
    printf("block_size: %d\n", info.block_size);

    int b_bm_size = info.block_count >> 3; //blocks bitmask
    info.b_bm_blocks = (b_bm_size + FFS_BLOCK_SIZE - 1) / FFS_BLOCK_SIZE;

    info.max_file_count = (size*0.005) / sizeof(struct ffs_fd);
    printf("file count: %d\n", info.max_file_count);
    int fd_bm_size = (info.max_file_count + 7) / 8; //file descriptor bitmask
    info.fd_bm_blocks = (fd_bm_size + FFS_BLOCK_SIZE - 1) / FFS_BLOCK_SIZE;

    int fd_per_block = FFS_BLOCK_SIZE / sizeof(struct ffs_fd);
    info.fd_blocks = (info.max_file_count+fd_per_block-1) / fd_per_block;
    
    int service_blocks = 1+info.b_bm_blocks+info.fd_bm_blocks+info.fd_blocks;
    printf("service info: %d blocks (%1.1f%%)\n", service_blocks, ((float)(service_blocks)/info.block_count)*100);

    lseek(fd, 0, SEEK_SET);  // file begin
    write(fd, &info, sizeof(info));

    lseek(fd, FFS_BLOCK_SIZE*1, SEEK_SET);  // blocks bitmap
    int c = service_blocks + 4; //FIXME: +1 for test file
    char b = 0;

    while (c) {
        if (c >= CHAR_BIT) {
            b = BIT_MASK(CHAR_BIT);
            c -= CHAR_BIT;
        } else {
            b = BIT_MASK(c);
            c -= c;
        }
        write(fd, &b, sizeof(b));
    }

    lseek(fd, FFS_BLOCK_SIZE*(info.b_bm_blocks), SEEK_SET);  // fd bitmap
    // empty for now
    b = BIT_MASK(2);  // FIXME: one file
    write(fd, &b, sizeof(b));

    lseek(fd, FFS_BLOCK_SIZE*(info.b_bm_blocks+info.fd_bm_blocks), SEEK_SET);  // fd list
    // FIXME: empty
    struct ffs_fd file;
    file.type = FFS_REG;
    file.datablock_id = service_blocks;
    file.link_count = 1;
    file.file_size = 3;
    strcpy(file.filename, "hello.txt");
    write(fd, &file, sizeof(struct ffs_fd));

    file.type = FFS_REG;
    file.datablock_id = service_blocks+1;
    file.link_count = 1;
    file.file_size = 4;
    strcpy(file.filename, "world.txt");
    write(fd, &file, sizeof(struct ffs_fd));

    // FIXME: data
    lseek(fd, FFS_BLOCK_SIZE*service_blocks, SEEK_SET);
    int data = 1;
    write(fd, &data, sizeof(data)); // block count
    data = service_blocks+2;
    write(fd, &data, sizeof(data)); // block index

    lseek(fd, FFS_BLOCK_SIZE*(service_blocks+1), SEEK_SET);
    data = 1;
    write(fd, &data, sizeof(data)); // block count
    data = service_blocks+3;
    write(fd, &data, sizeof(data)); // block index

    lseek(fd, FFS_BLOCK_SIZE*(service_blocks+2), SEEK_SET);
    char str[] = "42\n";
    write(fd, str, 3);

    lseek(fd, FFS_BLOCK_SIZE*(service_blocks+3), SEEK_SET);
    char str2[] = "bla\n";
    write(fd, str2, 4);

    printf("OK\n");
    return 0;
}
