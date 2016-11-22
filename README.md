
pdfs - An plausible deniability kernel filesystem implementation
===
(forked from accelazh/hellofs)

A very simple linux kernel filesystem for storing files in a plausibly deniable manner. It demonstrates how to implement a VFS filesystem, from superblock, inode, dir to file operations. The license is GPL because it was forked from a GPL example (hellofs.)

Actually, it is still hellofs... intention is to morph into pdfs.

The on-disk layout of pdfs is 

  * superblock (1 block)
  * inode bitmap (1 block)
  * data block bitmap (1 block)
  * inode table (variable length)
  * data block table (variable length)

One disk block contains multiple inodes. One data block corresponds to one disk block (and of the same size). Each inode contains only one data block for simplicity.

To run test cases

```
cd pdfs
sudo ./pdfs-test.sh | grep "Test finished successfully"
```

