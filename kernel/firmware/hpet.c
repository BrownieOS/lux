
/*
 * lux OS kernel
 * copyright (c) 2018 by Omar Mohammad
 */

#include <hpet.h>
#include <acpi.h>
#include <apic.h>
#include <timer.h>
#include <devmgr.h>
#include <mm.h>
#include <irq.h>
#include <io.h>
#include <rtc.h>

/* High Precision Event Timer Driver */

acpi_hpet_t *hpet;
size_t hpet_base, hpet_count;
size_t hpet_timer;		// this will tell us which timer we're using

// hpet_init(): Detects and initializes the HPET
// Param:	Nothing
// Return:	int - 0 on success

int hpet_init()
{
	hpet = acpi_scan("HPET", 0);		// scan for ACPI HPET table
	if(!hpet)
		return 1;

	// check if the HPET registers really are memory-mapped
	if(hpet->base.address_space != ACPI_GAS_MMIO)
	{
		kprintf("hpet: base is undefined address space %d\n", hpet->base.address_space);
		return 1;
	}

	hpet_base = vmm_request_map(hpet->base.base, 1, PAGE_PRESENT | PAGE_RW | PAGE_UNCACHEABLE);
	kprintf("hpet: base MMIO is 0x%xd\n", hpet->base.base);

#if __i386__
	kprintf("hpet: mapped MMIO at 0x%xd\n", hpet_base);
#endif
#if __x86_64__
	kprintf("hpet: mapped MMIO at 0x%xq\n", hpet_base);
#endif

	// if we're using the legacy PIC, we need to use HPET legacy mapping
	// because the standard mappings are for APIC only
	// if the HPET doesn't support legacy mapping and we're using the PIC,
	// we can't use the HPET...
	if((hpet_read(HPET_CAP) & HPET_CAP_LEGACY_IRQ) == 0 && irq_mode == IRQ_PIC)
	{
		kprintf("hpet: IRQ controller is PIC and HPET doesn't support legacy mapping.\n");
		return 1;
	}

	// the HPET period must be non-zero and less than 0x5F5E100
	uint32_t period = (hpet_read(HPET_CAP) >> 32) & 0xFFFFFFFF;
	if(period == 0 || period > 0x5F5E100)
	{
		kprintf("hpet: invalid period value 0x%xd, ignoring HPET.\n", period);
		return 1;
	}

	// turn off the HPET and reset its counter
	hpet_write(HPET_CONFIG, 0);
	hpet_write(HPET_COUNTER, 0);

	hpet_count = (hpet_read(HPET_CAP) >> 8) & 0x1F;
	hpet_count++;

	kprintf("hpet: %d timers present.\n", hpet_count);

	// now search for a timer that supports periodic mode
	// the HPET spec says every HPET must have at least three timers, of which
	// at least one must support periodic mode
	size_t current_timer = 0;
	hpet_timer = 0;
	uint64_t timer_cap;
	uint8_t valid = 0;

	while(current_timer < hpet_count)
	{
		timer_cap = hpet_read_timer(current_timer, HPET_TN_CONFIG_CAP);
		if(timer_cap & HPET_TN_CAP_PERIODIC && valid == 0)
		{
			kprintf("hpet: using periodic mode on timer %d\n", current_timer);
			hpet_timer = current_timer;
			valid = 1;
		}

		current_timer++;
	}

	if(valid != 1)
	{
		kprintf("hpet: no timers support periodic mode, aborting.\n");
		return 1;
	}

	// now we have a timer we can initialize
	timer_irq_line = hpet_init_timer(hpet_timer, (size_t)&timer_irq_stub);
	if(timer_irq_line == 0xFF)
		return 1;

	kprintf("hpet: using the HPET as a timer.\n");

	device_t *device = kmalloc(sizeof(device_t));
	device->category = DEVMGR_CATEGORY_SYSTEM;
	device->mmio[0].base = hpet->base.base;
	device->mmio[0].size = 0x400;		// HPET spec says MMIO is always 1 KB
	device->irq = timer_irq_line;
	devmgr_register(device, "High Precision Event Timer");
	kfree(device);

	return 0;
}

// hpet_read(): Reads a HPET register
// Param:	size_t index - register index
// Return:	uint64_t - value from register

uint64_t hpet_read(size_t index)
{
	volatile uint64_t *mmio = (volatile uint64_t*)((size_t)hpet_base + index);
	return mmio[0];
}

// hpet_write(): Writes a HPET register
// Param:	size_t index - register index
// Param:	uint64_t value - value to write
// Return:	Nothing

void hpet_write(size_t index, uint64_t value)
{
	volatile uint64_t *mmio = (volatile uint64_t*)((size_t)hpet_base + index);
	mmio[0] = value;
}

// hpet_read_timer(): Reads a HPET timer register
// Param:	size_t timer - timer number
// Param:	size_t index - timer register index
// Return:	uint64_t - value from register

inline uint64_t hpet_read_timer(size_t timer, size_t index)
{
	return hpet_read((timer << 5) + index);
}

// hpet_write_timer(): Writes a HPET timer register
// Param:	size_t timer - timer number
// Param:	size_t index - timer register index
// Param:	uint64_t value - value to write
// Return:	Nothing

inline void hpet_write_timer(size_t timer, size_t index, uint64_t value)
{
	hpet_write((timer << 5) + index, value);
}

// hpet_init_timer(): Initializes a HPET timer
// Param:	size_t timer - timer number
// Param:	size_t handler - interrupt handler
// Return:	uint8_t - IRQ number of this timer

uint8_t hpet_init_timer(size_t timer, size_t handler)
{
	uint8_t irq;
	uint64_t value;

	// turn off the timer and put it in periodic mode
	value = hpet_read_timer(timer, HPET_TN_CONFIG_CAP);
	value &= ~HPET_TN_CONFIG_ENABLE;
	value &= ~HPET_TN_CONFIG_FSB;
	value |= HPET_TN_CONFIG_FORCE32;
	value |= HPET_TN_CONFIG_PERIODIC;
	value |= HPET_TN_CONFIG_DIRECT;
	hpet_write_timer(timer, HPET_TN_CONFIG_CAP, value);

	if(irq_mode == IRQ_PIC)
	{
		if(timer == 0)
			irq = 0;
		else if(timer == 1)
			irq = 8;
		else
		{
			kprintf("hpet: can't initialize timer %d because PIC is IRQ controller...\n", timer);
			return 0xFF;
		}

		// enable legacy mapping
		hpet_write(HPET_CONFIG, HPET_CONFIG_LEGACY_IRQ);
	} else
	{
		// for I/O APICs, don't use legacy mapping unless it's timer 0 or 1
		if(hpet_read(HPET_CAP) & HPET_CAP_LEGACY_IRQ)
		{
			if(timer == 0)
				irq = 0;
			else if(timer == 1)
				irq = 8;

			else
				goto normal_routing;

			hpet_write(HPET_CONFIG, HPET_CONFIG_LEGACY_IRQ);
			if(hpet_read_timer(timer, HPET_TN_CONFIG_CAP) & HPET_TN_CAP_LEVEL)
				irq = irq_configure(irq, IRQ_ACTIVE_HIGH | IRQ_LEVEL | IRQ_BROADCAST);
			else
				irq = irq_configure(irq, IRQ_ACTIVE_HIGH | IRQ_EDGE | IRQ_BROADCAST);

			if(irq == 0xFF)
				return 0xFF;

			goto disable_pit;
		}

normal_routing:
		hpet_write(HPET_CONFIG, 0);

		// allocate an IRQ starting from IRQ 2
		irq = 2;
		while(((hpet_read_timer(timer, HPET_TN_CONFIG_CAP) >> 32) & 1 << irq) == 0)
		{
			irq++;
			if(irq >= 32)
			{
				kprintf("hpet: failed to allocate an IRQ for use with I/O APIC.\n");
				return 0xFF;
			}
		}

		// tell the HPET about the IRQ
		value = hpet_read_timer(timer, HPET_TN_CONFIG_CAP);
		value &= ~(0x1F << 9);
		value |= irq << 9;
		hpet_write_timer(timer, HPET_TN_CONFIG_CAP, value);

		// and configure the IRQ
		if(hpet_read_timer(timer, HPET_TN_CONFIG_CAP) & HPET_TN_CAP_LEVEL)
			irq = irq_configure(irq, IRQ_ACTIVE_HIGH | IRQ_LEVEL | IRQ_BROADCAST);
		else
			irq = irq_configure(irq, IRQ_ACTIVE_HIGH | IRQ_EDGE | IRQ_BROADCAST);

		if(irq == 0xFF)
			return 0xFF;
	}

disable_pit:
	// disable the PIT
	outb(0x43, 0x30);
	iowait();
	outb(0x40, 0xFF);
	iowait();
	//outb(0x40, 0xFF);
	iowait();

	// IRQ handler
	irq_install(irq, handler);
	irq_unmask(irq);

	// reset the main counter
	hpet_write(HPET_COUNTER, 0);
	hpet_write(HPET_INTR, hpet_read(HPET_INTR));

	// calculate the HPET frequency
	uint64_t frequency, period, divider;
	uint64_t fsec_per_sec = 0x38D7EA4C68000;
	period = (hpet_read(HPET_CAP) >> 32) & 0xFFFFFFFF;
	frequency = fsec_per_sec / period;	// 10^15 * period

	divider = frequency / TIMER_FREQUENCY;

	// write the periodic value
	hpet_write_timer(timer, HPET_TN_COUNTER, divider);

	// enable the timer
	value = hpet_read_timer(timer, HPET_TN_CONFIG_CAP);
	value |= HPET_TN_CONFIG_ENABLE | HPET_TN_CONFIG_PERIODIC;
	value |= HPET_TN_CONFIG_FORCE32;
	hpet_write_timer(timer, HPET_TN_CONFIG_CAP, value);

	// enable the global counter
	value = hpet_read(HPET_CONFIG);
	value |= HPET_CONFIG_ENABLE;
	hpet_write(HPET_CONFIG, value);

	return irq;
}




