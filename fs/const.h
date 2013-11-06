#ifndef _CONSTS_FFS_H
#define _CONSTS_FFS_H

#define FFS_MAGIC 0xBA0BAB00
#define FFS_BLOCK_SIZE 512

struct ffs_metainfo_sector {
    unsigned int magic;
    unsigned int block_count;
    unsigned int block_size;
    unsigned int fd_blocks;       // blocks for file descriptors
    unsigned int max_file_count;
	unsigned int b_bm_blocks;     // blocks bitmask
    unsigned int fd_bm_blocks;    // file descriptors bitmask
};

enum ffs_file_type {FFS_REG, FFS_DIR, FFS_LINK};

struct ffs_fd { // file descriptor
	char filename[10];
	unsigned int type;
	unsigned int datablock_id;  // block with list of file blocks
	unsigned int link_count;
	unsigned int file_size;
};

#endif /* !_CONSTS_FFS_H */
