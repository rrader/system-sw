#include <linux/module.h>
#include <linux/time.h>
#include <linux/buffer_head.h>
#include "ffs.h"

#define FFS_MAGIC 0xBA0BAB
#define FFS_ROOT_INO 1
#define FFS_FSINFO_INO 2

static const struct inode_operations ffs_dir_inode_operations = {
        // .create         = ffs_create,
        // .lookup         = ffs_lookup,
        // .unlink         = ffs_unlink,
        // .mkdir          = ffs_mkdir,
        // .rmdir          = ffs_rmdir,
        // .rename         = ffs_rename,
        // .setattr        = ffs_setattr,
        // .getattr        = ffs_getattr,
};

static const struct super_operations ffs_sops = {
        // .alloc_inode    = fat_alloc_inode,
        // .destroy_inode  = fat_destroy_inode,
        // .write_inode    = fat_write_inode,
        // .evict_inode    = fat_evict_inode,
        // .put_super      = fat_put_super,
        // .statfs         = fat_statfs,
        // .remount_fs     = fat_remount,

        // .show_options   = fat_show_options,
};
void kernel_msg(struct super_block *sb, const char *level, const char *fmt, ...)
{
    struct va_format vaf;
    va_list args;

    va_start(args, fmt);
    vaf.fmt = fmt;
    vaf.va = &args;
    printk("%sFFS (%s): %pV\n", level, sb->s_id, &vaf);
    va_end(args);
}

static int ffs_fill_super(struct super_block *sb, void *data, int silent)
{
    long error;
    /* filesystem information */
    ffs_sb_info *sbi = kzalloc(sizeof(struct ffs_sb_info), GFP_KERNEL);
    if (!sbi)
        return -ENOMEM;

    sb->s_fs_info = sbi;

    sb->s_magic = FFS_MAGIC;
    sb->s_op = &ffs_sops;
    sb->s_flags |= MS_NODIRATIME | MS_NOATIME;  // no modification time

    // read second block (first for boot sector)
    sb_min_blocksize(sb, 512);
    struct buffer_head *bh = sb_bread(sb, 1);
    // read metainfo to superblock-info
    ffs_metainfo_sector metainfo = (struct ffs_metainfo_sector *) bh->b_data;

    if (metainfo->magic == FFS_MAGIC) {
            kernel_msg(sb, KERN_ERR, "Invalid filesystem (incorrect magic)");
            brelse(bh);
            goto out_fail;
    }

    sb_set_blocksize(sb, metainfo->block_size);
    sbi->block_count = metainfo->block_count;
    sbi->block_size = metainfo->block_size;
    brelse(bh);

    // fill initial state of fs

    struct inode *fsinfo_inode = new_inode(sb);
    if (!fsinfo_inode)
        return -ENOMEM; 
    fsinfo_inode->i_ino = FFS_ROOT_INO;
    insert_inode_hash(fsinfo_inode);

    struct inode *root_inode = new_inode(sb);
    if (!root_inode)
        return -ENOMEM; 
    root_inode->i_ino = FFS_ROOT_INO;
    error = ffs_read_root(root_inode);
    if (error < 0) {
        iput(root_inode);
        goto out_fail;
    }
    insert_inode_hash(root_inode);
    ffs_attach(root_inode, 0);
    sb->s_root = d_make_root(root_inode);
    if (!sb->s_root) {
        kernel_msg(sb, KERN_ERR, "get root inode failed");
        goto out_fail;
    }
    return 0;

out_fail:
        if (fsinfo_inode)
                iput(fsinfo_inode);
        if (fat_inode)
                iput(fat_inode);
        sb->s_fs_info = NULL;
        kfree(sbi);
        return error;
}

static struct dentry *ffs_mount(struct file_system_type *fs_type,
                        int flags, const char *dev_name,
                        void *data)
{
    return mount_bdev(fs_type, flags, dev_name, data, ffs_fill_super);
}

static struct file_system_type ffs_fs_type = {
        .owner          = THIS_MODULE,
        .name           = "ffs",
        .mount          = ffs_mount,
        .kill_sb        = kill_block_super,
        .fs_flags       = FS_REQUIRES_DEV,
};
MODULE_ALIAS_FS("ffs");

static int __init init_ffs_fs(void)
{
    return register_filesystem(&ffs_fs_type);
}

static void __exit exit_ffs_fs(void)
{
    unregister_filesystem(&ffs_fs_type);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Roman Rader");
MODULE_DESCRIPTION("Flat FS");

module_init(init_ffs_fs)
module_exit(exit_ffs_fs)
