#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "kernel/riscv.h"
#include "kernel/vm.h"
#include "user/user.h"

#define NULL 0

//
// wrapper so that it's OK if main() does not call exit().
//
void
start()
{
  extern int main();
  main();
  exit(0);
}

char*
strcpy(char *s, const char *t)
{
  char *os;

  os = s;
  while((*s++ = *t++) != 0)
    ;
  return os;
}

int
strcmp(const char *p, const char *q)
{
  while(*p && *p == *q)
    p++, q++;
  return (uchar)*p - (uchar)*q;
}

uint
strlen(const char *s)
{
  int n;

  for(n = 0; s[n]; n++)
    ;
  return n;
}

void*
memset(void *dst, int c, uint n)
{
  char *cdst = (char *) dst;
  int i;
  for(i = 0; i < n; i++){
    cdst[i] = c;
  }
  return dst;
}

char*
strchr(const char *s, char c)
{
  for(; *s; s++)
    if(*s == c)
      return (char*)s;
  return 0;
}

char*
gets(char *buf, int max)
{
  int i, cc;
  char c;

  for(i=0; i+1 < max; ){
    cc = read(0, &c, 1);
    if(cc < 1)
      break;
    buf[i++] = c;
    if(c == '\n' || c == '\r')
      break;
  }
  buf[i] = '\0';
  return buf;
}

int
stat(const char *n, struct stat *st)
{
  int fd;
  int r;

  fd = open(n, O_RDONLY);
  if(fd < 0)
    return -1;
  r = fstat(fd, st);
  close(fd);
  return r;
}

int
atoi(const char *s)
{
  int n;

  n = 0;
  while('0' <= *s && *s <= '9')
    n = n*10 + *s++ - '0';
  return n;
}

void*
memmove(void *vdst, const void *vsrc, int n)
{
  char *dst;
  const char *src;

  dst = vdst;
  src = vsrc;
  if (src > dst) {
    while(n-- > 0)
      *dst++ = *src++;
  } else {
    dst += n;
    src += n;
    while(n-- > 0)
      *--dst = *--src;
  }
  return vdst;
}

int
memcmp(const void *s1, const void *s2, uint n)
{
  const char *p1 = s1, *p2 = s2;
  while (n-- > 0) {
    if (*p1 != *p2) {
      return *p1 - *p2;
    }
    p1++;
    p2++;
  }
  return 0;
}

void *
memcpy(void *dst, const void *src, uint n)
{
  return memmove(dst, src, n);
}

char *
sbrk(int n) {
  return sys_sbrk(n, SBRK_EAGER);
}

char *
sbrklazy(int n) {
  return sys_sbrk(n, SBRK_LAZY);
}

// Reads contents of given file ('fd') to 'buf' of size 'max'.
// 
// Returns character's read, excluding null terminator.
int
fgets(char *buf, unsigned int max, int fd)
{
  int i, cc;
  char c;

  for(i=0; i+1 < max; ){
    cc = read(fd, &c, 1);
    if(cc < 1) {
      return -1;
    }
    buf[i++] = c;
    if(c == '\n' || c == '\r'){
      break;
    }
  }

  buf[i] = '\0';
  return i;
}

// Like fgets, but dynamically allocates correct memory
// to store the string.
//
// Caller provides pointer to a malloced 'buf' and capacity ('size').
// 'buf' points to NULL, a new buffer is allocated. 
// Returns characters read, excluding null terminator.
int
getline(char **buf, int size, int fd)
{
  int chars_read = 0;
  int curr_read;

  // initilize buffer if not already initilized  (size = 2, malloc space for a char + /0)
  if (*buf == NULL || size < 2) {		
    free(*buf);	// in case buf was 1 (if null, does nothing)
    *buf = malloc(2);
    size = 2;
  }

  while ((curr_read = fgets(*buf + chars_read, size - chars_read, fd))) {

    if (curr_read < 0) {  // error in fgets
      return - 1;
    }

    chars_read += curr_read;

    //resize 
    char *temp = malloc(size * 2);
    memcpy(temp, *buf, chars_read + 1); // include null term
    free(*buf);
    *buf = temp;
    size = size * 2;

    // break if end of line/eof
    char *last_char = *buf + (chars_read - 1);
    if (strcmp(last_char, "\n") == 0 || strcmp(last_char, "\r") == 0) {
      break;
    }
  }

  return chars_read;
}
