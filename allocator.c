#include "allocator.h"

alc_info mi;

#define LOG(x, ...) if (verbose) printf(x, __VA_ARGS__)
#define LOGs(x) LOG("%s", x)

int ceil_4(int i) {
    return (i & ~3) + ((i & 3)?4:0);
}

int floor_4(int i) {
    return (i & ~3);
}

alc_info alc_init(size_t amount) {
    alc_info info;
    info.amount = amount;
    LOG("Memory: %d (0x%X)\n", amount, amount);

    void *mem = malloc(amount);
    if (!mem) exit(1);
    LOGs("Allocated successful\n");

    info.memory = mem;

    mi = info;

    return info;
}

void *mem_alloc(size_t size) {

    return NULL;
}

void *mem_realloc(void *addr, size_t size) {

    return NULL;
}

void mem_free(void *addr) {

}

void mem_dump() {

}

