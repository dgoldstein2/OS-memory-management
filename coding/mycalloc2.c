/*
    Author: Dakota Goldstein
    dgol720
    Assignment 2 
    file: mycalloc2.c
*/
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <stddef.h>
// Alignment requirement: 16 bytes
#define ALIGNMENT 16
#define ROUNDS 10
#define LOOP 100000
#define ALIGN(x) (((x) + ALIGNMENT - 1) & ~(ALIGNMENT - 1))
static size_t g_max_alloc = (size_t)-1; 
void set_max_alloc(size_t bytes) { g_max_alloc = bytes; }
struct chunk{
    int size;
    struct chunk *next;
};
struct chunk *flist = NULL;
void free(void *ptr){
    if (!ptr) return; //Return if already freed
    struct chunk *cnk = ((struct chunk *)ptr) - 1; //Recover header pointer
    cnk->next = flist; //Link back to free
    flist = cnk; 
}
void *calloc(size_t nmemb, size_t size){
    if((nmemb == 0) || (size == 0)) return NULL;
    size_t need = nmemb * size;
    struct chunk **p_to_ptr = &flist;
    //Increments through the pointers to the blocks to find a block big enough
    while (*p_to_ptr && (*p_to_ptr)->size < need) { 
        p_to_ptr = &(*p_to_ptr)->next;
    }
    if (*p_to_ptr) {
        struct chunk *cnk = *p_to_ptr; 
        *p_to_ptr = cnk->next;
        void *user = (void*)(cnk + 1);
        memset(user, 0, need); //Zero out using memset
        return user;
    }
    size_t total = sizeof(struct chunk) + need + ALIGNMENT;
    void *raw = sbrk(total);
    if (raw == (void*)-1) return NULL;
    uintptr_t after_hdr = (uintptr_t)raw + sizeof(struct chunk); //Place pointer right after head ptr
    void *user = (void*)ALIGN(after_hdr);
    struct chunk *cnk = ((struct chunk *)user) - 1;
    cnk->size = need;
    cnk->next = NULL;
    memset(user, 0, need);
    return user;
}


