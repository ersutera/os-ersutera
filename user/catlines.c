#include "kernel/fcntl.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
fgets(char *buf, int max, int fd)
{
  int i, cc;
  char c;

  for (i = 0; i + 1 < max; ) {
    cc = read(fd, &c, 1);
    if(cc < 1)
      break;
    buf[i++] = c;
    if(c == '\n' || c == '\r')
      break;
  }
  buf[i] = '\0';
  return i;
}

int
main(int argc, char *argv[])
{
  if (argc <= 1) {
    fprintf(2, "Usage: %s filename\n", argv[0]);
    return 1;
  }

  int fd = open(argv[1], O_RDONLY);
  char buf[128];
  int line_count = 0;
  while (fgets(buf, 128, fd) > 0 ) {
    printf("Line %d: %s", line_count++, buf);
  }

  return 0;
}
