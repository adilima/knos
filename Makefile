AR = $(HOME)/opt/cross/bin/x86_64-elf-ar
LD = $(HOME)/opt/cross/bin/x86_64-elf-ld
CC = $(HOME)/opt/cross/bin/x86_64-elf-gcc
AS = $(HOME)/opt/cross/bin/x86_64-elf-as
CXX = $(HOME)/opt/cross/bin/x86_64-elf-g++


LIBS  = -L$(HOME)/opt/cross/lib/gcc/x86_64-elf/9.2.0 -lgcc

CFLAGS = -ffreestanding -fno-builtin -O2 -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -mcmodel=large -c
CXXFLAGS = -O2 -ffreestanding -fno-builtin -fno-rtti -fno-exceptions \
		   -mno-red-zone -mno-mmx -mno-sse -mno-sse2 \
		   -mcmodel=kernel -c

LDFLAGS = -n -T linker.ld 

OBJECTS = boot.o debug.o memory.o main.o \
		  string.o fbdev.o terminal.o rtc.o \
		  keyboard.o

#################################################
# The name should be 'knos', instead of 'test1'
# I'm beginning to like it.
#################################################
TARGET  = knos

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(LD) $(LDFLAGS) -o $(TARGET) $(OBJECTS) $(LIBS)

%.o: %.c
	$(CC) -o $@ $(CFLAGS) $<


%.o: %.asm
	nasm -felf64 -o $@ $<


install: $(TARGET)
	e2cp -v init64 part1:/elf



clean:
	rm -fv *.o $(TARGET)


run: $(TARGET)
	mcopy -i part1 -o $(TARGET) ::/elf
	cat mbr part1 > disk0
	qemu-system-x86_64 --bios OVMF.fd -m 1024 -nographic -smp 4 -drive file=disk0,format=raw -no-reboot
	rm -fv disk0


qemu: $(TARGET)
	mcopy -i part1 -o $(TARGET) ::/elf
	cat mbr part1 > disk0
	qemu-system-x86_64 --bios OVMF.fd -m 1024 -serial stdio -drive file=disk0,format=raw -smp 4 -no-reboot
	rm -fv disk0


