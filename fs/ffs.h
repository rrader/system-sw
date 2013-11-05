#ifndef _FFS_H
#define _FFS_H

#include <linux/fs.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/buffer_head.h>
#include <linux/slab.h>
#include "const.h"

struct ffs_sb_info {
    unsigned int block_size;
    unsigned int block_count;
};

#endif /* !_FFS_H */
