
/*
 * lux OS kernel
 * copyright (c) 2018 by Omar Mohammad
 */

#include <acpi.h>
#include <apic.h>
#include <mm.h>
#include <kprintf.h>
#include <io.h>
#include <irq.h>
#include <string.h>
#include <timer.h>

uint8_t acpi_sci;
extern void acpi_irq_stub();

// acpi_enable(): Enables ACPI SCI
// Param:	Nothing
// Return:	Nothing

void acpi_enable()
{
	kprintf("acpi: enabling ACPI SCI...\n");

	// initialize the IRQ
	acpi_sci = irq_configure(acpi_fadt->sci_irq, IRQ_LEVEL | IRQ_ACTIVE_LOW);
	irq_install(acpi_sci, (size_t)&acpi_irq_stub);

	// call the root _INI function
	acpi_state_t state;
	acpi_object_t object;

	memset(&state, 0, sizeof(acpi_state_t));
	strcpy(state.name, "\\._INI");
	acpi_exec_method(&state, &object);

	memset(&state, 0, sizeof(acpi_state_t));
	strcpy(state.name, "\\._SB_._INI");
	acpi_exec_method(&state, &object);

	// now initialize every device on the namespace
	acpi_handle_t *device;
	size_t device_index = 0;

	device = acpins_get_device(device_index);

	char path[512];
	int eval_status;

	while(device != NULL)
	{
		// get _STA status for all devices
		strcpy(path, device->path);
		strcpy(path + strlen(path), "._STA");

		eval_status = acpi_eval(&object, path);
		if(eval_status == 0 && object.integer != 0)
		{
			kprintf("acpi: eval %s: 0x%xb\n", path, object.integer);

			if(object.integer & ACPI_STA_PRESENT && object.integer & ACPI_STA_FUNCTION)
			{
				// invoke _INI for devices that are working properly
				strcpy(state.name, device->path);
				strcpy(state.name + strlen(state.name), "._INI");

				acpi_exec_method(&state, &object);
			}
		}

		device_index++;
		device = acpins_get_device(device_index);
	}

	// tell the firmware which interrupt mode we're using
	acpi_handle_t *pic = acpins_resolve("_PIC");
	if(!pic)
		kprintf("acpi: _PIC() method not present, ignoring...\n");
	else
	{
		memset(&state, 0, sizeof(acpi_state_t));
		strcpy(state.name, pic->path);
		state.arg[0].type = ACPI_INTEGER;

		if(irq_mode == IRQ_IOAPIC)
			state.arg[0].integer = 1;	// APIC mode
		else if(irq_mode == IRQ_PIC)
			state.arg[0].integer = 0;	// PIC mode

		kprintf("acpi: execute _PIC(%d)\n", state.arg[0].integer);
		acpi_exec_method(&state, &object);
	}

	// finally, enable ACPI
	if(acpi_fadt->smi_command_port == 0 || acpi_fadt->acpi_enable == 0)
		kprintf("acpi: system doesn't support SMM.\n");

	else
	{
		outb(acpi_fadt->smi_command_port, acpi_fadt->acpi_enable);
		iowait();
		iowait();
		iowait();
		iowait();

		while((inw(acpi_fadt->pm1a_control_block) & 1) == 0)
		{
			iowait();
			iowait();
			iowait();
			iowait();
		}

		kprintf("acpi: ACPI is now enabled.\n");
	}

	outw(acpi_fadt->pm1a_event_block + 2, ACPI_POWER_BUTTON | ACPI_SLEEP_BUTTON);
	iowait();
	iowait();
	iowait();
	iowait();

	irq_unmask(acpi_sci);

	while(1)
	{
		asm volatile ("sti\nhlt");
	}
}

// acpi_irq(): ACPI SCI IRQ handler
// Param:	Nothing
// Return:	Nothing

void acpi_irq()
{
	uint16_t event = inw(acpi_fadt->pm1a_event_block);
	outw(acpi_fadt->pm1a_event_block, event);
	kprintf("acpi: SCI IRQ occured: event data 0x%xw\n");

	irq_eoi(acpi_sci);
}




