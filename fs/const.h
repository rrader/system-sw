#ifndef _CONSTS_FFS_H
#define _CONSTS_FFS_H

#define FFS_MAGIC 0xBA0BAB00
#define FFS_BLOCK_SIZE 512
#define FFS_FILENAME_LENGTH 10

struct ffs_metainfo_sector {
    unsigned int magic;
    unsigned int block_count;
    unsigned int block_size;
    unsigned int fd_blocks;       // blocks for file descriptors
    unsigned int max_file_count;
    unsigned int b_bm_blocks;     // blocks bitmask
    unsigned int fd_bm_blocks;    // file descriptors bitmask
};

enum ffs_file_type {FFS_REG, FFS_DIR, FFS_HLINK, FFS_SLINK};

struct ffs_fd { // file descriptor
    unsigned int type;
    unsigned int datablock_id;  // block with list of file blocks
    unsigned int link_count;
    unsigned int file_size;
};

struct ffs_directory_entry {
    unsigned int i_fd;  // file descriptor id. numbering from 1, 0 is empty
    char filename[FFS_FILENAME_LENGTH];
};

#endif /* !_CONSTS_FFS_H */
