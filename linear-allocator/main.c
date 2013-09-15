#include <stdbool.h>
#include <time.h>

#include "allocator.h"


size_t amount = 0;
bool verbose = false;
char cs_base = 230;
bool dumps = false;
bool stats = false;
bool test = false;

void read_args(int argc, char *argv[]);
void test_mem();
void get_stats();

int main(int argc, char *argv[]) {
    srand(time(NULL));
    read_args(argc, argv);

    alc_init(amount);
    printf("\n");
    mem_dump();
    printf("\n");

    if (test) {
        test_mem();
    }

    if (stats) {
        get_stats();
    }
}

void read_args(int argc, char *argv[]) {
    for(int i=1; i<argc; i++) {
        if (strcmp(argv[i], "-m") == 0) {
            amount = atoi(argv[i+1]);
            i++;
        } else

        if (strcmp(argv[i], "-v") == 0) {
            verbose = true;
        } else

        if (strcmp(argv[i], "-d") == 0) {
            dumps = true;
        } else

        if (strcmp(argv[i], "-s") == 0) {
            stats = true;
        }

        if (strcmp(argv[i], "-t") == 0) {
            test = true;
        }
    }

    if ((0 == amount) || (1 >= argc)) {
        printf("usage: ./alloc -m <amount> [-v] [-d] [-s] [-t]\n");
        printf("\n -m [MEMORY]\tmemory amount\n -v\t\tverbose output\n -d\t\tprint dumps\n -s\t\tstatistics\n -t\t\tdo tests\n\n");
        exit(0);
    }
}


void test_mem() {
    int test_size = 2;
    void* list[amount/test_size];
    int list_cs[amount/test_size];
    int list_sizes[amount/test_size];

    int count = 0;
    void *p;

    int block_size = rand() % 100 + test_size;
    for(int i=0;i<rand()%20; i++) {
        p = mem_alloc(block_size);
        // printf("%d\n", p - mi.memory);
        char cs = cs_base;
        char value;
        for (int i=0; i<block_size; i++) {
            value = rand() % 255;
            *(char*)(p+i) = value;
            cs ^= value;
        }
        list_sizes[count] = block_size;
        list_cs[count] = cs;
        list[count++] = p;
        block_size = rand() % 100 + test_size;
    }
    mem_dump();
    // count --;
    printf("Allocated %d blocks of %d..%d b\n", count, test_size, test_size+100);


//CHECK CS
    bool cs_check = true;

    for(int i=0; i<count; i++) {
        char cs = cs_base;
        for (int j=0; j<list_sizes[i]; j++) {
            cs ^= *((char*)(list[i]+j));
        }
        cs_check = cs_check && (cs == list_cs[i]);
    }

    printf("Control sum check: %d\n", cs_check);

    printf("Realloc...\n");

//REALLOC
    int count2 = 0;
    // int test_size2 = 16;
    for(int i=0; i<count; i++) {
        block_size = rand() % 100 + test_size;
        printf("%d\n", block_size);
        void *p = mem_realloc(list[i], block_size);
        if (!p) {
            printf("NULL\n");
            count2++;
            continue;
        }
        char cs = cs_base;
        char value;
        for (int i=0; i<block_size; i++) {
            value = rand() % 255;
            *(char*)(p+i) = value;
            cs ^= value;
        }
        list_sizes[count2] = block_size;
        list_cs[count2] = cs;
        list[count2++] = p;
    }
    mem_dump();


//CHECK CS
    cs_check = true;

    for(int i=0; i<count2; i++) {
        char cs = cs_base;
        for (int j=0; j<list_sizes[i]; j++) {
            cs ^= *((char*)(list[i]+j));
        }
        cs_check = cs_check && (cs == list_cs[i]);
    }

    printf("Control sum check: %d\n\n", cs_check);

    printf("Free all\n");

    for(int i=0; i<count2; i++) mem_free(list[i]);
    mem_dump();
}

void alloc_test(int test_size) {
    void* list[amount/test_size];
    clock_t uptime = clock() / (CLOCKS_PER_SEC / 1000);
    int count = 0;
    while ((list[count++] = mem_alloc(test_size)));
    clock_t uptime2 = clock() / (CLOCKS_PER_SEC / 1000);
    int delta = uptime2-uptime;
    printf("Allocated %d blocks of %db in %f seconds. Avg: %f\n",
            count, test_size, delta/1000., (delta/1000.)/count);

    uptime = clock() / (CLOCKS_PER_SEC / 1000);
    for(int i=0; i<count-1; i++) mem_free(list[i]);
    uptime2 = clock() / (CLOCKS_PER_SEC / 1000);
    delta = uptime2-uptime;
    printf("Released  %d blocks of %db in %f seconds. Avg: %f\n",
            count, test_size, delta/1000., (delta/1000.)/count);

}

void get_stats() {
    // alloc_test(32);
    alloc_test(512);
    alloc_test(1024);
    alloc_test(16*1024);
}
