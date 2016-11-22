#include "kpdfs.h"

static int pdfs_fill_super(struct super_block *sb, void *data, int silent) {
    struct inode *root_inode;
    struct pdfs_inode *root_pdfs_inode;
    struct buffer_head *bh;
    struct pdfs_superblock *pdfs_sb;
    int ret = 0;

    bh = sb_bread(sb, PDFS_SUPERBLOCK_BLOCK_NO);
    BUG_ON(!bh);
    pdfs_sb = (struct pdfs_superblock *)bh->b_data;
    if (unlikely(pdfs_sb->magic != PDFS_MAGIC)) {
        printk(KERN_ERR
               "The filesystem being mounted is not of type pdfs. "
               "Magic number mismatch: %llu != %llu\n",
               pdfs_sb->magic, (uint64_t)PDFS_MAGIC);
        goto release;
    }
    if (unlikely(sb->s_blocksize != pdfs_sb->blocksize)) {
        printk(KERN_ERR
               "pdfs seem to be formatted with mismatching blocksize: %lu\n",
               sb->s_blocksize);
        goto release;
    }

    sb->s_magic = pdfs_sb->magic;
    sb->s_fs_info = pdfs_sb;
    sb->s_maxbytes = pdfs_sb->blocksize;
    sb->s_op = &pdfs_sb_ops;

    root_pdfs_inode = pdfs_get_pdfs_inode(sb, PDFS_ROOTDIR_INODE_NO);
    root_inode = new_inode(sb);
    if (!root_inode || !root_pdfs_inode) {
        ret = -ENOMEM;
        goto release;
    }
    pdfs_fill_inode(sb, root_inode, root_pdfs_inode);
    inode_init_owner(root_inode, NULL, root_inode->i_mode);

    sb->s_root = d_make_root(root_inode);
    if (!sb->s_root) {
        ret = -ENOMEM;
        goto release;
    }

release:
    brelse(bh);
    return ret;
}

struct dentry *pdfs_mount(struct file_system_type *fs_type,
                             int flags, const char *dev_name,
                             void *data) {
    struct dentry *ret;
    ret = mount_bdev(fs_type, flags, dev_name, data, pdfs_fill_super);

    if (unlikely(IS_ERR(ret))) {
        printk(KERN_ERR "Error mounting pdfs.\n");
    } else {
        printk(KERN_INFO "pdfs is succesfully mounted on: %s\n",
               dev_name);
    }

    return ret;
}

void pdfs_kill_superblock(struct super_block *sb) {
    printk(KERN_INFO
           "pdfs superblock is destroyed. Unmount succesful.\n");
    kill_block_super(sb);
}

void pdfs_put_super(struct super_block *sb) {
    return;
}

void pdfs_save_sb(struct super_block *sb) {
    struct buffer_head *bh;
    struct pdfs_superblock *pdfs_sb = PDFS_SB(sb);

    bh = sb_bread(sb, PDFS_SUPERBLOCK_BLOCK_NO);
    BUG_ON(!bh);

    bh->b_data = (char *)pdfs_sb;
    mark_buffer_dirty(bh);
    sync_dirty_buffer(bh);
    brelse(bh);
}
