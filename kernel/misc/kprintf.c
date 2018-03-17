
/*
 * lux OS kernel
 * copyright (c) 2018 by Omar Mohammad
 */

#include <version.h>
#include <kprintf.h>
#include <string.h>
#include <io.h>
#include <timer.h>
#include <va_list.h>	// for formatting
#include <tty.h>
#include <lock.h>

uint16_t com1_base;
uint8_t com1_last_byte = 0;
char debug_mode = 0;
lock_t com1_mutex = 0;

void com1_wait();
void com1_send_byte(char);
void com1_send(char *);
extern uint8_t kernel_debug;

// kprint_init(): Initializes the kernel debug messages
// Param:	Nothing
// Return:	Nothing

void kprint_init()
{
	debug_mode = 0;

	// configure the serial port if the kernel params say so
	if(kernel_debug == 2)
	{
		uint16_t *ptr = (uint16_t*)0x400;
		com1_base = ptr[0];
	} else
	{
		com1_base = 0;
		return;
	}

	outb(com1_base+1, 1);		// interrupt with data
	iowait();

	outb(com1_base+3, 0x80);	// enable DLAB
	iowait();

	outb(com1_base, 2);
	iowait();

	outb(com1_base+1, 0);
	iowait();

	outb(com1_base+3, 3);		// disable DLAB
	iowait();

	outb(com1_base+2, 0xC7);	// enable FIFO
	iowait();

	com1_send(VERSION);
	com1_send("\n");
}

// com1_wait(): Waits for the serial port to be ready
// Param:	Nothing
// Return:	Nothing

void com1_wait()
{
	while((inb(com1_base+5) & 0x20) == 0);
}

// com1_send_byte(): Sends a byte to the serial port
// Param:	char byte - byte to be sent
// Return:	Nothing

void com1_send_byte(char byte)
{
	if(debug_mode != 0)
		tty_put(byte, 0);

	com1_last_byte = byte;
	if(!com1_base)
		return;

	if(byte == '\n')
	{
		com1_wait();
		outb(com1_base, 13);

		com1_wait();
		outb(com1_base, 10);
	} else
	{
		com1_wait();
		outb(com1_base, byte);
	}
}

// com1_send(): Sends a string through the serial port
// Param:	char *string - string to be sent
// Return:	Nothing

void com1_send(char *string)
{
	//if(!com1_base) return;

	while(string[0] != 0)
	{
		com1_send_byte(string[0]);
		string++;
	}
}

// sprintf(): Formats a string
// Param:	char *dest - destination
// Param:	const char *string - formatting string
// Return:	int - size of destination

int sprintf(char *dest, const char *string, ...)
{
	char conv_str[32];
	uint32_t val32;
	uint64_t val64;

	char *strptr;

	va_list params;
	va_start(params, string);

	while(string[0] != 0)
	{
		if(string[0] == '%' && string[1] == '%')
		{
			dest[0] = '%';
			dest[1] = '%';
			string += 2;
			dest += 2;
			continue;
		}

		// print character
		if(string[0] == '%' && string[1] == 'c')
		{
			val32 = va_arg(params, uint32_t);
			dest[0] = (char)val32;
			string += 2;
			dest++;
			continue;
		}

		// print string
		if(string[0] == '%' && string[1] == 's')
		{
			strptr = va_arg(params, char *);
			memcpy(dest, strptr, strlen(strptr));
			string += 2;
			dest += strlen(strptr);
			continue;
		}

		// print decimal
		if(string[0] == '%' && string[1] == 'd')
		{
			val32 = va_arg(params, uint32_t);
			memcpy(dest, dec_to_string(val32, conv_str), strlen(dec_to_string(val32, conv_str)));
			string += 2;
			dest += strlen(dec_to_string(val32, conv_str));
			continue;
		}

		// print hex byte
		if(string[0] == '%' && string[1] == 'x' && string[2] == 'b')
		{
			val32 = va_arg(params, uint32_t);
			memcpy(dest, hex8_to_string((uint8_t)val32, conv_str), strlen(conv_str));
			string += 3;
			dest += strlen(conv_str);
			continue;
		}

		// print hex word
		if(string[0] == '%' && string[1] == 'x' && string[2] == 'w')
		{
			val32 = va_arg(params, uint32_t);
			memcpy(dest, hex16_to_string((uint16_t)val32, conv_str), strlen(conv_str));
			string += 3;
			dest += strlen(conv_str);
			continue;
		}

		// print hex dword
		if(string[0] == '%' && string[1] == 'x' && string[2] == 'd')
		{
			val32 = va_arg(params, uint32_t);
			memcpy(dest, hex32_to_string(val32, conv_str), strlen(conv_str));
			string += 3;
			dest += strlen(conv_str);
			continue;
		}

		// print hex qword
		if(string[0] == '%' && string[1] == 'x' && string[2] == 'q')
		{
			val64 = va_arg(params, uint64_t);
			memcpy(dest, hex64_to_string(val64, conv_str), strlen(conv_str));
			string += 3;
			dest += strlen(conv_str);
			continue;
		}

		dest[0] = string[0];
		string++;
		dest++;
	}

	dest[0] = 0;
	va_end(params);
	return strlen(dest);
}

// kprintf(): Formats and sends a debug message through the serial port
// Param:	char *string - formatting string
// Return:	Nothing

void kprintf(char *string, ...)
{
	acquire_lock(&com1_mutex);

	if(debug_mode != 0)
		tty_lock(0);	// tty0 is the kernel's log

	//if(!com1_base) return;

	char conv_str[32];
	uint32_t val32;
	uint64_t val64;

	char *strptr;

	if(com1_last_byte < 32)		// space
	{
		// print uptime
		com1_send_byte('[');
		com1_send(hex64_to_string(global_uptime, conv_str));
		com1_send("] ");
	}

	va_list params;
	va_start(params, string);

	while(string[0] != 0)
	{
		if(string[0] == '%' && string[1] == '%')
		{
			com1_send("%%");
			string += 2;
			continue;
		}

		// print character
		if(string[0] == '%' && string[1] == 'c')
		{
			val32 = va_arg(params, uint32_t);
			com1_send_byte((char)val32);
			string += 2;
			continue;
		}

		// print string
		if(string[0] == '%' && string[1] == 's')
		{
			strptr = va_arg(params, char *);
			com1_send(strptr);
			string += 2;
			continue;
		}

		// print decimal
		if(string[0] == '%' && string[1] == 'd')
		{
			val32 = va_arg(params, uint32_t);
			com1_send(dec_to_string(val32, conv_str));
			string += 2;
			continue;
		}

		// print hex byte
		if(string[0] == '%' && string[1] == 'x' && string[2] == 'b')
		{
			val32 = va_arg(params, uint32_t);
			com1_send(hex8_to_string((uint8_t)val32, conv_str));
			string += 3;
			continue;
		}

		// print hex word
		if(string[0] == '%' && string[1] == 'x' && string[2] == 'w')
		{
			val32 = va_arg(params, uint32_t);
			com1_send(hex16_to_string((uint16_t)val32, conv_str));
			string += 3;
			continue;
		}

		// print hex dword
		if(string[0] == '%' && string[1] == 'x' && string[2] == 'd')
		{
			val32 = va_arg(params, uint32_t);
			com1_send(hex32_to_string(val32, conv_str));
			string += 3;
			continue;
		}

		// print hex qword
		if(string[0] == '%' && string[1] == 'x' && string[2] == 'q')
		{
			val64 = va_arg(params, uint64_t);
			com1_send(hex64_to_string(val64, conv_str));
			string += 3;
			continue;
		}

		com1_send_byte(string[0]);
		string++;
	}

	va_end(params);

	if(debug_mode != 0)
		tty_unlock(0);

	release_lock(&com1_mutex);
}





