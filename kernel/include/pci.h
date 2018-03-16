
/*
 * lux OS kernel
 * copyright (c) 2018 by Omar Mohammad
 */

#pragma once

#include <types.h>

#define PCI_INDEX			0xCF8
#define PCI_DATA			0xCFC

typedef struct pci_device_t
{
	uint8_t bus, slot, function;
} pci_device_t;

void pci_write(pci_device_t *, uint16_t, uint32_t);
uint32_t pci_read(pci_device_t *, uint16_t);





