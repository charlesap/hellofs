obj-m := pdfs.o
pdfs-objs := kpdfs.o super.o inode.o dir.o file.o
CFLAGS_kpdfs.o := -DDEBUG
CFLAGS_super.o := -DDEBUG
CFLAGS_inode.o := -DDEBUG
CFLAGS_dir.o := -DDEBUG
CFLAGS_file.o := -DDEBUG

all: ko mkfs-pdfs

ko:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

mkfs-hellofs_SOURCES:
	mkfs-pdfs.c pdfs.h

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm mkfs-pdfs
