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

struct ffs_sb_info {
    unsigned int block_size;
    unsigned int block_count;
    unsigned int fd_blocks;       // blocks for file descriptors
    unsigned int max_file_count;
    unsigned int b_bm_blocks;     // blocks bitmask
    unsigned int fd_bm_blocks;    // file descriptors bitmask

    char* b_bitmask;
    char* fd_bitmask;
};

struct ffs_inode_info {
    //    __u16        i_mode;                /* File mode */
    //    __u16        i_links_count;        /* Links count */
    //    __u32        i_size;                /* Size in bytes */
    //    __u32        i_atime;        /* Access time */
    //    __u32        i_ctime;        /* Creation time */
    //    __u32        i_mtime;        /* Modification time */
    //    __u32        i_dtime;        /* Deletion Time */
    //    __u32          i_gid;                /* Low 16 bits of Group Id */
    //    __u32        i_uid;                /* Low 16 bits of Owner Uid */
    // __u32        i_blocks;        /* Blocks count */
    // __le32       i_block[LAB4FS_N_BLOCKS];/* Pointers to blocks */
    //    __u32        i_file_acl;        /* File ACL */
    //    __u32        i_dir_acl;        /* Directory ACL */
    // unsigned i_dir_start_lookup;
    // rwlock_t rwlock;
    struct inode vfs_inode;
    struct ffs_fd fd;
    // struct buffer_head *bh;
};

#endif /* !_FFS_H */
