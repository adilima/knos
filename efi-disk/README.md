## About

The tarballs in this folder contains 2 files:
1. mbr (actually GPT).
2. part1 (EFI partition).

The tarball does not have folder, so you better uncompress it in a new folder:

```bash

mkdir knos-efi
cd knos-efi
tar -xf ../knos-0.0.3.tar.bz2

cat mbr part1 > disk0
qemu-system-x86_64 --bios OVMF.fd -drive file=disk0,format=raw -m 1024 -nographic -no-reboot


```

Note that using -nographic will automatically use -serial stdio.

The source codes already included inside the disk image in folder /sources/gnu/step1.
You can list the content of the directory as follow (needs mtools):

```bash

mdir -i part1 ::/sources/gnu/step1

```

 Volume in drive : is EFI        
 Volume Serial Number is DFD7-7733
Directory for ::/sources/gnu/step1

.            <DIR>     2019-10-24  16:25 
..           <DIR>     2019-10-24  16:25 
KNOS-0~1 BZ2     11879 2019-10-24  17:20  knos-0.0.1.tar.bz2
KNOS-0~2 BZ2     12703 2019-10-24  18:46  knos-0.0.2.tar.bz2
KNOS-0~3 BZ2     16330 2019-10-26   0:54  knos-0.0.3.tar.bz2
        5 files              40 912 bytes
                         34 127 872 bytes free

In the future, the oldest probably will not be included anymore.
