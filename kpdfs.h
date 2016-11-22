#ifndef __KPDFS_H__
#define __KPDFS_H__

/* kpdfs.h defines symbols to work in kernel space */

#include <linux/blkdev.h>
#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/namei.h>
#include <linux/module.h>
#include <linux/parser.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/version.h>

#include "pdfs.h"

/* Declare operations to be hooked to VFS */

extern struct file_system_type pdfs_fs_type;
extern const struct super_operations pdfs_sb_ops;
extern const struct inode_operations pdfs_inode_ops;
extern const struct file_operations pdfs_dir_operations;
extern const struct file_operations pdfs_file_operations;

struct dentry *pdfs_mount(struct file_system_type *fs_type,
                              int flags, const char *dev_name,
                              void *data);
void pdfs_kill_superblock(struct super_block *sb);

void pdfs_destroy_inode(struct inode *inode);
void pdfs_put_super(struct super_block *sb);

int pdfs_create(struct inode *dir, struct dentry *dentry,
                    umode_t mode, bool excl);
struct dentry *pdfs_lookup(struct inode *parent_inode,
                               struct dentry *child_dentry,
                               unsigned int flags);
int pdfs_mkdir(struct inode *dir, struct dentry *dentry,
                   umode_t mode);

int pdfs_readdir(struct file *filp, void *dirent, filldir_t filldir);

ssize_t pdfs_read(struct file * filp, char __user * buf, size_t len,
                      loff_t * ppos);
ssize_t pdfs_write(struct file * filp, const char __user * buf, size_t len,
                       loff_t * ppos);

extern struct kmem_cache *pdfs_inode_cache;

/* Helper functions */

// To translate VFS superblock to pdfs superblock
static inline struct pdfs_superblock *PDFS_SB(struct super_block *sb) {
    return sb->s_fs_info;
}
static inline struct pdfs_inode *PDFS_INODE(struct inode *inode) {
    return inode->i_private;
}

static inline uint64_t PDFS_INODES_PER_BLOCK(struct super_block *sb) {
    struct pdfs_superblock *pdfs_sb;
    pdfs_sb = PDFS_SB(sb);
    return PDFS_INODES_PER_BLOCK_HSB(pdfs_sb);
}

// Given the inode_no, calcuate which block in inode table contains the corresponding inode
static inline uint64_t PDFS_INODE_BLOCK_OFFSET(struct super_block *sb, uint64_t inode_no) {
    struct pdfs_superblock *pdfs_sb;
    pdfs_sb = PDFS_SB(sb);
    return inode_no / PDFS_INODES_PER_BLOCK_HSB(pdfs_sb);
}
static inline uint64_t PDFS_INODE_BYTE_OFFSET(struct super_block *sb, uint64_t inode_no) {
    struct pdfs_superblock *pdfs_sb;
    pdfs_sb = PDFS_SB(sb);
    return (inode_no % PDFS_INODES_PER_BLOCK_HSB(pdfs_sb)) * sizeof(struct pdfs_inode);
}

static inline uint64_t PDFS_DIR_MAX_RECORD(struct super_block *sb) {
    struct pdfs_superblock *pdfs_sb;
    pdfs_sb = PDFS_SB(sb);
    return pdfs_sb->blocksize / sizeof(struct pdfs_dir_record);
}

// From which block does data blocks start
static inline uint64_t PDFS_DATA_BLOCK_TABLE_START_BLOCK_NO(struct super_block *sb) {
    struct pdfs_superblock *pdfs_sb;
    pdfs_sb = PDFS_SB(sb);
    return PDFS_DATA_BLOCK_TABLE_START_BLOCK_NO_HSB(pdfs_sb);
}

void pdfs_save_sb(struct super_block *sb);

// functions to operate inode
void pdfs_fill_inode(struct super_block *sb, struct inode *inode,
                        struct pdfs_inode *pdfs_inode);
int pdfs_alloc_pdfs_inode(struct super_block *sb, uint64_t *out_inode_no);
struct pdfs_inode *pdfs_get_pdfs_inode(struct super_block *sb,
                                                uint64_t inode_no);
void pdfs_save_pdfs_inode(struct super_block *sb,
                                struct pdfs_inode *inode);
int pdfs_add_dir_record(struct super_block *sb, struct inode *dir,
                           struct dentry *dentry, struct inode *inode);
int pdfs_alloc_data_block(struct super_block *sb, uint64_t *out_data_block_no);
int pdfs_create_inode(struct inode *dir, struct dentry *dentry,
                         umode_t mode);

#endif /*__KPDFS_H__*/
