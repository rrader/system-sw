int fat_fill_super(struct super_block *sb, void *data, int silent, int isvfat,
                 void (*setup)(struct super_block *))
{
        struct inode *root_inode = NULL, *fat_inode = NULL;
        struct inode *fsinfo_inode = NULL;
        struct buffer_head *bh;
        struct fat_boot_sector *b;
        struct msdos_sb_info *sbi;
        u16 logical_sector_size;
        u32 total_sectors, total_clusters, fat_clusters, rootdir_sectors;
        int debug;
        unsigned int media;
        long error;
        char buf[50];

        /*
         * GFP_KERNEL is ok here, because while we do hold the
         * supeblock lock, memory pressure can't call back into
         * the filesystem, since we're only just about to mount
         * it and have no inodes etc active!
         */
        sbi = kzalloc(sizeof(struct msdos_sb_info), GFP_KERNEL);
        if (!sbi)
                return -ENOMEM;
        sb->s_fs_info = sbi;

        sb->s_flags |= MS_NODIRATIME;
        sb->s_magic = MSDOS_SUPER_MAGIC;
        sb->s_op = &fat_sops;
        sb->s_export_op = &fat_export_ops;
        mutex_init(&sbi->nfs_build_inode_lock);
        ratelimit_state_init(&sbi->ratelimit, DEFAULT_RATELIMIT_INTERVAL,
                         DEFAULT_RATELIMIT_BURST);

        error = parse_options(sb, data, isvfat, silent, &debug, &sbi->options);
        if (error)
                goto out_fail;

        setup(sb); /* flavour-specific stuff that needs options */

        error = -EIO;
        sb_min_blocksize(sb, 512);
        bh = sb_bread(sb, 0);
        if (bh == NULL) {
                fat_msg(sb, KERN_ERR, "unable to read boot sector");
                goto out_fail;
        }

        b = (struct fat_boot_sector *) bh->b_data;
        if (!b->reserved) {
                if (!silent)
                        fat_msg(sb, KERN_ERR, "bogus number of reserved sectors");
                brelse(bh);
                goto out_invalid;
        }
        if (!b->fats) {
                if (!silent)
                        fat_msg(sb, KERN_ERR, "bogus number of FAT structure");
                brelse(bh);
                goto out_invalid;
        }

        /*
         * Earlier we checked here that b->secs_track and b->head are nonzero,
         * but it turns out valid FAT filesystems can have zero there.
         */

        media = b->media;
        if (!fat_valid_media(media)) {
                if (!silent)
                        fat_msg(sb, KERN_ERR, "invalid media value (0x%02x)",
                         media);
                brelse(bh);
                goto out_invalid;
        }
        logical_sector_size = get_unaligned_le16(&b->sector_size);
        if (!is_power_of_2(logical_sector_size)
         || (logical_sector_size < 512)
         || (logical_sector_size > 4096)) {
                if (!silent)
                        fat_msg(sb, KERN_ERR, "bogus logical sector size %u",
                         logical_sector_size);
                brelse(bh);
                goto out_invalid;
        }
        sbi->sec_per_clus = b->sec_per_clus;
        if (!is_power_of_2(sbi->sec_per_clus)) {
                if (!silent)
                        fat_msg(sb, KERN_ERR, "bogus sectors per cluster %u",
                         sbi->sec_per_clus);
                brelse(bh);
                goto out_invalid;
        }

        if (logical_sector_size < sb->s_blocksize) {
                fat_msg(sb, KERN_ERR, "logical sector size too small for device"
                 " (logical sector size = %u)", logical_sector_size);
                brelse(bh);
                goto out_fail;
        }
        if (logical_sector_size > sb->s_blocksize) {
                brelse(bh);

                if (!sb_set_blocksize(sb, logical_sector_size)) {
                        fat_msg(sb, KERN_ERR, "unable to set blocksize %u",
                         logical_sector_size);
                        goto out_fail;
                }
                bh = sb_bread(sb, 0);
                if (bh == NULL) {
                        fat_msg(sb, KERN_ERR, "unable to read boot sector"
                         " (logical sector size = %lu)",
                         sb->s_blocksize);
                        goto out_fail;
                }
                b = (struct fat_boot_sector *) bh->b_data;
        }

        mutex_init(&sbi->s_lock);
        sbi->cluster_size = sb->s_blocksize * sbi->sec_per_clus;
        sbi->cluster_bits = ffs(sbi->cluster_size) - 1;
        sbi->fats = b->fats;
        sbi->fat_bits = 0;                /* Don't know yet */
        sbi->fat_start = le16_to_cpu(b->reserved);
        sbi->fat_length = le16_to_cpu(b->fat_length);
        sbi->root_cluster = 0;
        sbi->free_clusters = -1;        /* Don't know yet */
        sbi->free_clus_valid = 0;
        sbi->prev_free = FAT_START_ENT;
        sb->s_maxbytes = 0xffffffff;

        if (!sbi->fat_length && b->fat32.length) {
                struct fat_boot_fsinfo *fsinfo;
                struct buffer_head *fsinfo_bh;

                /* Must be FAT32 */
                sbi->fat_bits = 32;
                sbi->fat_length = le32_to_cpu(b->fat32.length);
                sbi->root_cluster = le32_to_cpu(b->fat32.root_cluster);

                /* MC - if info_sector is 0, don't multiply by 0 */
                sbi->fsinfo_sector = le16_to_cpu(b->fat32.info_sector);
                if (sbi->fsinfo_sector == 0)
                        sbi->fsinfo_sector = 1;

                fsinfo_bh = sb_bread(sb, sbi->fsinfo_sector);
                if (fsinfo_bh == NULL) {
                        fat_msg(sb, KERN_ERR, "bread failed, FSINFO block"
                         " (sector = %lu)", sbi->fsinfo_sector);
                        brelse(bh);
                        goto out_fail;
                }

                fsinfo = (struct fat_boot_fsinfo *)fsinfo_bh->b_data;
                if (!IS_FSINFO(fsinfo)) {
                        fat_msg(sb, KERN_WARNING, "Invalid FSINFO signature: "
                         "0x%08x, 0x%08x (sector = %lu)",
                         le32_to_cpu(fsinfo->signature1),
                         le32_to_cpu(fsinfo->signature2),
                         sbi->fsinfo_sector);
                } else {
                        if (sbi->options.usefree)
                                sbi->free_clus_valid = 1;
                        sbi->free_clusters = le32_to_cpu(fsinfo->free_clusters);
                        sbi->prev_free = le32_to_cpu(fsinfo->next_cluster);
                }

                brelse(fsinfo_bh);
        }

        /* interpret volume ID as a little endian 32 bit integer */
        if (sbi->fat_bits == 32)
                sbi->vol_id = (((u32)b->fat32.vol_id[0]) |
                                        ((u32)b->fat32.vol_id[1] << 8) |
                                        ((u32)b->fat32.vol_id[2] << 16) |
                                        ((u32)b->fat32.vol_id[3] << 24));
        else /* fat 16 or 12 */
                sbi->vol_id = (((u32)b->fat16.vol_id[0]) |
                                        ((u32)b->fat16.vol_id[1] << 8) |
                                        ((u32)b->fat16.vol_id[2] << 16) |
                                        ((u32)b->fat16.vol_id[3] << 24));

        sbi->dir_per_block = sb->s_blocksize / sizeof(struct msdos_dir_entry);
        sbi->dir_per_block_bits = ffs(sbi->dir_per_block) - 1;

        sbi->dir_start = sbi->fat_start + sbi->fats * sbi->fat_length;
        sbi->dir_entries = get_unaligned_le16(&b->dir_entries);
        if (sbi->dir_entries & (sbi->dir_per_block - 1)) {
                if (!silent)
                        fat_msg(sb, KERN_ERR, "bogus directory-entries per block"
                         " (%u)", sbi->dir_entries);
                brelse(bh);
                goto out_invalid;
        }

        rootdir_sectors = sbi->dir_entries
                * sizeof(struct msdos_dir_entry) / sb->s_blocksize;
        sbi->data_start = sbi->dir_start + rootdir_sectors;
        total_sectors = get_unaligned_le16(&b->sectors);
        if (total_sectors == 0)
                total_sectors = le32_to_cpu(b->total_sect);

        total_clusters = (total_sectors - sbi->data_start) / sbi->sec_per_clus;

        if (sbi->fat_bits != 32)
                sbi->fat_bits = (total_clusters > MAX_FAT12) ? 16 : 12;

        /* some OSes set FAT_STATE_DIRTY and clean it on unmount. */
        if (sbi->fat_bits == 32)
                sbi->dirty = b->fat32.state & FAT_STATE_DIRTY;
        else /* fat 16 or 12 */
                sbi->dirty = b->fat16.state & FAT_STATE_DIRTY;

        /* check that FAT table does not overflow */
        fat_clusters = calc_fat_clusters(sb);
        total_clusters = min(total_clusters, fat_clusters - FAT_START_ENT);
        if (total_clusters > MAX_FAT(sb)) {
                if (!silent)
                        fat_msg(sb, KERN_ERR, "count of clusters too big (%u)",
                         total_clusters);
                brelse(bh);
                goto out_invalid;
        }

        sbi->max_cluster = total_clusters + FAT_START_ENT;
        /* check the free_clusters, it's not necessarily correct */
        if (sbi->free_clusters != -1 && sbi->free_clusters > total_clusters)
                sbi->free_clusters = -1;
        /* check the prev_free, it's not necessarily correct */
        sbi->prev_free %= sbi->max_cluster;
        if (sbi->prev_free < FAT_START_ENT)
                sbi->prev_free = FAT_START_ENT;

        brelse(bh);

        /* set up enough so that it can read an inode */
        fat_hash_init(sb);
        dir_hash_init(sb);
        fat_ent_access_init(sb);

        /*
         * The low byte of FAT's first entry must have same value with
         * media-field. But in real world, too many devices is
         * writing wrong value. So, removed that validity check.
         *
         * if (FAT_FIRST_ENT(sb, media) != first)
         */

        error = -EINVAL;
        sprintf(buf, "cp%d", sbi->options.codepage);
        sbi->nls_disk = load_nls(buf);
        if (!sbi->nls_disk) {
                fat_msg(sb, KERN_ERR, "codepage %s not found", buf);
                goto out_fail;
        }

        /* FIXME: utf8 is using iocharset for upper/lower conversion */
        if (sbi->options.isvfat) {
                sbi->nls_io = load_nls(sbi->options.iocharset);
                if (!sbi->nls_io) {
                        fat_msg(sb, KERN_ERR, "IO charset %s not found",
                         sbi->options.iocharset);
                        goto out_fail;
                }
        }

        error = -ENOMEM;
        fat_inode = new_inode(sb);
        if (!fat_inode)
                goto out_fail;
        MSDOS_I(fat_inode)->i_pos = 0;
        sbi->fat_inode = fat_inode;

        fsinfo_inode = new_inode(sb);
        if (!fsinfo_inode)
                goto out_fail;
        fsinfo_inode->i_ino = MSDOS_FSINFO_INO;
        sbi->fsinfo_inode = fsinfo_inode;
        insert_inode_hash(fsinfo_inode);

        root_inode = new_inode(sb);
        if (!root_inode)
                goto out_fail;
        root_inode->i_ino = MSDOS_ROOT_INO;
        root_inode->i_version = 1;
        error = fat_read_root(root_inode);
        if (error < 0) {
                iput(root_inode);
                goto out_fail;
        }
        error = -ENOMEM;
        insert_inode_hash(root_inode);
        fat_attach(root_inode, 0);
        sb->s_root = d_make_root(root_inode);
        if (!sb->s_root) {
                fat_msg(sb, KERN_ERR, "get root inode failed");
                goto out_fail;
        }

        if (sbi->options.discard) {
                struct request_queue *q = bdev_get_queue(sb->s_bdev);
                if (!blk_queue_discard(q))
                        fat_msg(sb, KERN_WARNING,
                                        "mounting with \"discard\" option, but "
                                        "the device does not support discard");
        }

        fat_set_state(sb, 1, 0);
        return 0;

out_invalid:
        error = -EINVAL;
        if (!silent)
                fat_msg(sb, KERN_INFO, "Can't find a valid FAT filesystem");

out_fail:
        if (fsinfo_inode)
                iput(fsinfo_inode);
        if (fat_inode)
                iput(fat_inode);
        unload_nls(sbi->nls_io);
        unload_nls(sbi->nls_disk);
        if (sbi->options.iocharset != fat_default_iocharset)
                kfree(sbi->options.iocharset);
        sb->s_fs_info = NULL;
        kfree(sbi);
        return error;
}