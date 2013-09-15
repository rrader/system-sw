#include <stdbool.h>
#include <time.h>

#include "allocator.h"


size_t amount = 0;
size_t page = 0;
bool verbose = false;

void read_args(int argc, char *argv[]);
void test_mem();
void get_stats();

#define N 100
#define max_size 5000
void *list[N];
int sizes[N];
int checksums[N];

int calc_checksum(int id) {
    int *val = (int*)list[id];
    int cs = *val;
    for(int i=0; i<(sizes[N]/sizeof(int))-1; i++) {
        val++;
        cs ^= *val;
    }
    return cs;
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    read_args(argc, argv);

    alc_init(amount, page);
    printf("\n");

    for(int i=0; i<N; i++) list[i] = NULL;

    for (int i=0; i<2000; i++) {
        printf("\n##################################\ni = %d ", i);
        int id = rand()%N;
        int var = rand()%4;
        int new_size = rand()%max_size;
        if (list[id]) {
            if (rand()%2) {
                printf("free\n");
                //check checksum
                int cs = 0;
                char *t = list[id];
                for(int j=0; j<sizes[id]; j++) {
                    cs ^= *(t + j);
                }
                if (cs != checksums[id]) {
                    printf("ERROR CHECKSUM on step %d (%X and %X)\n", i, checksums[id], cs);
                }

                mem_free(list[id]);
                list[id] = NULL;
            } else {
                printf("realloc\n");
                //check checksum
                int cs = 0;
                void *t = list[id];
                for(int j=0; j<sizes[id]; j++) {
                    cs ^= *((char*)t + j);
                }
                if (cs != checksums[id]) {
                    printf("ERROR CHECKSUM on step %d (%X and %X)\n", i, checksums[id], cs);
                }
                t = mem_realloc(list[id], new_size);

                if (t) {
                    list[id]=t;
                    sizes[id] = new_size;
                    checksums[id] = 0;
                    for(int j=0; j<new_size; j++) {
                        *((char*)(t) + j) = rand()%256;
                        checksums[id] ^= *((char*)(t) + j);
                    }
                }
            }
        } else {
            printf("alloc %d\n", new_size);
            void *a = mem_alloc(new_size);
            printf("list[%d] = %X\n", id, a);
            list[id] = a;
            if (list[id]) {
                sizes[id] = new_size;
                checksums[id] = 0;
                for(int j=0; j<new_size; j++) {
                    *((char*)(a) + j) = rand()%256;
                    checksums[id] ^= *((char*)(a) + j);
                }
            }
        }
    }

    for(int i=0; i<N; i++) if (list[i]) mem_free(list[i]);

    mem_dump();
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
        exit(0);
    }
}

