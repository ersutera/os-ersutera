#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

int main(void) {
    char *p1 = (char*)mmap(0, 4096, 0, 0, -1, 0);
    char *p2 = (char*)mmap(0, 4096, 0, 0, -1, 0);
    char *p3 = (char*)mmap(0, 4096, 0, 0, -1, 0);

    strcpy(p1, "hello world!");
    strcpy(p2, "page two");
    strcpy(p3, "page three");

    printf("-> %s\n", p1);
    printf("-> %s\n", p2);
    printf("-> %s\n", p3);

    exit(0);

}
