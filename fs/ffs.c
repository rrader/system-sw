#include "ffs.h"

#define FFS_ROOT_INO 1
#define FFS_FSINFO_INO 2

static unsigned int last_ino = 3;

struct inode *user_file_inode;

struct dentry *ffs_lookup(struct inode *parent_inode, struct dentry *dentry, unsigned int flags) {
    // struct super_block *sb = parent_inode->i_sb;
    // struct ffs_sb_info *sbi = sb->s_fs_info;
    // printk(KERN_DEBUG "lookup...\n");
    if (parent_inode->i_ino != FFS_ROOT_INO)
        return ERR_PTR(-ENOENT);

    d_add(dentry, user_file_inode);
    return NULL;
}

int ffs_f_readdir( struct file *file, void *dirent, filldir_t filldir ) {
    int err;
    struct dentry *de = file->f_dentry;

    printk( "ffs: file_operations.readdir called\n" );
    if(file->f_pos > 0 )
        return 1;
    if(filldir(dirent, ".", 1, file->f_pos++, de->d_inode->i_ino, DT_DIR)||
       (filldir(dirent, "..", 2, file->f_pos++, de->d_parent->d_inode->i_ino, DT_DIR)))
        return 0;
    if(filldir(dirent, "hello.txt", 9, file->f_pos++, user_file_inode->i_ino, DT_REG ))
        return 0;
    return 1;
}

static const struct inode_operations ffs_dir_inode_operations = {
        // .create         = ffs_create,
        .lookup         = ffs_lookup,
        // .unlink         = ffs_unlink,
        // .mkdir          = ffs_mkdir,
        // .rmdir          = ffs_rmdir,
        // .rename         = ffs_rename,
        // .setattr        = ffs_setattr,
        // .getattr        = ffs_getattr,
};

struct file_operations ffs_file_fops = {
    // read : &rkfs_f_read,
    // write: &rkfs_f_write
    //    release: &rkfs_f_release
};

struct file_operations ffs_dir_fops = {
    read   : generic_read_dir,
    readdir: &ffs_f_readdir
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

// ssize_t ffs_f_read( struct file *file, char *buf, size_t max, loff_t *offset ) {
//     int i;
//     int buflen;
//     if(*offset > 0)
//         return 0;
//     printk( "rkfs: file_operations.read called %d %d\n", max, *offset );
//     buflen = file_size > max ? max : file_size;
//     __generic_copy_to_user(buf, file_buf, buflen);
//     //           copy_to_user(buf, file_buf, buflen);
//     *offset += buflen; // advance the offset
//     return buflen;
// }

static int ffs_read_root(struct inode *inode)
{
    struct super_block *sb = inode->i_sb;
    printk(KERN_DEBUG "ffs_read_root\n");
    user_file_inode = new_inode(sb);
    user_file_inode->i_ino = last_ino++;
    // user_file_inode->i_size = 6;
    user_file_inode->i_mode = S_IFREG; //|S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH;
    // user_file_inode->i_fop = &ffs_file_fops;
    return 0;
}

static int ffs_fill_super(struct super_block *sb, void *data, int silent)
{
    long error;
    struct ffs_sb_info *sbi;
    struct buffer_head *bh;
    struct ffs_metainfo_sector *metainfo;
    struct inode *fsinfo_inode = 0, *root_inode = 0;
    printk(KERN_DEBUG "ffs_fill_super\n");
    /* filesystem information */
    sbi = kzalloc(sizeof(struct ffs_sb_info), GFP_KERNEL);
    if (!sbi)
        return -ENOMEM;

    sb->s_fs_info = sbi;

    sb->s_magic = FFS_MAGIC;
    sb->s_op = &ffs_sops;
    sb->s_flags |= MS_NODIRATIME | MS_NOATIME;  // no modification time

    // read second block (first for boot sector)
    printk(KERN_DEBUG "reading info block\n");
    sb_min_blocksize(sb, FFS_BLOCK_SIZE);
    bh = sb_bread(sb, 1);
    // read metainfo to superblock-info
    metainfo = (struct ffs_metainfo_sector *) bh->b_data;

    if (metainfo->magic != cpu_to_le32(FFS_MAGIC)) {
        kernel_msg(sb, KERN_ERR, "Invalid filesystem (incorrect magic)");
        kernel_msg(sb, KERN_DEBUG, "magic: %X (required %X)", metainfo->magic, cpu_to_le32(FFS_MAGIC));
        brelse(bh);
        goto out_fail;
    } else {
        kernel_msg(sb, KERN_DEBUG, "magic: OK", metainfo->magic, cpu_to_le32(FFS_MAGIC));
    }

    sb_set_blocksize(sb, metainfo->block_size);
    sbi->block_count = metainfo->block_count;
    sbi->block_size = metainfo->block_size;
    brelse(bh);

    // fill initial state of fs
    fsinfo_inode = new_inode(sb);
    if (!fsinfo_inode)
        return -ENOMEM; 
    fsinfo_inode->i_ino = FFS_FSINFO_INO;
    insert_inode_hash(fsinfo_inode);

    root_inode = new_inode(sb);
    if (!root_inode)
        return -ENOMEM; 
    root_inode->i_ino = FFS_ROOT_INO;
    root_inode->i_version = 1;
    root_inode->i_mode = S_IFDIR | S_IRUGO | S_IXUGO | S_IWUSR;
    root_inode->i_op = &ffs_dir_inode_operations;
    root_inode->i_fop = &ffs_dir_fops;
    error = ffs_read_root(root_inode);
    if (error < 0) {
        iput(root_inode);
        goto out_fail;
    }
    insert_inode_hash(root_inode);
    //ffs_attach(root_inode, 0);
    sb->s_root = d_make_root(root_inode);
    if (!sb->s_root) {
        kernel_msg(sb, KERN_ERR, "get root inode failed");
        goto out_fail;
    }
    printk(KERN_DEBUG "superblock generated\n");
    return 0;

out_fail:
        if (fsinfo_inode)
                iput(fsinfo_inode);
        if (root_inode)
                iput(root_inode);
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
        // .fs_flags       = FS_REQUIRES_DEV,
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
