#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUSY_FLAG 1


struct alc_info
{
    size_t amount;
    void *memory;

};
#define alc_info struct alc_info

extern bool verbose;
extern alc_info main_info;


//================================
alc_info alc_init(size_t amount);

void *mem_alloc(size_t size);
void *mem_realloc(void *addr, size_t size);
void mem_free(void *addr);
void mem_dump();

#endif