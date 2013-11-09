#ifndef _FFS_H
#define _FFS_H

#include <linux/fs.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/buffer_head.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/limits.h>
#include "const.h"

#define BITMASK_BYTE(n) ((n)/8)
#define BITMASK_PAGE(n) (BITMASK_BYTE(n)/FFS_BLOCK_SIZE)
#define CHECK_BIT(bm, n) (*(char*)((char*)(bm) + ((n)/8)) & (char)(1 << ((n) % 8)))
#define SET_BIT(bm, n) *((char*)(bm) + ((n)/8)) = (*((char*)(bm) + ((n)/8)) | (char)(1 << ((n) % 8)))

#define FFS_FD_BITMASK_BLOCK(sbi) (1 + sbi->b_bm_blocks)
#define FFS_B_BITMASK_BLOCK(sbi) (1)

struct ffs_sb_info {
    unsigned int block_size;
    unsigned int block_count;
    unsigned int fd_blocks;       // blocks for file descriptors
    unsigned int max_file_count;
    unsigned int b_bm_blocks;     // blocks bitmask
    unsigned int fd_bm_blocks;    // file descriptors bitmask

    unsigned int fd_bitmask_len;
    unsigned int b_bitmask_len;

    char* b_bitmask;
    char* fd_bitmask;
    struct hlist_head inodes;
};

struct ffs_inode_info {
    struct inode vfs_inode;
    struct ffs_fd fd;
    struct hlist_node list_node;
    struct buffer_head *datablock;
};

#endif /* !_FFS_H */
