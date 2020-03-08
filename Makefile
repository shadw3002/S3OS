#########################
# Makefile for Orange'S #
#########################

# Entry point of Orange'S
# It must have the same value with 'KernelEntryPointPhyAddr' in load.inc!
ENTRYPOINT	= 0x30400

# Offset of entry point in kernel file
# It depends on ENTRYPOINT
ENTRYOFFSET	=   0x400

# Programs, flags, etc.
ASM		= nasm
DASM	= ndisasm
CC		= clang++
LD		= ld
ASMBFLAGS	= -I boot/include/
ASMKFLAGS	= -I include/ -f elf32
CFLAGS		= -m32 -w -I include/ -c -fno-builtin -fno-builtin -fno-stack-protector
LDFLAGS		= -m elf_i386 -s -Ttext $(ENTRYPOINT)
DASMFLAGS	= -u -o $(ENTRYPOINT) -e $(ENTRYOFFSET)

# This Program
ORANGESBOOT	= boot/boot.bin boot/loader.bin
ORANGESKERNEL	= kernel.bin
OBJS		= kernel/kernel.o lib/syscall.o kernel/start.o kernel/main.o\
			lib/clock.o lib/keyboard.o lib/tty.o lib/console.o\
			  lib/protect.o lib/proc.o lib/assert.o lib/hd_driver.o\
			lib/stdio.o  lib/interruption.o lib/systask.o\
			lib/stringA.o lib/portA.o lib/string.o lib/memory.o\
			lib/ttyA.o lib/interruptionA.o lib/filesystem.o lib/task.o \
			lib/thread.o
DASMOUTPUT	= kernel.bin.asm

# All Phony Targets
.PHONY : everything final image clean realclean disasm all buildimg

# Default starting position
nop :
	@echo "why not \`make image' huh? :)"

everything : $(ORANGESBOOT) $(ORANGESKERNEL)

all : realclean everything

image : realclean everything clean buildimg

clean :
	rm -f $(OBJS)

realclean :
	rm -f $(OBJS) $(ORANGESBOOT) $(ORANGESKERNEL)

disasm :
	$(DASM) $(DASMFLAGS) $(ORANGESKERNEL) > $(DASMOUTPUT)

# We assume that "img/a.img" exists in current folder
buildimg :
	dd if=boot/boot.bin of=img/a.img bs=512 count=1 conv=notrunc
	sudo mount -o loop img/a.img /mnt/floppy/
	sudo cp -fv boot/loader.bin /mnt/floppy/
	sudo cp -fv kernel.bin /mnt/floppy
	sudo umount /mnt/floppy

boot/boot.bin : boot/boot.asm boot/include/load.inc boot/include/fat12hdr.inc
	$(ASM) $(ASMBFLAGS) -o $@ $<

boot/loader.bin : boot/loader.asm boot/include/load.inc boot/include/fat12hdr.inc boot/include/pm.inc
	$(ASM) $(ASMBFLAGS) -o $@ $<

$(ORANGESKERNEL) : $(OBJS)
	$(LD) $(LDFLAGS) -o $(ORANGESKERNEL) $(OBJS)

kernel/kernel.o : kernel/kernel.asm include/sconst.inc
	$(ASM) $(ASMKFLAGS) -o $@ $<

lib/syscall.o : lib/syscall.asm include/sconst.inc
	$(ASM) $(ASMKFLAGS) -o $@ $<

kernel/start.o: kernel/start.cpp

	$(CC) $(CFLAGS) -o $@ $<

kernel/main.o: kernel/main.cpp

	$(CC) $(CFLAGS) -o $@ $<

lib/clock.o: lib/clock.cpp
	$(CC) $(CFLAGS) -o $@ $<

lib/keyboard.o: lib/keyboard.cpp
	$(CC) $(CFLAGS) -o $@ $<

lib/tty.o: lib/tty.cpp
	$(CC) $(CFLAGS) -o $@ $<

lib/console.o: lib/console.cpp
	$(CC) $(CFLAGS) -o $@ $<

lib/protect.o: lib/protect.cpp

	$(CC) $(CFLAGS) -o $@ $<

lib/proc.o: lib/proc.cpp
	$(CC) $(CFLAGS) -o $@ $<

lib/stdio.o: lib/stdio.cpp
	$(CC) $(CFLAGS) -o $@ $<

lib/stringA.o : lib/stringA.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<

lib/interruption.o: lib/interruption.cpp
	$(CC) $(CFLAGS) -o $@ $<

lib/portA.o : lib/portA.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<

lib/ttyA.o : lib/ttyA.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<

lib/interruptionA.o : lib/interruptionA.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<

lib/string.o : lib/string.cpp
	$(CC) $(CFLAGS) -o $@ $<

lib/assert.o : lib/assert.cpp
	$(CC) $(CFLAGS) -o $@ $<

lib/systask.o : lib/systask.cpp
	$(CC) $(CFLAGS) -o $@ $<

lib/hd_driver.o : lib/hd_driver.cpp
	$(CC) $(CFLAGS) -o $@ $<

lib/filesystem.o : lib/filesystem.cpp
	$(CC) $(CFLAGS) -o $@ $<

lib/memory.o : lib/memory.cpp
	$(CC) $(CFLAGS) -o $@ $<

lib/task.o : lib/task.cpp
	$(CC) $(CFLAGS) -o $@ $<

lib/thread.o : lib/thread.cpp
	$(CC) $(CFLAGS) -o $@ $<

###############################################
PGM_ENTRYPOINT	= 0x82000

UPG_ASMKFLAGS	= -f elf32
UPG_CFLAGS		= -m32 -I user_pgm/ -Og -fno-pic -c -fno-builtin -fno-stack-protector
UPG_LDFLAGS		= -m elf_i386 -Ttext $(PGM_ENTRYPOINT) --oformat binary

# This Program
PGM	 = user_pgm.bin
UPG_OBJS = user_pgm/main.o user_pgm/cstart.o

pgm: $(PGM)
	dd if=user_pgm.bin seek=16 of=img/80m.img bs=512 count=16 conv=notrunc

$(PGM) : $(UPG_OBJS)
	$(LD) $(UPG_LDFLAGS) -o $(PGM) $(UPG_OBJS)

user_pgm/main.o: user_pgm/main.cpp
	$(CC) $(UPG_CFLAGS) -o $@ $<

user_pgm/cstart.o : user_pgm/cstart.asm
	$(ASM) $(ASMKFLAGS) -o $@ $<