#include <stdlib.h>
#include <math.h>
#include "allocator.h"

#define LOG(x, ...) do if (verbose) printf(x, __VA_ARGS__); while(0)
#define LOGs(x) LOG("%s", x)
#define LOGi(x, ...) do printf(x, __VA_ARGS__); while(0)
#define LOGsi(x) LOGi("%s", x)

#define ceil_4(i) ((i & ~3) + ((i & 3)?4:0))
#define floor_4(i) (i & ~3)
#define min(a,b) (((a)>(b))?(b):(a))
#define STATE_BUSY (descriptor*)-1
#define STATE_MULTIPAGE (descriptor*)-2
#define STATE_FREE (descriptor*)0
#define EMPTY_CLASS (descriptor*)0
#define PAGE_ADDR(i) ((void*)(mi.memory + (mi.page_size * i)))
#define BLOCK_ADDR(desc, size, id) ((void**)((char*)(desc) + (id)*(size)))
#define PAGE_OF_ADDR(a) (((char*)a-(char*)mi.memory) / mi.page_size)
#define MAX_PAGE_BLOCKS(i) (mi.page_size / (int)pow(2, i))

uint round_pow_2(uint v) {
    float f = (float)(v - 1);  
    return 1U << ((*(unsigned int*)(&f) >> 23) - 126);
}

//page descriptor
struct descriptor
{
    int free_count;
    int index;
    char class;
    void **block;
    struct descriptor *next, *prev;
};
#define descriptor struct descriptor

struct alc_info
{
    size_t amount;
    void *memory;
    size_t page_count;
    size_t page_size;
    descriptor **descriptors;

    size_t class_count;
    descriptor **classes;
    char *page_classes;
};
#define alc_info struct alc_info

alc_info mi;


void alc_init(size_t amount, size_t page) {
    alc_info info;
    info.amount = amount;
    info.page_size = page;
    info.page_count = amount/page;
    LOG("Memory: %d\n", info.amount);
    LOG("Page size: %d\n", info.page_size);
    LOG("Page count: %d\n", info.page_count);

    void *mem = malloc(amount);
    if (!mem) exit(1);
    LOGs("Allocated successful\n");

    info.memory = mem;

    mi = info;

    mi.descriptors = (descriptor**)mi.memory;

    mi.class_count = log2(mi.page_size/2);
    mi.classes = mi.descriptors + mi.page_count;
    mi.page_classes = mi.classes + mi.class_count;

    mi.descriptors[0] = STATE_BUSY;
    for (int i=1; i<mi.page_count; i++) {
        mi.descriptors[i] = STATE_FREE;
    }

    for (int i=1; i<mi.class_count; i++) {
        mi.classes[i] = EMPTY_CLASS;
    }

    for (int i=1; i<mi.page_count; i++) {
        mi.page_classes[i] = 0;
    }
}

void append_page_to_class(int class, descriptor *d) {
    if (mi.classes[class] == EMPTY_CLASS) {
        LOG("class %d created\n", class);
        mi.classes[class] = d;
    } else {
        LOG("class %d appending...\n", class);
        descriptor *last = mi.classes[class];
        while (last->next) {
            last = last->next;
        }
        last->next = d;
        d->prev = last;
        LOG("class %d appended\n", class);
    }
    LOG("TEST page_classes[%d] = %d\n", d->index, class);
    mi.page_classes[d->index] = class;
}

void remove_from_class(int id) {
    descriptor *d = mi.descriptors[id];
    LOG("removed %d from class %d\n", id, d->class);
    if (mi.classes[d->class] == d)
        mi.classes[d->class] = d->next;
    if (d->next)
        d->next->prev = d->prev;
    if (d->prev)
        d->prev->next = d->next;
}

descriptor *alloc_page(size_t size) {
    LOG("alloc page: size %d. ", size);
    for (int i=1; i<mi.page_count; i++) {
        if (mi.descriptors[i] == STATE_FREE) {
            LOG("p. %d; ", i);
            mi.descriptors[i] = STATE_BUSY;
            descriptor d;
            d.index = i;
            d.free_count = mi.page_size / size;
            d.next = d.prev = NULL;
            d.class = log2(size);
            
            descriptor *descp;
            
            if ((size > sizeof(descriptor)) && size < sizeof(descriptor)*2) {
                //save descriptor in page
                LOGs("local descriptor; ");
                descp = PAGE_ADDR(i);
                d.block = PAGE_ADDR(i) + size;
                d.free_count --;
            } else {
                LOGs("delegated descriptor; \n\t");
                //allocate description in another page
                descp = mem_alloc(sizeof(d));
                if (!descp)
                    return NULL;
                d.block = PAGE_ADDR(i);
            }
            *descp = d;

            for (int j=0; j<d.free_count-1; j++) {
                *BLOCK_ADDR(descp->block, size, j) = (char*)d.block + (j+1)*size;
            }
            *BLOCK_ADDR(descp->block, size, d.free_count-1) = NULL; // last block
            LOG("%d blocks linked\n", d.free_count);

            mi.descriptors[i] = descp;

            append_page_to_class(d.class, mi.descriptors[i]);

            return mi.descriptors[i];
        }
    }
    LOGs("NO FREE PAGE\n");
    return NULL;
}

int find_free_pages(int num) {
    int r = 0;
    for (int i=0; i<mi.page_count; i++) {
        if (mi.descriptors[i] == STATE_FREE)
            r ++;
        else
            r = 0;

        if (r >= num) {
            return (i+1) - num;
        }
    }
    return -1;
}

void *mem_alloc(size_t size_in) {
    size_in = (size_in>=sizeof(descriptor))?size_in:sizeof(descriptor);
    uint size = round_pow_2(size_in);
    uint class = log2(size);

    LOG("alloc %d: class %d\n", size, class);
    if (class < mi.class_count) {

        descriptor *page = NULL;
        if (mi.classes[class] == EMPTY_CLASS) {
            // alloc new page for class
            page = alloc_page(size);
        } else {
            LOG("Class %d exists\n", class);
            mem_dump();
            page = mi.classes[class];
            printf("%X, %d\n", page, PAGE_OF_ADDR(page));
        }
        if (!page) return NULL;

        void *ret = page->block;
        printf("%X\n", ret);

        page->block = *(int**)(page->block);
        page->free_count --;
        if (!page->block) {
            LOG("page %d is busy now (class %d)\n", page->index, class);
            descriptor *next = page->next;
            if (page->next)
                page->next->prev = page->prev;
            if (page->prev)
                page->prev->next = page->next;
            if (PAGE_OF_ADDR(mi.descriptors[page->index]) != page->index) {
                mem_free(mi.descriptors[page->index]);
            }
            mi.descriptors[page->index] = STATE_BUSY;
            mi.classes[class] = next;
        }
        LOG("allocated successful %X\n", ret);
        return ret;
    } else {
        //multipage block
        int num = (size + mi.page_size/2)/mi.page_size;
        LOG("Multipage block (%d blocks)\n", num);
        int page = find_free_pages(num);
        if (page == -1) {
            LOG("No coutinuous %d free pages\n", num);
            return NULL;
        }
        LOG("Allocating started from %d\n", page);
        for (int i=page; i<page+num; i++) {
            mi.descriptors[i] = STATE_MULTIPAGE;
            mi.page_classes[i] = -1;
        }
        mi.page_classes[page] = num;
        LOG("NUMBER %d\n", num);

        return PAGE_ADDR(page);
    }

    LOGs("ERROR mem_alloc\n");
    return NULL;
}

void *mem_realloc(void *addr, size_t size) {
    LOG("Realloc: new size %d\n", size);
    void *new = mem_alloc(size);
    if (!new) return NULL;
    int copy_size;
    if (mi.descriptors[PAGE_OF_ADDR(addr)] == STATE_MULTIPAGE)
        copy_size = min(mi.page_classes[PAGE_OF_ADDR(addr)] * mi.page_size, size);
    else 
        copy_size = min(pow(2,mi.page_classes[PAGE_OF_ADDR(addr)]), size);
    LOG("Old size: %d\n", copy_size);
    memcpy(new, addr, copy_size);
    mem_free(addr);
    return new;
}

void free_page(int page_id) {
    LOG("empty page %d\n", page_id);
    mi.page_classes[page_id] = 0;
    remove_from_class(page_id);
    if (PAGE_OF_ADDR(mi.descriptors[page_id]) != page_id)
        //delegated
        mem_free(mi.descriptors[page_id]);
    mi.descriptors[page_id] = STATE_FREE;
}

void mem_free(void *addr) {
    int page_id = PAGE_OF_ADDR(addr);
    descriptor *d = mi.descriptors[page_id];
    LOG("Free block from page %d\n", page_id);

    if (d == STATE_MULTIPAGE) {
        //TODO:
        int page_num = mi.page_classes[page_id];
        LOG("free multipage block of size %d\n", page_num);
        for (int i=page_id; i<page_id + page_num; i++) {
            mi.descriptors[i] = STATE_FREE;
            mi.page_classes[i] = 0;
        }
    } else
    if ((d != STATE_FREE) && (d != STATE_BUSY)) {
        d->free_count ++;
        *(void**)addr = d->block;
        d->block = addr;

        LOG("Page has %d/%d\n", d->free_count, MAX_PAGE_BLOCKS(d->class));
        if (d->free_count >= MAX_PAGE_BLOCKS(d->class))
            //empty page with delegated descriptor
            free_page(page_id);

        if ((d->free_count >= MAX_PAGE_BLOCKS(d->class)-1) &&
            (PAGE_OF_ADDR(mi.descriptors[page_id]) == page_id))
            //empty page with local descriptor
            free_page(page_id);

    } else {
        LOGs("BUSY page => page with one block\n");
        mi.descriptors[page_id] = STATE_BUSY;
        int size = pow(2, mi.page_classes[page_id]);
        if ((size > sizeof(descriptor)) && size < sizeof(descriptor)*2) {
            LOGs("local descriptor\n");
            d = PAGE_ADDR(page_id);
        } else {
            LOGs("delegated descriptor\n");
            d = mem_alloc(sizeof(descriptor));
            if (!d) {
                LOGs("ERROR ALLOCATING DESCRIPTOR!\n");
                return;
            }
        }
        d->free_count = 1;
        d->block = addr;
        d->index = page_id;
        d->class = mi.page_classes[page_id];
        d->next = d->prev = NULL;
        *(void**)addr = NULL;
        append_page_to_class(d->class, d);
        mi.descriptors[page_id] = d;
    }
}

void mem_dump() {
    for(int i=0; i<mi.page_count; i++) {
        if (mi.descriptors[i] == STATE_FREE) {
            LOGi("%d: free page\n", i);
        } else
        if (mi.descriptors[i] == STATE_BUSY) {
            LOGi("%d: busy page class %d\n", i, mi.page_classes[i]);
        } else
        if (mi.descriptors[i] == STATE_MULTIPAGE) {
            if (mi.page_classes[i] != -1)
                LOGi("%d: multipage block of size %d\n", i, mi.page_classes[i]);
            else
                LOGi("%d: multipage block (continuation)\n", i);
        } else {
            LOGi("%d: page (%d free) class %d\n", i, mi.descriptors[i]->free_count, mi.page_classes[i]);
        }

    }

    LOGsi("==================\n");

    for(int i=0; i<mi.class_count; i++) {
        LOGi("%d (%d B):", i, (int)pow(2, i));
        if (mi.classes[i] == EMPTY_CLASS) {
            LOGsi("empty class\n");
        } else {
            LOGi("existing class %X\n", mi.classes[i]);
        }
    }
}

