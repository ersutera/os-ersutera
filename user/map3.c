#include "kernel/types.h"
#include "user/user.h"

int main(void) {
    printf("MAPTEST: starting\n");

    // Check free memory before mapping
    int free_before = freemem();
    printf("Free memory before mapping: %d KiB\n", free_before);

    // Map one shared page
    uint64 addr = mmap(0, 4096, 0, 0, -1, 0);
    if(addr == (uint64)-1){
        printf("MAPTEST: mmap failed\n");
        exit(1);
    }

    // Initialize value
    *(int*)addr = 0;

    int num_children = 3;
    int i;
    for(i = 0; i < num_children; i++){
        int pid = fork();
        if(pid == 0){
            // child
            *(int*)addr += (i+1) * 10;   // increment value by 10, 20, 30...
            printf("Child %d: updated value = %d\n", i+1, *(int*)addr);
            exit(0);
        }
    }

    // parent waits for all children
    for(i = 0; i < num_children; i++){
        wait(0);
    }

    // Final value in parent
    printf("MAPTEST: final value in parent = %d\n", *(int*)addr);

    // Unmap page
    munmap(addr, 4096);

    // Check free memory after cleanup
    int free_after = freemem();
    printf("Free memory after unmapping: %d KiB\n", free_after);

    if(free_after != free_before){
        printf("WARNING: possible kernel page leak!\n");
    } else {
        printf("No kernel pages leaked. Physical page ref counting works!\n");
    }

    printf("MAPTEST: done\n");
    exit(0);
}

