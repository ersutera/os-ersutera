#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  char *line = malloc(512);
  while (getline(&line, 512, 0) > 0) {
    for (int i = 1; i < argc; i += 2) {
      char *find = argv[i];
      char *repl = argv[i + 1];
      uint find_len = strlen(find);
      if (strlen(line) < find_len) {
        continue;
      }
      for (int j = 0; j < strlen(line) - find_len; ++j) {
        char *p = line + j;
        char *q = find;
        int k;
        for (k = 0; *p && *p == *q && k < find_len; ++k) {
          p++, q++;
        }
        if (k == find_len) {
          memcpy(line + j, repl, find_len);
        }
      }
    }
    printf("%s", line);
  }
  return 0;
}
