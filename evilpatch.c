#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

void* myFunc() {
    return NULL;
}

void reallocHeist(void* patch, void* buffer, int size) {
    void* vptr = realloc;
    intptr_t ptr = (intptr_t) vptr;
    size_t pagesize = sysconf(_SC_PAGESIZE);
    ptr = ptr & ~(pagesize - 1);
    mprotect((void*) ptr, pagesize, PROT_READ | PROT_WRITE | PROT_EXEC);
    memcpy(buffer, vptr, size);
    memcpy(vptr, patch, size);
}

void reallocRestore(void* buffer, int size) {
    void* vptr = realloc;
    intptr_t ptr = (intptr_t) vptr;
    size_t pagesize = sysconf(_SC_PAGESIZE);
    memcpy(vptr, buffer, size);
    ptr = ptr & ~(pagesize - 1);
    mprotect((void*) ptr, pagesize, PROT_READ | PROT_EXEC);
}
