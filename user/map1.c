#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main()
{
    printf("MAP1: calling mmap...\n");

    // single-page mmap test
    uint64 addr = mmap(0, 4096,
                       PROT_READ | PROT_WRITE,
                       MAP_ANON | MAP_PRIVATE,
                       -1,
                       0);

    if(addr == (uint64)-1){
        printf("MAP1: mmap failed\n");
        exit(1);
    }

    printf("MAP1: got address %p\n", (void*)addr);

    int *p = (int*)addr;
    *p = 777;

    printf("MAP1: read back value = %d\n", *p);

    printf("MAP1: done\n");
    exit(0);
}

