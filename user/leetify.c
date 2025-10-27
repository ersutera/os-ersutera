#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define MAX_CMDS 8
#define MAX_ARGS 16

// Spawn a program (argv is NULL-terminated array), using fd_in as stdin and fd_out as stdout.
// If fd_in == 0 or fd_out == 1, those fds are left as-is (no dup/close necessary).
// Returns pid on success, -1 on fork/exec failure (prints error).
int spawn(char *prog, char **argv, int fd_in, int fd_out) {
  int pid = fork();
  if (pid < 0) {
    fprintf(2, "leetify: fork failed\n");
    return -1;
  }
  if (pid == 0) {
    // child
    // set up stdin
    if (fd_in != 0) {
      // close stdin then dup fd_in into 0
      close(0);
      if (dup(fd_in) != 0) {
        // dup should return lowest free fd (we expect 0)
        // if not, try to duplicate until we get 0 (rare)
      }
      // close original if it's not 0 (dup created fd 0)
      if (fd_in > 2) close(fd_in);
    }

    // set up stdout
    if (fd_out != 1) {
      close(1);
      if (dup(fd_out) != 1) {
        // same note as above
      }
      if (fd_out > 2) close(fd_out);
    }

    // exec
    exec(prog, argv);
    // if exec returns, it's an error
    fprintf(2, "exec %s failed\n", prog);
    exit(1);
  }
  // parent
  return pid;
}

int
main(int argc, char *argv[])
{
  if (argc < 2) {
    fprintf(2, "usage: leetify input-file [output-file]\n");
    exit(1);
  }

  char *infile = argv[1];
  char *outfile = 0;
  if (argc >= 3) outfile = argv[2];

  // Build command argument arrays
  // cat infile
  char *cat_argv[3];
  cat_argv[0] = "cat";
  cat_argv[1] = infile;
  cat_argv[2] = 0;

  // tolower
  char *tolower_argv[2];
  tolower_argv[0] = "tolower";
  tolower_argv[1] = 0;

  // fnr the teh a 4 e 3 i !
  char *fnr1_argv[] = { "fnr", "the", "teh", "a", "4", "e", "3", "i", "!", 0 };

  // fnr l 1 o 0 s 5
  char *fnr2_argv[] = { "fnr", "l", "1", "o", "0", "s", "5", 0 };

  // We'll create pipes between the commands.
  // pipeline: cat -> tolower -> fnr1 -> fnr2 -> output

  int p1[2], p2[2], p3[2];
  if (pipe(p1) < 0) { fprintf(2, "leetify: pipe failed\n"); exit(1); }
  if (pipe(p2) < 0) { fprintf(2, "leetify: pipe failed\n"); exit(1); }
  if (pipe(p3) < 0) { fprintf(2, "leetify: pipe failed\n"); exit(1); }

  // spawn cat: stdin default (0), stdout -> p1[1]
  int pid_cat = spawn("cat", cat_argv, 0, p1[1]);
  if (pid_cat < 0) exit(1);

  // parent can close write end after spawning child (or close both ends we don't need)
  close(p1[1]); // parent doesn't write to p1

  // spawn tolower: stdin <- p1[0], stdout -> p2[1]
  int pid_tol = spawn("tolower", tolower_argv, p1[0], p2[1]);
  if (pid_tol < 0) exit(1);
  // we can close p1[0] and p2[1] in parent
  close(p1[0]);
  close(p2[1]);

  // spawn fnr1: stdin <- p2[0], stdout -> p3[1]
  int pid_fnr1 = spawn("fnr", fnr1_argv, p2[0], p3[1]);
  if (pid_fnr1 < 0) exit(1);
  close(p2[0]);
  close(p3[1]);

  // Determine final stdout: either a file or stdout (1)
  int final_out_fd = 1;
  int fd_out;
  if (outfile) {
    fd_out = open(outfile, O_WRONLY | O_CREATE | O_TRUNC);
    if (fd_out < 0) {
      fprintf(2, "leetify: cannot open %s\n", outfile);
      // still wait for children to avoid zombies
      wait(0); wait(0); wait(0);
      exit(1);
    }
    final_out_fd = fd_out;
  }

  // spawn fnr2: stdin <- p3[0], stdout -> final_out_fd
  int pid_fnr2 = spawn("fnr", fnr2_argv, p3[0], final_out_fd);
  if (pid_fnr2 < 0) {
    if (outfile) close(fd_out);
    exit(1);
  }

  // parent closes read end and final fd if not stdout
  close(p3[0]);
  if (outfile) close(fd_out);

  // wait for all children
  wait(0); // cat
  wait(0); // tolower
  wait(0); // fnr1
  wait(0); // fnr2

  exit(0);
}
