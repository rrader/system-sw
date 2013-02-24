#include <stdbool.h>

#include "allocator.h"


size_t amount = 0;
bool verbose = false;
char cs_base = 230;
bool dumps = false;

void read_args(int argc, char *argv[]);

int main(int argc, char *argv[]) {
    read_args(argc, argv);

    alc_init(amount);
    printf("\n");
    mem_dump();
    printf("\n");

    int test_size = 5;
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
    int test_size2 = 2;
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
        }
    }

    if ((0 == amount) || (1 >= argc)) {
        printf("usage: ./alloc -m <amount> [-v] [-d]\n");
        exit(0);
    }
}
