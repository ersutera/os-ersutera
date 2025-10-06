#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// Use rtcgettime() to get high-resolution nanosecond timestamps
uint64 get_time() {
    return rtcgettime();
}

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        fprintf(2, "Usage: %s program [args...]\n", argv[0]);
        exit(1);
    }

    // Build child argv array (NULL-terminated)
    char *child_argv[argc];
    for (int i = 1; i < argc; i++) {
        child_argv[i - 1] = argv[i];
    }
    child_argv[argc - 1] = 0;

    int pid = fork();
    if (pid == 0) {
        // Child: execute the target program
        exec(child_argv[0], child_argv);
        printf("exec failed\n");
        exit(1);
    }

    // Parent: start timer right before waiting
    uint64 start = get_time();

    int status, syscalls;
    wait2(&status, &syscalls);

    uint64 end = get_time();          // end timer
    uint64 elapsed_ms = (end - start);// / 1000000ULL;

    printf("------------------\n");
    printf("Benchmark Complete\n");
    printf("Time Elapsed: %lu ms\n", elapsed_ms);
    printf("System Calls: %d\n", syscalls);

    exit(0);
}

