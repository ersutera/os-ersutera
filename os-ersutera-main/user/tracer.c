// user/tracer.c
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  if (argc < 2) {
    printf("usage: tracer program [args...]\n");
    exit(1);
  }

  int pid = fork();
  if (pid < 0) {
    printf("tracer: fork failed\n");
    exit(1);
  }

  if (pid == 0) {
    // child: enable tracing then exec
    if (strace_on() < 0) {
      printf("tracer: strace_on failed\n");
      exit(1);
    }
    exec(argv[1], &argv[1]);
    printf("tracer: exec failed\n");
    exit(1);
  } else {
    // parent: wait
    wait(0);
    exit(0);
  }
}
