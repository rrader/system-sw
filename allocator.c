#include "allocator.h"

alc_info mi;

struct header {
    char flags;
    size_t size;
    size_t prev_size;
};
#define header struct header
#define size_h sizeof(header)

#define OFFSET(x) ((int)x - (int)mi.memory)

#define LOG(x, ...) if (verbose) printf(x, __VA_ARGS__)
#define LOGs(x) LOG("%s", x)

int ceil_4(int i) {
    return (i & ~3) + ((i & 3)?4:0);
}

int floor_4(int i) {
    return (i & ~3);
}

void write_raw_header(int offset, bool busy, int size, int prev_size) {
    header h;
    if (busy)
        h.flags |= BUSY_FLAG;
    else
        h.flags &= ~BUSY_FLAG;
    
    h.size = size;
    h.prev_size = prev_size;
    memcpy((mi.memory + offset), &h, sizeof(h));
}

void write_header(int offset, bool busy, int size, int prev_size) {
    header *next_h = (void*)(mi.memory + offset + size + size_h);
    if (!busy && !(next_h->flags & BUSY_FLAG)) {
        write_header(offset, false, size + next_h->size + size_h, prev_size);
        LOGs("Block expanded\n");
        return;
    }

    write_raw_header(offset, busy, size, prev_size);
    write_raw_header(OFFSET(next_h), next_h->flags & BUSY_FLAG, next_h->size, size);
    LOG("%s block created 0x%X (size: %d)\n", (busy?"Busy":"Free"), offset, size);
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

    LOG("header size: %d\n", size_h);

    // first block
    write_raw_header(0, true, ceil_4(size_h) - size_h, 0);

    // full free block
    int start_free = ceil_4(size_h);
    int free_size = floor_4(mi.amount - start_free - 2*size_h);

    write_raw_header(start_free, false, free_size, 0);

    LOG("Effective memory: %d (0x%X)\n", free_size, free_size);

    // last block
    int last_start = free_size + start_free + size_h;
    write_raw_header(last_start, true, mi.amount - (last_start + size_h), 0);

    return info;
}

void *mem_alloc(size_t size) {
    header *h;
    header *prev_h;
    prev_h = mi.memory + 0;
    
    int offset = prev_h->size + size_h;
    while (offset < mi.amount) {
        h = mi.memory + offset;
        if ((h->size >= size) && !(h->flags & BUSY_FLAG)) {
            int ret_size = ceil_4(size);

            //is there space for free block?
            if (h->size - ret_size > size_h) {
                //free block next to new
                int free_offset = offset + ret_size + size_h;
                int free_size = h->size - ret_size - size_h;

                write_header(free_offset,
                             false, free_size,
                             ret_size);
            } else {
                ret_size = h->size;
            }
            //new block
            write_header(offset, true, ret_size, prev_h->size);
            
            return mi.memory + offset + size_h;
        }
        offset += h->size + size_h;
        prev_h = h;
    }
    LOGs("error: mem_alloc\n");
    return NULL;
}

void *mem_realloc(void *addr, size_t size) {
    if (addr) {
        header *h = addr - size_h;
        header *prev_h = addr - h->prev_size - 2*size_h;
        header *next_h = addr + h->size;
        int ret_size = ceil_4(size);

        if (size < h->size) {
            //is there space for free block
            int free_size = h->size - size_h - ret_size;
            if (free_size > 0) {
                size_t free_offset = OFFSET(h) + ret_size + size_h;
                write_header(free_offset,
                             false, free_size,
                             ret_size);
            } else {
                ret_size = h->size;
            }

            write_header(OFFSET(h), true, ret_size, prev_h->size);

            return addr;
        } else {
            //right:
            if (!(next_h->flags & BUSY_FLAG) &&
                next_h->size + h->size + size_h >= ret_size) {
                int available = next_h->size + h->size + size_h;
                int free_offset = OFFSET(h) + size_h + ret_size;
                int free_size = h->size + size_h + next_h->size - ret_size - size_h;
                LOG("realloc: expand to right %d; prev shrink to %d\n", available, free_size);
                if (free_size>0) {
                    write_header(free_offset,
                                 false, free_size,
                                 ret_size);
                } else {
                    ret_size = h->size + next_h->size + size_h;
                }

                write_header(OFFSET(h),
                             true, ret_size,
                             prev_h->size);
                return addr;
            }

            //left:
            if (!(prev_h->flags & BUSY_FLAG) &&
                prev_h->size + h->size + size_h >= ret_size) {
                //new
                int available = prev_h->size + h->size + size_h;
                int prev_size = available - ret_size - size_h;
                int new_offset = OFFSET(h) + h->size - ret_size;
                LOG("realloc: expand to left %d; prev shrink to %d\n", available, prev_size);
                if (prev_size <= 0) {
                    new_offset = OFFSET(prev_h);
                    prev_size = prev_h->prev_size;
                    ret_size = available;
                }
                write_header(new_offset,
                             true, ret_size,
                             prev_size);

                if (prev_size>0) {
                    //shrink previous
                    write_header(OFFSET(prev_h),
                                 false, prev_size,
                                 prev_h->prev_size);
                }
                
                return mi.memory + new_offset + size_h;
            }

            //both
            if (!(prev_h->flags & BUSY_FLAG) && !(next_h->flags & BUSY_FLAG) &&
                prev_h->size + h->size + next_h->size + size_h*2 >= ret_size) {
                //new
                int available = prev_h->size + h->size + next_h->size + size_h*2;
                int free_size = available - ret_size - size_h;
                LOG("realloc: expand to both %d; prev shrink to %d\n", available, free_size);
                write_header(OFFSET(prev_h),
                             true, ret_size,
                             prev_h->prev_size);

                if (free_size > 0) {
                    int free_offset = OFFSET(prev_h) + ret_size + size_h;
                    write_header(free_offset,
                             false, free_size,
                             ret_size);
                }

                return mi.memory + OFFSET(prev_h) + size_h;
            }


            mem_free(addr);
            return mem_alloc(size);
        }

    } else {
        return mem_alloc(size);
    }
    LOGs("error: mem_realloc\n");
    return NULL;
}

void mem_free(void *addr) {
    header *h = addr - size_h;
    header *prev_h = addr - h->prev_size - 2*size_h;
    header *next_h = addr + h->size;
    int new_size = h->size;
    int prev_size = h->prev_size;

    if (! (prev_h->flags & BUSY_FLAG)) {
        LOGs("left hand block is free: merging\n");
        new_size += prev_h->size + size_h;
        prev_size = prev_h->prev_size;
        h = prev_h;
    }

    if (! (next_h->flags & BUSY_FLAG)) {
        LOGs("right hand block is free: merging\n");
        new_size += next_h->size + size_h;
    }

    LOG("block free 0x%X (size: %d)\n", OFFSET(h), new_size);

    write_header(OFFSET(h), false, new_size, prev_size);
}

void mem_dump() {
    if (!dumps) return;
    int s = 0;
    header *h;
    printf("state:\t          size:\t     prev_size:"
           "\t          addr:\t            hex:\n");

    int effective = 0;
    int count = 0;

    while (s < mi.amount) {
        h = mi.memory + s;
        printf("%5s\t%15d\t%15d\t%15d\t%15Xh\n",
               (h->flags & BUSY_FLAG)?"B":"F", h->size, h->prev_size, s, s);
        s += h->size + size_h;

        effective += h->size;
        count ++;
    }

    printf("Effective memory: %d - %f%%\n", effective, (effective/(float)(mi.amount))*100);
    printf("Block count: %d\n", count);
}

