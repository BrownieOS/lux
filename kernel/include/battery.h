
/*
 * lux OS kernel
 * copyright (c) 2018 by Omar Mohammad
 */

#pragma once

#include <types.h>

#define MAX_BATTERIES			32

// _BIF Package
#define BIF_POWER_UNIT			0
#define BIF_DESIGN_CAPACITY		1
#define BIF_FULL_CAPACITY		2
#define BIF_CHARGEABLE			3
#define BIF_VOLTAGE			4
#define BIF_WARNING			5
#define BIF_LOW				6
#define BIF_MODEL			9
#define BIF_SERIAL			10
#define BIF_TYPE			11
#define BIF_OEM_SPECIFIC		12

// Power Units
#define POWER_UNIT_MW			0
#define POWER_UNIT_MA			1

// _BST Package
#define BST_STATE			0
#define BST_PRESENT_RATE		1
#define BST_REMAINING			2
#define BST_VOLTAGE			3

// Battery State
#define BATTERY_DISCHARGING		0x01
#define BATTERY_CHARGING		0x02
#define BATTERY_CRITICAL		0x04

typedef struct acpi_battery_t
{
	char path[512];
	uint64_t power_unit;
	uint64_t capacity;
	uint64_t chargeable;
	uint64_t voltage;
	uint64_t warning;
	uint64_t low;

	char model[512];
	char serial[512];
	char type[512];
	char oem[512];

	uint64_t state;
	uint64_t remaining;
} acpi_battery_t;

extern acpi_battery_t *batteries[];
size_t battery_count;

void battery_init();
int battery_create(size_t);
int battery_update(size_t);





