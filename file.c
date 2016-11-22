#include "kpdfs.h"

ssize_t pdfs_read(struct file *filp, char __user *buf, size_t len,
                     loff_t *ppos) {
    struct super_block *sb;
    struct inode *inode;
    struct pdfs_inode *pdfs_inode;
    struct buffer_head *bh;
    char *buffer;
    int nbytes;

    inode = filp->f_path.dentry->d_inode;
    sb = inode->i_sb;
    pdfs_inode = PDFS_INODE(inode);
    
    if (*ppos >= pdfs_inode->file_size) {
        return 0;
    }

    bh = sb_bread(sb, pdfs_inode->data_block_no);
    if (!bh) {
        printk(KERN_ERR "Failed to read data block %llu\n",
               pdfs_inode->data_block_no);
        return 0;
    }

    buffer = (char *)bh->b_data + *ppos;
    nbytes = min((size_t)(pdfs_inode->file_size - *ppos), len);

    if (copy_to_user(buf, buffer, nbytes)) {
        brelse(bh);
        printk(KERN_ERR
               "Error copying file content to userspace buffer\n");
        return -EFAULT;
    }

    brelse(bh);
    *ppos += nbytes;
    return nbytes;
}

/* TODO We didn't use address_space/pagecache here.
   If we hook file_operations.write = do_sync_write,
   and file_operations.aio_write = generic_file_aio_write,
   we will use write to pagecache instead. */
ssize_t pdfs_write(struct file *filp, const char __user *buf, size_t len,
                      loff_t *ppos) {
    struct super_block *sb;
    struct inode *inode;
    struct pdfs_inode *pdfs_inode;
    struct buffer_head *bh;
    struct pdfs_superblock *pdfs_sb;
    char *buffer;
    int ret;

    inode = filp->f_path.dentry->d_inode;
    sb = inode->i_sb;
    pdfs_inode = PDFS_INODE(inode);
    pdfs_sb = PDFS_SB(sb);

    ret = generic_write_checks(filp, ppos, &len, 0);
    if (ret) {
        return ret;
    }

    bh = sb_bread(sb, pdfs_inode->data_block_no);
    if (!bh) {
        printk(KERN_ERR "Failed to read data block %llu\n",
               pdfs_inode->data_block_no);
        return 0;
    }

    buffer = (char *)bh->b_data + *ppos;
    if (copy_from_user(buffer, buf, len)) {
        brelse(bh);
        printk(KERN_ERR
               "Error copying file content from userspace buffer "
               "to kernel space\n");
        return -EFAULT;
    }
    *ppos += len;

    mark_buffer_dirty(bh);
    sync_dirty_buffer(bh);
    brelse(bh);

    pdfs_inode->file_size = max((size_t)(pdfs_inode->file_size),
                                   (size_t)(*ppos));
    pdfs_save_pdfs_inode(sb, pdfs_inode);

    /* TODO We didn't update file size here. To be frank I don't know how. */

    return len;
}
