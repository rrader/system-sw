#ifndef _CONSTS_FFS_H
#define _CONSTS_FFS_H

#define FFS_MAGIC 0xBA0BAB01
#define FFS_BLOCK_SIZE 512

struct ffs_metainfo_sector {
    unsigned int magic;
    unsigned int block_count;
    unsigned int block_size;
};

#endif /* !_CONSTS_FFS_H */