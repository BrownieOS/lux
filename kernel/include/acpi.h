
/*
 * lux OS kernel
 * copyright (c) 2018 by Omar Mohammad
 */

#pragma once

#include <types.h>
#include <lock.h>
#include <lai.h>

void acpi_enable();
void acpi_init();
int acpi_checksum(void *);
void acpi_load_fadt();





