#include "kernel/types.h"
#include "user/user.h"

int main() {
  for (int c = 0; read(0, &c, 1) > 0; ) {
    if (c >= 'A' && c <= 'Z') {
      c = c - 'A' + 'a';
    }
    write(1, &c, 1);
  }
  return 0;
}
