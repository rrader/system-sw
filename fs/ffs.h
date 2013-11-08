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

#define CHECK_BIT(bm, n) (*(char*)((char*)bm + ((n)/8)) & (char)(1 << (n % 8)))

struct ffs_sb_info {
    unsigned int block_size;
    unsigned int block_count;
    unsigned int fd_blocks;       // blocks for file descriptors
    unsigned int max_file_count;
    unsigned int b_bm_blocks;     // blocks bitmask
    unsigned int fd_bm_blocks;    // file descriptors bitmask

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
