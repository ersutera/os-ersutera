#include "defs.h"
#include "syscall.h"

// declarations for strace helpers if needed
#pragma GCC diagnostic ignored "-Wunused-function"

static uint64 read_int(int n) {
    int i;
    argint(n, &i);
    return i;
}

static void *read_ptr(int n) {
    uint64 p;
    argaddr(n, &p);
    return (void*)p;
}

static inline char *
read_str(struct proc *p, int n)
{
  static char buf[128];   // static so printf sees it
  uint64 uaddr;

  // get the raw user address of the nth arg
  argaddr(n, &uaddr);

  // copy from *that process’s* pagetable
  if (copyinstr(p->pagetable, buf, uaddr, sizeof(buf)) < 0) {
    buf[0] = '\0';
  }

  return buf;
}

void
strace(struct proc *p, int syscall_num, uint64 ret_val)
{
  switch(syscall_num){
    case SYS_read:
      printf("[%d|%s] read(fd=%d, buf=%p, count=%d) = %d\n",
         p->pid, p->name,
         (int)read_int(0), read_ptr(1), (int)read_int(2), (int)ret_val);
      break;
    case SYS_write:
      printf("[%d|%s] write(fd=%d, buf=%p, count=%d) = %d\n",
         p->pid, p->name,
         (int)read_int(0), read_ptr(1), (int)read_int(2), (int)ret_val);
      break;
    case SYS_open:
     printf("[%d|%s] open(pathname=\"%s\", flags=%d) = %d\n",
        p->pid, p->name, read_str(p,0), (int)read_int(1), (int)ret_val);
     break;
    default:
      printf("[%d|%s] syscall %d returned %d\n",
         p->pid, p->name, syscall_num, (int)ret_val);
      break;
  }
}
