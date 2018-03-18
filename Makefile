
	CC=clang
	CFLAGS=-Wall -fno-builtin -ffreestanding -fomit-frame-pointer -nostdlib -nodefaultlibs -O2 -msse2
	CFILES=kernel/*.c kernel/*/*.c
	LAIFILES=lai/src/*.c lai/src/lux/*.c
	OBJECTS=*.o
	DATE=`date +"%d%m%Y-%H%M%S"`

lux32:
	rm -f *.iso
	rm -f *.o
	$(CC) $(CFLAGS) -target i386-pc-none -Ikernel/include -c runtime/*.c
	ar r runtime.a *.o
	rm -f *.o
	$(CC) $(CFLAGS) -target i386-pc-none -Ikernel/include -Ilai/src -Ilai/src/lux -c $(LAIFILES)

	fasm kernel/asm_i386/vbe.asm vbe.sys
	fasm kernel/asm_i386/bootstrap.asm bootstrap.o
	fasm kernel/asm_i386/io.asm io.o
	fasm kernel/asm_i386/state.asm state.o
	fasm kernel/asm_i386/cpu.asm cpu.o
	fasm kernel/asm_i386/sse2.asm sse2.o
	fasm kernel/asm_i386/irq_stub.asm irq_stub.o
	$(CC) $(CFLAGS) -target i386-pc-none -Ikernel/include -Ilai/src -Ilai/src/lux -c $(CFILES)
	ld -melf_i386 -nostdlib -nodefaultlibs -O2 -T kernel/ld_i386.ld $(OBJECTS) *.a -o iso/boot/kernel.sys

	#cd initrd; tar --owner=root --group=root -cf ../iso/boot/initrd.img *; cd ..
	if [ ! -d "mnt" ]; then mkdir mnt; fi
	dd if=/dev/zero conv=notrunc bs=512 count=10240 of=iso/boot/initrd.img
	sudo losetup /dev/loop0 iso/boot/initrd.img
	sudo mke2fs /dev/loop0
	sudo mount /dev/loop0 mnt
	sudo cp -R initrd/* mnt
	sudo umount -l /dev/loop0
	sudo losetup -d /dev/loop0
	rm -Rf mnt

	grub-mkrescue -o lux.iso iso
	qemu-system-i386 -cdrom lux.iso -enable-kvm -m 128 -vga std

lux64:
	rm -f *.iso
	rm -f *.o
	iasl kernel/acpi.asl 
	fasm kernel/asm_i386/vbe.asm vbe.sys
	fasm kernel/asm_x86_64/bootstrap.asm bootstrap.o
	fasm kernel/asm_x86_64/io.asm io.o
	fasm kernel/asm_x86_64/state.asm state.o
	fasm kernel/asm_x86_64/cpu.asm cpu.o
	fasm kernel/asm_x86_64/sse2.asm sse2.o
	fasm kernel/asm_x86_64/irq_stub.asm irq_stub.o
	$(CC) $(CFLAGS) -target x86_64-pc-none -m64 -mno-red-zone -mcmodel=large -Ikernel/include -Ilai/src -Ilai/src/lux -c $(LAIFILES)
	$(CC) $(CFLAGS) -target x86_64-pc-none -m64 -mno-red-zone -mcmodel=large -Ikernel/include -Ilai/src -Ilai/src/lux -c $(CFILES)
	ld -melf_x86_64 -nostdlib -nodefaultlibs -O2 -T kernel/ld_x86_64.ld $(OBJECTS) -o kernel64.sys

	#cd initrd; tar --owner=root --group=root -cf ../iso/boot/initrd.img *; cd ..
	if [ ! -d "mnt" ]; then mkdir mnt; fi
	dd if=/dev/zero conv=notrunc bs=512 count=10240 of=iso/boot/initrd.img
	sudo losetup /dev/loop0 iso/boot/initrd.img
	sudo mke2fs /dev/loop0
	sudo mount /dev/loop0 mnt
	sudo cp -R initrd/* mnt
	sudo umount -l /dev/loop0
	sudo losetup -d /dev/loop0
	rm -Rf mnt

	fasm kernel/asm_x86_64/setup.asm iso/boot/kernel.sys
	grub-mkrescue -o lux.iso iso
	qemu-system-x86_64 -cdrom lux.iso -enable-kvm -m 128 -vga std

clean:
	rm -f iso/boot/*.*
	rm -f *.o *.a *.sys



