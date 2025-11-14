/*
    Author: Dakota Goldstein
    dgol720
    Assignment 2 
    file: mycalloc3.c
*/
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <stdalign.h>
#include <stddef.h>
// Alignment requirement: 16 bytes
#define ALIGNMENT 16
#define ALIGN(x) (((x) + ALIGNMENT - 1) & ~(ALIGNMENT - 1))
#define HDR_SIZE  ALIGN(sizeof(struct chunk))
#define MIN_SPLIT (HDR_SIZE + ALIGNMENT) 
#define HEAP_BYTES (64 * 1024)   //grow heap in 64 KB sections
//where we left off in the free list

static size_t g_max_alloc = (size_t)-1; 
void set_max_alloc(size_t bytes) { g_max_alloc = bytes; }
struct chunk{
    size_t size;
    struct chunk *next;
};
static struct chunk *flist = NULL;
static struct chunk **cursor = &flist;
static struct chunk *last_grown = NULL;
/*Picking a block to use, need to find the free block that fits the data we need*/
static struct chunk **find_fit(size_t need){
    struct chunk **p_to_ptr = (*cursor ? cursor : &flist); 
    while (*p_to_ptr && (*p_to_ptr)->size < need) p_to_ptr = &(*p_to_ptr)->next;
    if (*p_to_ptr) {
        cursor = p_to_ptr; return p_to_ptr; 
    }
    p_to_ptr = &flist;
    while ( p_to_ptr != cursor && *p_to_ptr && (*p_to_ptr)->size < need) p_to_ptr = &(*p_to_ptr)->next;
    if (p_to_ptr != cursor && *p_to_ptr) {
        cursor = p_to_ptr; 
        return p_to_ptr; 
    }

    return NULL; //no fit
}
static int grow_heap(size_t need){
    size_t want = HDR_SIZE + need; //overhead for the head ptr
    if (want < HEAP_BYTES) want = HEAP_BYTES;
    void *temp = sbrk(want); //using a temp pointer to run the sbrk then cast to chunk to finish                
    if (temp == (void*)-1) return 0;
    struct chunk *cnk = (struct chunk*)temp;
    last_grown = cnk;
    cnk->size = want - HDR_SIZE;            
    cnk->next = flist;                      
    flist = cnk;
    return 1;
}
static struct chunk* coalesce(struct chunk *cnk){
    // Merge any immediate right neighbor(s): block starting exactly at cnk_end
    while(1) {
        char *cnk_end = (char*)cnk + HDR_SIZE + cnk->size;
        struct chunk **p_to_ptr = &flist;
        int merged = 0;
        while (*p_to_ptr) {
            if ((char*)(*p_to_ptr) == cnk_end) {
                struct chunk *right = *p_to_ptr;//unlink right neighbor
                *p_to_ptr = right->next;
                cnk->size += HDR_SIZE + right->size;//absorb it
                merged = 1;
                break;
            }
            p_to_ptr = &(*p_to_ptr)->next;
        }
        if (!merged) break;
    }
    //Merge any immediate left neighbor(s): block whose end equals cnk_start
    while(1) {
        char *cnk_start = (char*)cnk; //char traversing easier
        struct chunk **p_to_ptr = &flist;
        int merged = 0;
        while (*p_to_ptr) {
            char *p_end = (char*)(*p_to_ptr) + HDR_SIZE + (*p_to_ptr)->size;
            if (p_end == cnk_start) {
                struct chunk *left = *p_to_ptr;//unlink left neighbor
                *p_to_ptr = left->next;
                left->size += HDR_SIZE + cnk->size;//absorb current into left
                cnk = left; //new merged block is at left
                merged = 1;
                break;
            }
            p_to_ptr = &(*p_to_ptr)->next;
        }
        if (!merged) break;
    }
    return cnk;
}
void free(void *ptr){
    if (!ptr) return; //Return if already freed
    struct chunk *cnk = (struct chunk *)((char*)ptr - HDR_SIZE); //Use char* so math is done in bites and we dont overshoot
    cnk = coalesce(cnk);//run the merger
    cnk->next = flist;//push onto free list
    flist = cnk;
    cursor = &flist; 
}
void *calloc(size_t nmemb, size_t size){
    if((nmemb == 0) || (size == 0)) return NULL;
    if (nmemb > SIZE_MAX / size) return NULL; //prevent nmemb*size overflow
    size_t need = ALIGN(nmemb * size);
    struct chunk **p_to_ptr = find_fit(need);
    if (!p_to_ptr) {//If there is no fit grow the heap so there is space
        if (!grow_heap(need)) return NULL;
        p_to_ptr = find_fit(need);
        if (!p_to_ptr) return NULL;
    }
    //Increments through the pointers to the blocks to find a block big enough
    
    
    struct chunk *cnk = *p_to_ptr;
    *p_to_ptr = cnk->next;
    cursor = p_to_ptr;
    if (cnk->size >= need + MIN_SPLIT) {
        char *base = (char*)cnk;
        struct chunk *rem = (struct chunk*)(base + HDR_SIZE + need);
        rem->size = cnk->size-need-HDR_SIZE;
        rem->next = flist; 
        flist = rem;
        cnk->size = need;
    }

    void *user = (char*)cnk + HDR_SIZE;
    if (cnk != last_grown) memset(user, 0, need); else last_grown = NULL;
    return user;

}


