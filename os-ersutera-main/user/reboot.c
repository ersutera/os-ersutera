#include "kernel/types.h"
#include "user/user.h"

int
main(void)
{
    reboot();
    return 0; // never returns
}
