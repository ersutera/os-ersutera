#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
    uint64 t = rtcgettime();
    uint64 secs = t / 1000000000ULL;  // nanoseconds → seconds

    printf("UNIX timestamp: %lu seconds\n", secs);
    exit(0);

}
