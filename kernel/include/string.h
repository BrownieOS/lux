
/*
 * lux OS kernel
 * copyright (c) 2018 by Omar Mohammad
 */

#pragma once

#include <types.h>

void *memmove(void *, const void *, size_t);
size_t strlen(const char *);
char *dec_to_string(uint64_t, char *);
char *hex4_to_string(uint8_t, char *);
char *hex8_to_string(uint8_t, char *);
char *hex16_to_string(uint16_t, char *);
char *hex32_to_string(uint32_t, char *);
char *hex64_to_string(uint64_t, char *);

void *memset(void *, int, size_t);


