
/*
 * lux OS kernel
 * copyright (c) 2018 by Omar Mohammad
 */

#include <mm.h>
#include <string.h>

// kmalloc(): Allocates kernel memory
// Param:	size_t size - number of bytes to allocate
// Return:	void * - pointer to allocated memory, SSE-aligned

void *kmalloc(size_t size)
{
	if(!size)
		return NULL;

	size_t pages = (size + HEAP_ALIGNMENT + PAGE_SIZE - 1) >> PAGE_SIZE_SHIFT;

	void *ptr = (void*)(vmm_alloc(KERNEL_HEAP, pages, PAGE_PRESENT | PAGE_RW));
	if(!ptr)
		return NULL;

	size_t *header = (size_t*)(ptr);
	header[0] = pages;		// store number of pages
	header[1] = size;		// store number of bytes

	return ptr + HEAP_ALIGNMENT;
}

// kcalloc(): Allocates kernel memory
// Param:	size_t size - size of each entry
// Param:	size_t count - number of entries
// Return:	void * - pointer to allocated memory, size_t aligned

void *kcalloc(size_t size, size_t count)
{
	return kmalloc(size * count);
}

// krealloc(): Reallocates kernel memory
// Param:	void *ptr - pointer to memory
// Param:	size_t size - new size
// Return:	void * - pointer to memory

void *krealloc(void *ptr, size_t size)
{
	void *newptr = kmalloc(size);

	size_t *header = (size_t*)((size_t)ptr - HEAP_ALIGNMENT);
	size_t old_size = header[1];

	memcpy(newptr, ptr, old_size);

	kfree(ptr);
	return newptr;
}

// kfree(): Frees kernel memory
// Param:	void *ptr - pointer to memory
// Return:	Nothing

void kfree(void *ptr)
{
	size_t *header = (size_t*)((size_t)ptr - HEAP_ALIGNMENT);
	vmm_free((size_t)ptr, header[0]);
}



