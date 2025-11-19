#include "kernel/types.h"
#include "user.h"

int main(void) {
    printf("MAP2: calling mmap...\n");
    uint64 addr = mmap(0, 4096, 0, 0, -1, 0); // ignore prot/flags
    if(addr == (uint64)-1){
        printf("MAP2: mmap failed\n");
        exit(1);
    }

    *(int*)addr = 123;
    printf("MAP2: initial value = %d\n", *(int*)addr);

    int pid = fork();
    if(pid == 0){
        // child
        printf("MAP2: child sees value = %d\n", *(int*)addr);
        *(int*)addr = 456;
        printf("MAP2: child updated value = %d\n", *(int*)addr);
        exit(0);
    } else {
        // simple delay
        for(volatile int i=0;i<1000000;i++);
        printf("MAP2: parent sees value after child = %d\n", *(int*)addr);
        wait(0);
    }

    printf("MAP2: done\n");
    exit(0);
}

