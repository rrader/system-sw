#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


extern bool verbose;


//================================
void alc_init(size_t amount, size_t page);

void *mem_alloc(size_t size);
void *mem_realloc(void *addr, size_t size);
void mem_free(void *addr);
void mem_dump();

#endif