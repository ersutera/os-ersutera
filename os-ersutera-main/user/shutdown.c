#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
    shutdown();
    return 0; // never returns
}
