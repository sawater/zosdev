SOURCE=boot.o main.o commona.o commonb.o monitor.o descriptor.o gdt.o interrupt.o isr.o timer.o paging.o buddy.o syscall.o task.o keybord.o initrd.o
ASFLAGS=-f elf
CC=i586-elf-gcc
CFLAGS=-ffreestanding
#CFLAGS=-ffreestanding -O2 -Wall -Wextra
LDFLAGS=-T link.ld -nostdlib -s
all: $(SOURCE) link
clean:
	rm *.o kernel

link:
	i586-elf-ld $(LDFLAGS) -o kernel $(SOURCE)

.s.o:
	nasm $(ASFLAGS) $<
