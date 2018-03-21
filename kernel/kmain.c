
/*
 * lux OS kernel
 * copyright (c) 2018 by Omar Mohammad
 */

#include <boot.h>
#include <kprintf.h>
#include <devmgr.h>
#include <timer.h>
#include <mm.h>
#include <gdt.h>
#include <tty.h>
#include <acpi.h>
#include <apic.h>
#include <vfs.h>
#include <tasking.h>
#include <blkdev.h>
#include <string.h>
#include <rand.h>
#include <battery.h>

void *kend;

uint8_t kernel_debug = 0;

void kmain(uint32_t multiboot_magic, multiboot_info_t *multiboot_info, vbe_mode_t *vbe_mode)
{
	// initialize kernel debugging
	char *command_line = (char*)((size_t)multiboot_info->cmdline);
	size_t i = 0;
	while(i < strlen(command_line))
	{
		if(memcmp(command_line + i, "debug=tty", 9) == 0)
			kernel_debug = 1;		// tty
		else if(memcmp(command_line + i, "debug=com1", 10) == 0)
			kernel_debug = 2;		// com1
		else
		{
			i++;
			continue;
		}

		break;
	}

	kprint_init();

	if(multiboot_magic != MULTIBOOT_MAGIC)
		kprintf("warning: invalid multiboot magic; taking a risk and continuing...\n");

	// set *kend to the start of free memory, after GRUB modules
	uint32_t mod_count = 0;
	multiboot_module_t *module;
	if(((multiboot_info->flags & MULTIBOOT_FLAGS_MODULES) != 0) && multiboot_info->mods_count != 0)
	{
		module = (multiboot_module_t*)((size_t)multiboot_info->mods_addr);

		while(mod_count < multiboot_info->mods_count)
		{
			kend = (void*)((size_t)module[mod_count].mod_end + 0x100000);
			mod_count++;
		}
	} else
	{
#if __i386__
	kend = (void*)0x400000;
#endif

#if __x86_64__
	kend = (void*)0x400000 + PHYSICAL_MEMORY;
#endif
	}

	mm_init(multiboot_info);
	devmgr_init();
	screen_init(vbe_mode);
	gdt_init();
	install_exceptions();
	acpi_init();
	apic_init();
	timer_init();
	acpi_enable();
	tasking_init();
	vfs_init();
	blkdev_init(multiboot_info);
	mount("/dev/initrd", "/", "ext2", 0, 0);
	battery_init();

	kprintf("Boot finished, %d MB used, %d MB free\n", used_pages/256, (total_pages-used_pages) / 256);

	while(1)
		asm volatile ("sti\nhlt");
}



