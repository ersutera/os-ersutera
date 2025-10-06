#include "kernel/fcntl.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define BUF_SIZE 512

// Optimized fgets using internal buffer
int
fgets1(char *buf, int max, int fd)
{
  static char internal[BUF_SIZE];
  static int pos = 0, len = 0;
  int i = 0;

  while (i + 1 < max) {
    if (pos >= len) {
      len = read(fd, internal, BUF_SIZE);
      if (len <= 0)
        break;
      pos = 0;
    }
    buf[i++] = internal[pos++];
    if (buf[i-1] == '\n' || buf[i-1] == '\r')
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
    exit(1);
  }

  int fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    fprintf(2, "catlines: cannot open %s\n", argv[1]);
    exit(1);
  }

  char buf[128];
  int line_count = 0;
  while (fgets1(buf, sizeof(buf), fd) > 0) {
    printf("Line %d: %s", line_count++, buf);
  }

  close(fd);
  exit(0);
}
