
/*
 * lux OS kernel
 * copyright (c) 2018 by Omar Mohammad
 */

#include <boot.h>
#include <kprintf.h>
#include <time.h>
#include <mm.h>
#include <string.h>
#include <tty.h>

void kmain(uint32_t multiboot_magic, multiboot_info_t *multiboot_info, vbe_mode_t *vbe_mode)
{
	global_uptime = 0;
	kprint_init();

	if(multiboot_magic != MULTIBOOT_MAGIC)
		kprintf("warning: invalid multiboot magic; taking a risk and continuing...\n");

	mm_init(multiboot_info);
	screen_init(vbe_mode);

	tty_writestr("\nWelcome to lux.\n", 0);
	tty_writestr("For now, there is nothing to see, but here's a working tty.", 0);
	while(1);
}



