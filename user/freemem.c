#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"   
#include "freemem.h" 

int main(void) {
    printf("%d KiB\n", freemem());
    return 0;
}
