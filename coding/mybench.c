#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include "rand.h"
#include <stddef.h>
void set_max_alloc(size_t bytes);
#define BUFFER 100
#define LOOP 100000
#define ROUNDS 10
int main(void) {
    set_max_alloc(64ULL * 1024 * 1024);
    //rolling buffer of live allocations (initialised to NULL by calloc)
    unsigned char **buffer = (unsigned char**)calloc((size_t)BUFFER, sizeof(*buffer));
    assert(buffer != NULL);
    
    srand((unsigned)time(NULL));
    void *init = sbrk(0);
    void *current;
    printf("The initial top of the heap is %p.\n", init);
    for (int r = 0; r < ROUNDS; r++) {
        for (int i = 0; i < LOOP; i++) {
            int index = rand() % BUFFER;              
            if (buffer[index]) {                            
                free(buffer[index]);
                buffer[index] = NULL;
            } else {                                
                size_t size = (size_t)request();
                buffer[index] = (unsigned char*)calloc(1, size);
            }
        }
        current = sbrk(0);
        int allocated = (int)((current - init) / 1024);
        printf("%d\n", r);
        printf("The current top of the heap is %p.\n", current);
        printf("Increased by %d Kbyte\n", allocated);
    }

    // cleanup
    for (int i = 0; i < BUFFER; i++) free(buffer[i]);
    free(buffer);
    return 0;
}