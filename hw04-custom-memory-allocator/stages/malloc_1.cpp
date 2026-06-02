#include <unistd.h>

#define MAX_ALLOCATION_SIZE 100000000

void * smalloc(size_t size){
    if(size == 0){
        return NULL;
    }
    if(size > MAX_ALLOCATION_SIZE){
        return NULL;
    }
    //increment the program break.
    void * program_break = sbrk(size);

    //if failed return null
    if(program_break == (void *) -1){
        return NULL;
    }
    return program_break;
}