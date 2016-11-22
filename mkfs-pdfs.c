#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "pdfs.h"

int main(int argc, char *argv[]) {
    int fd;
    ssize_t ret;
    uint64_t welcome_inode_no;
    uint64_t welcome_data_block_no_offset;

    fd = open(argv[1], O_RDWR);
    if (fd == -1) {
        perror("Error opening the device");
        return -1;
    }

    // construct superblock
    struct pdfs_superblock pdfs_sb = {
        .version = 1,
        .magic = PDFS_MAGIC,
        .blocksize = PDFS_DEFAULT_BLOCKSIZE,
        .inode_table_size = PDFS_DEFAULT_INODE_TABLE_SIZE,
        .inode_count = 2,
        .data_block_table_size = PDFS_DEFAULT_DATA_BLOCK_TABLE_SIZE,
        .data_block_count = 2,
    };

    // construct inode bitmap
    char inode_bitmap[pdfs_sb.blocksize];
    memset(inode_bitmap, 0, sizeof(inode_bitmap));
    inode_bitmap[0] = 1;

    // construct data block bitmap
    char data_block_bitmap[pdfs_sb.blocksize];
    memset(data_block_bitmap, 0, sizeof(data_block_bitmap));
    data_block_bitmap[0] = 1;

    // construct root inode
    struct pdfs_inode root_pdfs_inode = {
        .mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH,
        .inode_no = PDFS_ROOTDIR_INODE_NO,
        .data_block_no 
            = PDFS_DATA_BLOCK_TABLE_START_BLOCK_NO_HSB(&pdfs_sb)
                + PDFS_ROOTDIR_DATA_BLOCK_NO_OFFSET,
        .dir_children_count = 1,
    };

    // construct welcome file inode
    char welcome_body[] = "Welcome Hellofs!!\n";
    welcome_inode_no = PDFS_ROOTDIR_INODE_NO + 1;
    welcome_data_block_no_offset = PDFS_ROOTDIR_DATA_BLOCK_NO_OFFSET + 1;
    struct pdfs_inode welcome_pdfs_inode = {
        .mode = S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH,
        .inode_no = welcome_inode_no,
        .data_block_no 
            = PDFS_DATA_BLOCK_TABLE_START_BLOCK_NO_HSB(&pdfs_sb)
                + welcome_data_block_no_offset,
        .file_size = sizeof(welcome_body),
    };

    // construct root inode data block
    struct pdfs_dir_record root_dir_records[] = {
        {
            .filename = "wel_helo.txt",
            .inode_no = welcome_inode_no,
        },
    };

    ret = 0;
    do {
        // write super block
        if (sizeof(pdfs_sb)
                != write(fd, &pdfs_sb, sizeof(pdfs_sb))) {
            ret = -1;
            break;
        }
        if ((off_t)-1
                == lseek(fd, pdfs_sb.blocksize, SEEK_SET)) {
            ret = -2;
            break;
        }

        // write inode bitmap
        if (sizeof(inode_bitmap)
                != write(fd, inode_bitmap, sizeof(inode_bitmap))) {
            ret = -3;
            break;
        }

        // write data block bitmap
        if (sizeof(data_block_bitmap)
                != write(fd, data_block_bitmap,
                         sizeof(data_block_bitmap))) {
            ret = -4;
            break;
        }

        // write root inode
        if (sizeof(root_pdfs_inode)
                != write(fd, &root_pdfs_inode,
                         sizeof(root_pdfs_inode))) {
            ret = -5;
            break;
        }

        // write welcome file inode
        if (sizeof(welcome_pdfs_inode)
                != write(fd, &welcome_pdfs_inode,
                         sizeof(welcome_pdfs_inode))) {
            ret = -6;
            break;
        }

        // write root inode data block
        if ((off_t)-1
                == lseek(
                    fd,
                    PDFS_DATA_BLOCK_TABLE_START_BLOCK_NO_HSB(&pdfs_sb)
                        * pdfs_sb.blocksize,
                    SEEK_SET)) {
            ret = -7;
            break;
        }
        if (sizeof(root_dir_records)
                != write(fd, root_dir_records,
                         sizeof(root_dir_records))) {
            ret = -8;
            break;
        }

        // write welcome file inode data block
        if ((off_t)-1
                == lseek(
                    fd,
                    (PDFS_DATA_BLOCK_TABLE_START_BLOCK_NO_HSB(&pdfs_sb)
                        + 1) * pdfs_sb.blocksize,
                    SEEK_SET)) {
            ret = -9;
            break;
        }
        if (sizeof(welcome_body) != write(fd, welcome_body,
                                          sizeof(welcome_body))) {
            ret = -10;
            break;
        }
    } while (0);

    close(fd);
    return ret;
}
