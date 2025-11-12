#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    if(argc < 3){
        fprintf(2, "Usage: nice <level> <command> [args...]\n");
        exit(1);
    }

    // Parse niceness level
    int level = atoi(argv[1]);
    if(level < 0) level = 0;
    if(level > 3) level = 3;

    // Set process niceness via syscall
    if(setnice(level) < 0){
        fprintf(2, "setnice failed\n");
        exit(1);
    }

    // Execute the command with arguments
    exec(argv[2], &argv[2]);

    // If exec returns, it failed
    fprintf(2, "exec %s failed\n", argv[2]);
    exit(1);
}
