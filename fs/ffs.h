#ifndef _FFS_H
#define _FFS_H

struct ffs_sb_info {
    unsigned int block_size;
    unsigned int block_count;
//        unsigned short sec_per_clus; /* sectors/cluster */
//        unsigned short cluster_bits; /* log2(cluster_size) */
//        unsigned int cluster_size; /* cluster size */
//        unsigned char fats, fat_bits; /* number of FATs, FAT bits (12 or 16) */
//        unsigned short fat_start;
//        unsigned long fat_length; /* FAT start & length (sec.) */
//        unsigned long dir_start;
//        unsigned short dir_entries; /* root dir start & entries */
//        unsigned long data_start; /* first data sector */
//        unsigned long max_cluster; /* maximum cluster number */
//        unsigned long root_cluster; /* first cluster of the root directory */
//        unsigned long fsinfo_sector; /* sector number of FAT32 fsinfo */
//        struct mutex fat_lock;
//        struct mutex nfs_build_inode_lock;
//        struct mutex s_lock;
//        unsigned int prev_free; /* previously allocated cluster number */
//        unsigned int free_clusters; /* -1 if undefined */
//        unsigned int free_clus_valid; /* is free_clusters valid? */
//        struct fat_mount_options options;
//        struct nls_table *nls_disk; /* Codepage used on disk */
//        struct nls_table *nls_io; /* Charset used for input and display */
//        const void *dir_ops;         /* Opaque; default directory operations */
//        int dir_per_block;         /* dir entries per block */
//        int dir_per_block_bits;         /* log2(dir_per_block) */
//        unsigned int vol_id;                /*volume ID*/

//        int fatent_shift;
//        struct fatent_operations *fatent_ops;
//        struct inode *fat_inode;
//        struct inode *fsinfo_inode;

//        struct ratelimit_state ratelimit;

//        spinlock_t inode_hash_lock;
//        struct hlist_head inode_hashtable[FAT_HASH_SIZE];

//        spinlock_t dir_hash_lock;
//        struct hlist_head dir_hashtable[FAT_HASH_SIZE];

//        unsigned int dirty; /* fs state before mount */
};

struct ffs_metainfo_sector {
    unsigned int magic;
    unsigned int block_count;
    unsigned int block_size;
};

#endif /* !_FFS_H */