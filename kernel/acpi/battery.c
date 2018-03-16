
/*
 * lux OS kernel
 * copyright (c) 2018 by Omar Mohammad
 */

#include <acpi.h>
#include <mm.h>
#include <battery.h>
#include <string.h>

/* ACPI Control Method Battery Driver */

#define ACPI_BATTERY_ID			"PNP0C0A"

acpi_battery_t *batteries[MAX_BATTERIES];
size_t battery_count;

// battery_init(): Detects and initializes ACPI batteries
// Param:	Nothing
// Return:	Nothing

void battery_init()
{
	// generate a list of ACPI battery objects
	acpi_object_t battery_id;
	acpi_eisaid(&battery_id, ACPI_BATTERY_ID);

	acpi_handle_t *handle;
	size_t index = 0;
	handle = acpins_get_deviceid(index, &battery_id);
	while(handle != NULL)
	{
		//kprintf("acpi: found battery device %s\n", handle->path);

		// register the battery
		batteries[battery_count] = kmalloc(sizeof(acpi_battery_t));

		strcpy(batteries[battery_count]->path, handle->path);
		if(battery_create(battery_count) == 0)
			battery_count++;

		if(battery_count >= MAX_BATTERIES)
			break;

		index++;
		handle = acpins_get_deviceid(index, &battery_id);
	}

	if(battery_count != 0)
		kprintf("acpi: %d batteries present.\n", battery_count);
}

// battery_create(): Initializes a specific ACPI battery
// Param:	size_t index - battery index
// Return:	int - 0 on success

int battery_create(size_t index)
{
	// verify the device's _STA
	char path[512];
	int eval_status;
	acpi_object_t object;

	strcpy(path, batteries[index]->path);
	strcpy(path + strlen(path), "._STA");		// device status

	eval_status = acpi_eval(&object, path);

	if(eval_status == 0)
	{
		// if the device is not present, disabled, or non-functional --
		// -- ignore this battery device
		if(!object.integer & ACPI_STA_PRESENT || !object.integer & ACPI_STA_ENABLED || !object.integer & ACPI_STA_FUNCTION)
			return 1;
	}

	kprintf("acpi: init battery %s\n", batteries[index]->path);

	// ACPI gives us two functions to get static info: _BIF and _BIX
	// We'll use _BIF only now, for simplicity's sake
	strcpy(path, batteries[index]->path);
	strcpy(path + strlen(path), "._BIF");

	eval_status = acpi_eval(&object, path);
	if(eval_status != 0)
	{
		// TO-DO: Implement _BIX here
		kprintf("acpi: battery device doesn't support _BIF, ignoring device.\n");
		return 1;
	}

	acpi_object_t package_entry;

	// detect battery information from the returned package
	acpi_eval_package(&object, BIF_POWER_UNIT, &package_entry);
	batteries[index]->power_unit = package_entry.integer;

	acpi_eval_package(&object, BIF_FULL_CAPACITY, &package_entry);
	batteries[index]->capacity = package_entry.integer;

	acpi_eval_package(&object, BIF_CHARGEABLE, &package_entry);
	batteries[index]->chargeable = package_entry.integer;

	acpi_eval_package(&object, BIF_VOLTAGE, &package_entry);
	batteries[index]->voltage = package_entry.integer;

	acpi_eval_package(&object, BIF_WARNING, &package_entry);
	batteries[index]->warning = package_entry.integer;

	acpi_eval_package(&object, BIF_LOW, &package_entry);
	batteries[index]->low = package_entry.integer;

	acpi_eval_package(&object, BIF_MODEL, &package_entry);
	strcpy(batteries[index]->model, package_entry.string);

	acpi_eval_package(&object, BIF_SERIAL, &package_entry);
	strcpy(batteries[index]->serial, package_entry.string);

	acpi_eval_package(&object, BIF_TYPE, &package_entry);
	strcpy(batteries[index]->type, package_entry.string);

	acpi_eval_package(&object, BIF_OEM_SPECIFIC, &package_entry);
	strcpy(batteries[index]->oem, package_entry.string);

	// the only two defined power units for batteries
	if(batteries[index]->power_unit != POWER_UNIT_MA && batteries[index]->power_unit != POWER_UNIT_MW)
	{
		kprintf("acpi: undefined power unit %d, ignoring battery.\n", batteries[index]->power_unit);
		return 1;
	}

	kprintf("acpi: battery model %s, type %s, OEM-specific data %s\n", batteries[index]->model, batteries[index]->type, batteries[index]->oem);

	kprintf("acpi: full charge capacity is %d ", batteries[index]->capacity);

	if(batteries[index]->power_unit == POWER_UNIT_MA)
		kprintf("mAh");
	else
		kprintf("mWh");

	kprintf(", voltage is %d mV\n", batteries[index]->voltage);
	return battery_update(index);
}

// battery_update(): Updates a battery's dynamic status
// Param:	size_t index - index of battery
// Return:	int - 0 on success

int battery_update(size_t index)
{
	// theoretically full charge level and measuring unit may change
	// so read _BIF() again
	char path[512];
	int eval_status;
	acpi_object_t object, package_entry;

	strcpy(path, batteries[index]->path);
	strcpy(path + strlen(path), "._BIF");

	eval_status = acpi_eval(&object, path);
	if(eval_status != 0)
		return eval_status;

	// get the full charge level and the power unit
	acpi_eval_package(&object, BIF_POWER_UNIT, &package_entry);
	batteries[index]->power_unit = package_entry.integer;

	acpi_eval_package(&object, BIF_FULL_CAPACITY, &package_entry);
	batteries[index]->capacity = package_entry.integer;

	// get battery status using _BST() package
	strcpy(path, batteries[index]->path);
	strcpy(path + strlen(path), "._BST");

	eval_status = acpi_eval(&object, path);
	if(eval_status != 0)
		return eval_status;

	// get the charging/discharging state and battery level
	acpi_eval_package(&object, BST_STATE, &package_entry);
	batteries[index]->state = package_entry.integer;

	acpi_eval_package(&object, BST_REMAINING, &package_entry);
	batteries[index]->remaining = package_entry.integer;

	kprintf("acpi: battery %s is ", batteries[index]->path);

	if(batteries[index]->state & BATTERY_CHARGING)
		kprintf("charging, ");
	else
		kprintf("discharging, ");

	int percentage;
	double remaining, max, percentage_dbl;

	remaining = (double)batteries[index]->remaining;
	max = (double)batteries[index]->capacity;
	percentage_dbl = (remaining / max) * 100.0;

	percentage = (int)percentage_dbl;

	kprintf("%d% \n", percentage);

	/*kprintf("acpi: max capacity is 0x%xd\n", batteries[index]->capacity);
	kprintf("acpi: remaining capacity is 0x%xd\n", batteries[index]->remaining);*/
	return 0;
}

\




