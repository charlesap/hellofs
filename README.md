
pdfs - A plausible deniability kernel filesystem implementation
===
(forked from accelazh/hellofs)

A very simple linux kernel filesystem for storing files in a plausibly deniable manner. It demonstrates how to implement a VFS filesystem, from superblock, inode, dir to file operations. The license is GPL because it was forked from a GPL example (hellofs.)

Actually, it is still hellofs... intention is to morph into pdfs.

The on-disk layout of pdfs is currently

  * superblock (1 block)
  * inode bitmap (1 block)
  * data block bitmap (1 block)
  * inode table (variable length)
  * data block table (variable length)

One disk block contains multiple inodes. One data block corresponds to one disk block (and of the same size). Each inode contains only one data block for simplicity.

That is hellofs. pdfs will build on that.

The on-disk layout of pdfs should instead be a sea of encrpyed blocks. Those blocks may be:
 
  * encrypted empty space with a forgotten key (all un-used space is encrypted)
  * a superblock (1 per coexisting file system in the sea of blocks, each encrypted with an independent key)
  * inode bitmap (1 per coexisting file system, each encrypted with an independent key)
  * data block bitmap (1 per coexisting file system, each encrypted with an independent key)
  * inode table (variable length, 1 per, each encrypted with an independent key)
  * data block table (variable length, 1 per, each encrypted with an independent key)
  
Mounting a pdfs volume in a block device takes a key. The blocks are scanned until the key decrypts a superblock. Other keys may decrypt other superblocks, if present. A superblock may reference another superblock, incorporating that other pdfs volume as a lower-level volume. Superblocks may form a directed acyclic graph.


To run test cases

```
cd pdfs
sudo ./pdfs-test.sh | grep "Test finished successfully"
```

