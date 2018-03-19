
/*
 * lux OS kernel
 * copyright (c) 2018 by Omar Mohammad
 */

#pragma once

#include <types.h>
#include <acpi.h>

/* HPET MMIO Registers */
#define HPET_CAP			0x0000
#define HPET_CONFIG			0x0010
#define HPET_INTR			0x0020
#define HPET_COUNTER			0x00F0

#define HPET_TN_CONFIG_CAP		0x0100
#define HPET_TN_COUNTER			0x0108
#define HPET_TN_INTR			0x0110

/* HPET General Capabilities */
#define HPET_CAP_64BIT			0x2000
#define HPET_CAP_LEGACY_IRQ		0x8000

/* HPET General Configuration */
#define HPET_CONFIG_ENABLE		0x0001
#define HPET_CONFIG_LEGACY_IRQ		0x0002

/* HPET Timer Config/Capability Register */
#define HPET_TN_CAP_LEVEL		0x0002
#define HPET_TN_CONFIG_ENABLE		0x0004
#define HPET_TN_CONFIG_PERIODIC		0x0008
#define HPET_TN_CAP_PERIODIC		0x0010
#define HPET_TN_CAP_64BIT		0x0020
#define HPET_TN_CONFIG_DIRECT		0x0040
#define HPET_TN_CONFIG_FORCE32		0x0100
#define HPET_TN_CONFIG_FSB		0x4000

typedef struct acpi_hpet_t
{
	acpi_header_t header;
	uint16_t cap;
	uint16_t vendor_id;
	acpi_gas_t base;
	uint8_t hpet_sequence;
	uint16_t minimum_periodic_count;
	uint8_t page_protection;
}__attribute__((packed)) acpi_hpet_t;

int hpet_init();
uint64_t hpet_read(size_t);
void hpet_write(size_t, uint64_t);
uint64_t hpet_read_timer(size_t, size_t);
void hpet_write_timer(size_t, size_t, uint64_t);
uint8_t hpet_init_timer(size_t, size_t);



