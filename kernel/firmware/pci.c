
/*
 * lux OS kernel
 * copyright (c) 2018 by Omar Mohammad
 */

#include <pci.h>
#include <io.h>

// pci_write(): Writes to PCI config space
// Param:	pci_device_t *device - PCI device
// Param:	uint16_t offset - byte offset
// Param:	uint32_t data - data to write
// Return:	Nothing

void pci_write(pci_device_t *device, uint16_t offset, uint32_t data)
{
	uint32_t index = (uint32_t)((uint32_t)device->bus << 16) | ((uint32_t)device->slot << 11) | ((uint32_t)device->function << 8) | ((uint32_t)offset & 0xFC);
	index |= 0x80000000;

	outd(PCI_INDEX, index);
	iowait();
	outd(PCI_DATA, data);
	iowait();
}

// pci_read(): Reads from PCI config space
// Param:	pci_device_t *device - PCI device
// Param:	uint16_t offset - byte offset
// Return:	uint32_t - data read

uint32_t pci_read(pci_device_t *device, uint16_t offset)
{
	uint32_t index = (uint32_t)((uint32_t)device->bus << 16) | ((uint32_t)device->slot << 11) | ((uint32_t)device->function << 8) | ((uint32_t)offset & 0xFC);
	index |= 0x80000000;

	outd(PCI_INDEX, index);
	iowait();
	return ind(PCI_DATA);
}




