#include "kpdfs.h"

void pdfs_destroy_inode(struct inode *inode) {
    struct pdfs_inode *pdfs_inode = PDFS_INODE(inode);

    printk(KERN_INFO "Freeing private data of inode %p (%lu)\n",
           pdfs_inode, inode->i_ino);
    kmem_cache_free(pdfs_inode_cache, pdfs_inode);
}

void pdfs_fill_inode(struct super_block *sb, struct inode *inode,
                        struct pdfs_inode *pdfs_inode) {
    inode->i_mode = pdfs_inode->mode;
    inode->i_sb = sb;
    inode->i_ino = pdfs_inode->inode_no;
    inode->i_op = &pdfs_inode_ops;
    // TODO hope we can use pdfs_inode to store timespec
    inode->i_atime = inode->i_mtime 
                   = inode->i_ctime
                   = CURRENT_TIME;
    inode->i_private = pdfs_inode;    
    
    if (S_ISDIR(pdfs_inode->mode)) {
        inode->i_fop = &pdfs_dir_operations;
    } else if (S_ISREG(pdfs_inode->mode)) {
        inode->i_fop = &pdfs_file_operations;
    } else {
        printk(KERN_WARNING
               "Inode %lu is neither a directory nor a regular file",
               inode->i_ino);
        inode->i_fop = NULL;
    }

    /* TODO pdfs_inode->file_size seems not reflected in inode */
}

/* TODO I didn't implement any function to dealloc pdfs_inode */
int pdfs_alloc_pdfs_inode(struct super_block *sb, uint64_t *out_inode_no) {
    struct pdfs_superblock *pdfs_sb;
    struct buffer_head *bh;
    uint64_t i;
    int ret;
    char *bitmap;
    char *slot;
    char needle;

    pdfs_sb = PDFS_SB(sb);

    mutex_lock(&pdfs_sb_lock);

    bh = sb_bread(sb, PDFS_INODE_BITMAP_BLOCK_NO);
    BUG_ON(!bh);

    bitmap = bh->b_data;
    ret = -ENOSPC;
    for (i = 0; i < pdfs_sb->inode_table_size; i++) {
        slot = bitmap + i / BITS_IN_BYTE;
        needle = 1 << (i % BITS_IN_BYTE);
        if (0 == (*slot & needle)) {
            *out_inode_no = i;
            *slot |= needle;
            pdfs_sb->inode_count += 1;
            ret = 0;
            break;
        }
    }

    mark_buffer_dirty(bh);
    sync_dirty_buffer(bh);
    brelse(bh);
    pdfs_save_sb(sb);

    mutex_unlock(&pdfs_sb_lock);
    return ret;
}

struct pdfs_inode *pdfs_get_pdfs_inode(struct super_block *sb,
                                                uint64_t inode_no) {
    struct buffer_head *bh;
    struct pdfs_inode *inode;
    struct pdfs_inode *inode_buf;

    bh = sb_bread(sb, PDFS_INODE_TABLE_START_BLOCK_NO + PDFS_INODE_BLOCK_OFFSET(sb, inode_no));
    BUG_ON(!bh);
    
    inode = (struct pdfs_inode *)(bh->b_data + PDFS_INODE_BYTE_OFFSET(sb, inode_no));
    inode_buf = kmem_cache_alloc(pdfs_inode_cache, GFP_KERNEL);
    memcpy(inode_buf, inode, sizeof(*inode_buf));

    brelse(bh);
    return inode_buf;
}

void pdfs_save_pdfs_inode(struct super_block *sb,
                                struct pdfs_inode *inode_buf) {
    struct buffer_head *bh;
    struct pdfs_inode *inode;
    uint64_t inode_no;

    inode_no = inode_buf->inode_no;
    bh = sb_bread(sb, PDFS_INODE_TABLE_START_BLOCK_NO + PDFS_INODE_BLOCK_OFFSET(sb, inode_no));
    BUG_ON(!bh);

    inode = (struct pdfs_inode *)(bh->b_data + PDFS_INODE_BYTE_OFFSET(sb, inode_no));
    memcpy(inode, inode_buf, sizeof(*inode));

    mark_buffer_dirty(bh);
    sync_dirty_buffer(bh);
    brelse(bh);
}

int pdfs_add_dir_record(struct super_block *sb, struct inode *dir,
                           struct dentry *dentry, struct inode *inode) {
    struct buffer_head *bh;
    struct pdfs_inode *parent_pdfs_inode;
    struct pdfs_dir_record *dir_record;

    parent_pdfs_inode = PDFS_INODE(dir);
    if (unlikely(parent_pdfs_inode->dir_children_count
            >= PDFS_DIR_MAX_RECORD(sb))) {
        return -ENOSPC;
    }

    bh = sb_bread(sb, parent_pdfs_inode->data_block_no);
    BUG_ON(!bh);

    dir_record = (struct pdfs_dir_record *)bh->b_data;
    dir_record += parent_pdfs_inode->dir_children_count;
    dir_record->inode_no = inode->i_ino;
    strcpy(dir_record->filename, dentry->d_name.name);

    mark_buffer_dirty(bh);
    sync_dirty_buffer(bh);
    brelse(bh);

    parent_pdfs_inode->dir_children_count += 1;
    pdfs_save_pdfs_inode(sb, parent_pdfs_inode);

    return 0;
}

int pdfs_alloc_data_block(struct super_block *sb, uint64_t *out_data_block_no) {
    struct pdfs_superblock *pdfs_sb;
    struct buffer_head *bh;
    uint64_t i;
    int ret;
    char *bitmap;
    char *slot;
    char needle;

    pdfs_sb = PDFS_SB(sb);

    mutex_lock(&pdfs_sb_lock);

    bh = sb_bread(sb, PDFS_DATA_BLOCK_BITMAP_BLOCK_NO);
    BUG_ON(!bh);

    bitmap = bh->b_data;
    ret = -ENOSPC;
    for (i = 0; i < pdfs_sb->data_block_table_size; i++) {
        slot = bitmap + i / BITS_IN_BYTE;
        needle = 1 << (i % BITS_IN_BYTE);
        if (0 == (*slot & needle)) {
            *out_data_block_no
                = PDFS_DATA_BLOCK_TABLE_START_BLOCK_NO(sb) + i;
            *slot |= needle;
            pdfs_sb->data_block_count += 1;
            ret = 0;
            break;
        }
    }

    mark_buffer_dirty(bh);
    sync_dirty_buffer(bh);
    brelse(bh);
    pdfs_save_sb(sb);

    mutex_unlock(&pdfs_sb_lock);
    return ret;
}

int pdfs_create_inode(struct inode *dir, struct dentry *dentry,
                         umode_t mode) {
    struct super_block *sb;
    struct pdfs_superblock *pdfs_sb;
    uint64_t inode_no;
    struct pdfs_inode *pdfs_inode;
    struct inode *inode;
    int ret;

    sb = dir->i_sb;
    pdfs_sb = PDFS_SB(sb);

    /* Create pdfs_inode */
    ret = pdfs_alloc_pdfs_inode(sb, &inode_no);
    if (0 != ret) {
        printk(KERN_ERR "Unable to allocate on-disk inode. "
                        "Is inode table full? "
                        "Inode count: %llu\n",
                        pdfs_sb->inode_count);
        return -ENOSPC;
    }
    pdfs_inode = kmem_cache_alloc(pdfs_inode_cache, GFP_KERNEL);
    pdfs_inode->inode_no = inode_no;
    pdfs_inode->mode = mode;
    if (S_ISDIR(mode)) {
        pdfs_inode->dir_children_count = 0;
    } else if (S_ISREG(mode)) {
        pdfs_inode->file_size = 0;
    } else {
        printk(KERN_WARNING
               "Inode %llu is neither a directory nor a regular file",
               inode_no);
    }

    /* Allocate data block for the new pdfs_inode */
    ret = pdfs_alloc_data_block(sb, &pdfs_inode->data_block_no);
    if (0 != ret) {
        printk(KERN_ERR "Unable to allocate on-disk data block. "
                        "Is data block table full? "
                        "Data block count: %llu\n",
                        pdfs_sb->data_block_count);
        return -ENOSPC;
    }

    /* Create VFS inode */
    inode = new_inode(sb);
    if (!inode) {
        return -ENOMEM;
    }
    pdfs_fill_inode(sb, inode, pdfs_inode);

    /* Add new inode to parent dir */
    ret = pdfs_add_dir_record(sb, dir, dentry, inode);
    if (0 != ret) {
        printk(KERN_ERR "Failed to add inode %lu to parent dir %lu\n",
               inode->i_ino, dir->i_ino);
        return -ENOSPC;
    }

    inode_init_owner(inode, dir, mode);
    d_add(dentry, inode);

    /* TODO we should free newly allocated inodes when error occurs */

    return 0;
}

int pdfs_create(struct inode *dir, struct dentry *dentry,
                   umode_t mode, bool excl) {
    return pdfs_create_inode(dir, dentry, mode);
}

int pdfs_mkdir(struct inode *dir, struct dentry *dentry,
                  umode_t mode) {
    /* @Sankar: The mkdir callback does not have S_IFDIR set.
       Even ext2 sets it explicitly. Perhaps this is a bug */
    mode |= S_IFDIR;
    return pdfs_create_inode(dir, dentry, mode);
}

struct dentry *pdfs_lookup(struct inode *dir,
                              struct dentry *child_dentry,
                              unsigned int flags) {
    struct pdfs_inode *parent_pdfs_inode = PDFS_INODE(dir);
    struct super_block *sb = dir->i_sb;
    struct buffer_head *bh;
    struct pdfs_dir_record *dir_record;
    struct pdfs_inode *pdfs_child_inode;
    struct inode *child_inode;
    uint64_t i;

    bh = sb_bread(sb, parent_pdfs_inode->data_block_no);
    BUG_ON(!bh);

    dir_record = (struct pdfs_dir_record *)bh->b_data;

    for (i = 0; i < parent_pdfs_inode->dir_children_count; i++) {
        printk(KERN_INFO "pdfs_lookup: i=%llu, dir_record->filename=%s, child_dentry->d_name.name=%s", i, dir_record->filename, child_dentry->d_name.name);    // TODO
        if (0 == strcmp(dir_record->filename, child_dentry->d_name.name)) {
            pdfs_child_inode = pdfs_get_pdfs_inode(sb, dir_record->inode_no);
            child_inode = new_inode(sb);
            if (!child_inode) {
                printk(KERN_ERR "Cannot create new inode. No memory.\n");
                return NULL; 
            }
            pdfs_fill_inode(sb, child_inode, pdfs_child_inode);
            inode_init_owner(child_inode, dir, pdfs_child_inode->mode);
            d_add(child_dentry, child_inode);
            return NULL;    
        }
        dir_record++;
    }

    printk(KERN_ERR
           "No inode found for the filename: %s\n",
           child_dentry->d_name.name);
    return NULL;
}
