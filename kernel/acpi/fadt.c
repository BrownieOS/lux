
/*
 * lux OS kernel
 * copyright (c) 2018 by Omar Mohammad
 */

#include <acpi.h>
#include <mm.h>
#include <kprintf.h>
#include <io.h>

acpi_fadt_t *acpi_fadt;
acpi_aml_t *acpi_dsdt;

// acpi_load_fadt(): Loads the FADT and DSDT tables
// Param:	Nothing
// Return:	Nothing

void acpi_load_fadt()
{
	acpi_fadt = acpi_scan("FACP", 0);
	if(!acpi_fadt)
	{
		kprintf("acpi: boot failed, FADT table not present.\n");
		while(1);
	}

	uint64_t dsdt_physical;

	acpi_dsdt = (acpi_aml_t*)((size_t)acpi_fadt->dsdt);

	dsdt_physical = (uint64_t)acpi_dsdt;

	acpi_header_t *dsdt_header = (acpi_header_t*)vmm_request_map((size_t)acpi_dsdt, 1, PAGE_PRESENT | PAGE_RW);
	acpi_dsdt = (acpi_aml_t*)vmm_request_map((size_t)acpi_dsdt, (size_t)(dsdt_header->length + PAGE_SIZE - 1) / PAGE_SIZE, PAGE_PRESENT | PAGE_RW);
	vmm_unmap((size_t)dsdt_header, 1);

	// do the checksum
	if(acpi_checksum(acpi_dsdt) != 0)
	{
		kprintf("acpi: boot failed, DSDT checksum error.\n");
		while(1);
	}

	kprintf("acpi: 'DSDT' 0x%xq len %d v%xb OEM '%c%c%c%c%c%c'\n", dsdt_physical, acpi_dsdt->header.length, acpi_dsdt->header.revision, acpi_dsdt->header.oem[0], acpi_dsdt->header.oem[1], acpi_dsdt->header.oem[2], acpi_dsdt->header.oem[3], acpi_dsdt->header.oem[4], acpi_dsdt->header.oem[5]);
}




