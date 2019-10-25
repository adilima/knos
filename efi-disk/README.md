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

