#ifndef __PDFS_H__
#define __PDFS_H__

#define BITS_IN_BYTE 8
#define PDFS_MAGIC 0x19690716
#define PDFS_DEFAULT_BLOCKSIZE 4096
#define PDFS_DEFAULT_INODE_TABLE_SIZE 1024
#define PDFS_DEFAULT_DATA_BLOCK_TABLE_SIZE 1024
#define PDFS_FILENAME_MAXLEN 255

/* Define filesystem structures */

extern struct mutex pdfs_sb_lock;

struct pdfs_dir_record {
    char filename[PDFS_FILENAME_MAXLEN];
    uint64_t inode_no;
};

struct pdfs_inode {
    mode_t mode;
    uint64_t inode_no;
    uint64_t data_block_no;

    // TODO struct timespec is defined kenrel space,
    // but mkfs-pdfs.c is compiled in user space
    /*struct timespec atime;
    struct timespec mtime;
    struct timespec ctime;*/

    union {
        uint64_t file_size;
        uint64_t dir_children_count;
    };
};

struct pdfs_superblock {
    uint64_t version;
    uint64_t magic;
    uint64_t blocksize;

    uint64_t inode_table_size;
    uint64_t inode_count;

    uint64_t data_block_table_size;
    uint64_t data_block_count;
};

static const uint64_t PDFS_SUPERBLOCK_BLOCK_NO = 0;
static const uint64_t PDFS_INODE_BITMAP_BLOCK_NO = 1;
static const uint64_t PDFS_DATA_BLOCK_BITMAP_BLOCK_NO = 2;
static const uint64_t PDFS_INODE_TABLE_START_BLOCK_NO = 3;

static const uint64_t PDFS_ROOTDIR_INODE_NO = 0;
// data block no is the absolute block number from start of device
// data block no offset is the relative block offset from start of data block table
static const uint64_t PDFS_ROOTDIR_DATA_BLOCK_NO_OFFSET = 0;

/* Helper functions */

static inline uint64_t PDFS_INODES_PER_BLOCK_HSB(
        struct pdfs_superblock *pdfs_sb) {
    return pdfs_sb->blocksize / sizeof(struct pdfs_inode);
}

static inline uint64_t PDFS_DATA_BLOCK_TABLE_START_BLOCK_NO_HSB(
        struct pdfs_superblock *pdfs_sb) {
    return PDFS_INODE_TABLE_START_BLOCK_NO
           + pdfs_sb->inode_table_size / PDFS_INODES_PER_BLOCK_HSB(pdfs_sb)
           + 1;
}

#endif /*__PDFS_H__*/
