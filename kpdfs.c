#include "kpdfs.h"

DEFINE_MUTEX(pdfs_sb_lock);

struct file_system_type pdfs_fs_type = {
    .owner = THIS_MODULE,
    .name = "pdfs",
    .mount = pdfs_mount,
    .kill_sb = pdfs_kill_superblock,
    .fs_flags = FS_REQUIRES_DEV,
};

const struct super_operations pdfs_sb_ops = {
    .destroy_inode = pdfs_destroy_inode,
    .put_super = pdfs_put_super,
};

const struct inode_operations pdfs_inode_ops = {
    .create = pdfs_create,
    .mkdir = pdfs_mkdir,
    .lookup = pdfs_lookup,
};

const struct file_operations pdfs_dir_operations = {
    .owner = THIS_MODULE,
    .readdir = pdfs_readdir,
};

const struct file_operations pdfs_file_operations = {
    .read = pdfs_read,
    .write = pdfs_write,
};

struct kmem_cache *pdfs_inode_cache = NULL;

static int __init pdfs_init(void)
{
    int ret;

    pdfs_inode_cache = kmem_cache_create("pdfs_inode_cache",
                                         sizeof(struct pdfs_inode),
                                         0,
                                         (SLAB_RECLAIM_ACCOUNT| SLAB_MEM_SPREAD),
                                         NULL);
    if (!pdfs_inode_cache) {
        return -ENOMEM;
    }

    ret = register_filesystem(&pdfs_fs_type);
    if (likely(0 == ret)) {
        printk(KERN_INFO "Sucessfully registered pdfs\n");
    } else {
        printk(KERN_ERR "Failed to register pdfs. Error code: %d\n", ret);
    }

    return ret;
}

static void __exit pdfs_exit(void)
{
    int ret;

    ret = unregister_filesystem(&pdfs_fs_type);
    kmem_cache_destroy(pdfs_inode_cache);

    if (likely(ret == 0)) {
        printk(KERN_INFO "Sucessfully unregistered pdfs\n");
    } else {
        printk(KERN_ERR "Failed to unregister pdfs. Error code: %d\n",
               ret);
    }
}

module_init(pdfs_init);
module_exit(pdfs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("accelazh,charlesap");
