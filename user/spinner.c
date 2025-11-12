#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  if (argc <= 2) {
    fprintf(2, "Usage: spinner name amount\n");
    exit(1);
  }

  char *name = argv[1];
  int amount = atoi(argv[2]);

  int start = uptime();
  for (int i = 0; i < amount; ++i) {
    printf("%s", name);
    for (int j = 0; j < 10000000; ++j) {
      // wheee :-)
    }
  }
  int end = uptime();

  printf("\n[%s] has finished, %d work in %d ticks.\n",
         name, amount, end - start);

  exit(0);
}
