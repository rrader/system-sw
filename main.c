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
        printf("usage: ./alloc -m <amount> [-v] [-d]\n");
        exit(0);
    }
}


void test_mem() {
    int test_size = 32;
    void* list[amount/test_size];
    int list_cs[amount/test_size];

    int count = 0;
    void *p;

    while ((p = mem_alloc(test_size))) {
        // printf("%d\n", p - mi.memory);
        char cs = cs_base;
        char value;
        for (int i=0; i<test_size; i++) {
            value = rand() % 255;
            *(char*)(p+i) = value;
            cs ^= value;
        }
        list_cs[count] = cs;
        list[count++] = p;
    }
    mem_dump();
    // count --;
    printf("Allocated %d blocks of %d b\n", count, test_size);


//CHECK CS
    bool cs_check = true;

    for(int i=0; i<count; i++) {
        char cs = cs_base;
        for (int j=0; j<test_size; j++) {
            cs ^= *((char*)(list[i]+j));
        }
        cs_check = cs_check && (cs == list_cs[i]);
    }

    printf("Control sum check: %d\n", cs_check);

    printf("Realloc...\n");

//REALLOC
    int count2 = 0;
    int test_size2 = 15;
    for(int i=0; i<count; i++) {
        void *p = mem_realloc(list[i], test_size2);
        
        char cs = cs_base;
        char value;
        for (int i=0; i<test_size2; i++) {
            value = rand() % 255;
            *(char*)(p+i) = value;
            cs ^= value;
        }
        list_cs[count2] = cs;
        list[count2++] = p;
    }
    mem_dump();


//CHECK CS
    cs_check = true;

    for(int i=0; i<count; i++) {
        char cs = cs_base;
        for (int j=0; j<test_size2; j++) {
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
