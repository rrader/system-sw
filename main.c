#include <stdbool.h>
#include <time.h>

#include "allocator.h"


size_t amount = 0;
size_t page = 0;
bool verbose = false;
// char cs_base = 230;
// bool dumps = false;
// bool stats = false;
// bool test = false;

void read_args(int argc, char *argv[]);
void test_mem();
void get_stats();

int main(int argc, char *argv[]) {
    srand(time(NULL));
    read_args(argc, argv);

    alc_init(amount, page);
    printf("\n");


    // void *x;
    // for (int i=0; i<200; i++) {
    //     if (i%3 == 0) {
    //         x = mem_alloc(i);
    //     } else {
    //         if (x)
    //             mem_free(x);
    //     }
    // }
    void* x = mem_alloc(17);

    void* y = mem_alloc(15);
    // mem_alloc(15);
    // mem_alloc(15);
    // mem_alloc(15);
    // mem_alloc(15);
    // mem_alloc(15);
    void* z = mem_alloc(200);
    mem_realloc(z, 100);
    // mem_alloc(200);
    // mem_alloc(200);
    // mem_alloc(200);
    // void *x=mem_alloc(200);
    // mem_free(x);
    // mem_alloc(200);
    // mem_free(x);
    // mem_alloc(200);
    // mem_alloc(200);
    // mem_alloc(200);
    // mem_alloc(200);
    // mem_alloc(200);
    // void *x = mem_alloc(200);
    // mem_free(x);
    
    // mem_alloc(15);
    // mem_alloc(15);
    mem_dump();
    // printf("%d\n", );
}

void read_args(int argc, char *argv[]) {
    for(int i=1; i<argc; i++) {
        if (strcmp(argv[i], "-m") == 0) {
            amount = atoi(argv[i+1]);
            i++;
        } else

        if (strcmp(argv[i], "-p") == 0) {
            page = atoi(argv[i+1]);
            i++;
        } else

        if (strcmp(argv[i], "-v") == 0) {
            verbose = true;
        }
    }

    if ((0 == amount) || (1 >= argc)) {
        printf("usage: ./alloc -m <amount> -p <pagesize> [-v]\n");
        // printf("\n -m [MEMORY]\tmemory amount\n -v\t\tverbose output\n -d\t\tprint dumps\n -s\t\tstatistics\n -t\t\tdo tests\n\n");
        exit(0);
    }
}


void test_mem() {
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
