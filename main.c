#include <stdbool.h>

#include "allocator.h"


size_t amount = 0;
bool verbose = false;

void read_args(int argc, char *argv[]);

int main(int argc, char *argv[]) {
    read_args(argc, argv);

    alc_init(amount);
    printf("\n");
    mem_dump();
    printf("\n");
    void* d = mem_alloc(512);
    void* d2 = mem_alloc(10);
    void* d3 = mem_alloc(24);
    void* d4 = mem_alloc(396);
    
    mem_dump();
    mem_free(d);
    mem_dump();
    mem_free(d2);
    mem_dump();
    mem_free(d4);
    mem_dump();

    void *n_d3 = mem_realloc(d3, 600);
    mem_dump();


    mem_free(n_d3);
    mem_dump();
}

void read_args(int argc, char *argv[]) {
    if (1 >= argc) {
        printf("usage: ./alloc -m <amount> [-v]\n");
    }

    for(int i=1; i<argc; i++) {
        if (strcmp(argv[i], "-m") == 0) {
            amount = atoi(argv[i+1]);
            i++;
        }

        if (strcmp(argv[i], "-v") == 0) {
            verbose = true;
        }
    }
}
