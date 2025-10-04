#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "syscall.h"
#include "defs.h"
#include "strace.h"

// Fetch the uint64 at addr from the current process.
int
fetchaddr(uint64 addr, uint64 *ip)
{
  struct proc *p = myproc();
  if(addr >= p->sz || addr+sizeof(uint64) > p->sz) // both tests needed, in case of overflow
    return -1;
  if(copyin(p->pagetable, (char *)ip, addr, sizeof(*ip)) != 0)
    return -1;
  return 0;
}

// Fetch the nul-terminated string at addr from the current process.
// Returns length of string, not including nul, or -1 for error.
int
fetchstr(uint64 addr, char *buf, int max)
{
  struct proc *p = myproc();
  if(copyinstr(p->pagetable, buf, addr, max) < 0)
    return -1;
  return strlen(buf);
}

static uint64
argraw(int n)
{
  struct proc *p = myproc();
  switch (n) {
  case 0:
    return p->trapframe->a0;
  case 1:
    return p->trapframe->a1;
  case 2:
    return p->trapframe->a2;
  case 3:
    return p->trapframe->a3;
  case 4:
    return p->trapframe->a4;
  case 5:
    return p->trapframe->a5;
  }
  panic("argraw");
  return -1;
}

// Fetch the nth 32-bit system call argument.
void
argint(int n, int *ip)
{
  *ip = argraw(n);
}

// Retrieve an argument as a pointer.
// Doesn't check for legality, since
// copyin/copyout will do that.
void
argaddr(int n, uint64 *ip)
{
  *ip = argraw(n);
}

// Fetch the nth word-sized system call argument as a null-terminated string.
// Copies into buf, at most max.
// Returns string length if OK (including nul), -1 if error.
int
argstr(int n, char *buf, int max)
{
  uint64 addr;
  argaddr(n, &addr);
  return fetchstr(addr, buf, max);
}

// Prototypes for the functions that handle system calls.
extern uint64 sys_fork(void);
extern uint64 sys_exit(void);
extern uint64 sys_wait(void);
extern uint64 sys_pipe(void);
extern uint64 sys_read(void);
extern uint64 sys_kill(void);
extern uint64 sys_exec(void);
extern uint64 sys_fstat(void);
extern uint64 sys_chdir(void);
extern uint64 sys_dup(void);
extern uint64 sys_getpid(void);
extern uint64 sys_sbrk(void);
extern uint64 sys_pause(void);
extern uint64 sys_uptime(void);
extern uint64 sys_open(void);
extern uint64 sys_write(void);
extern uint64 sys_mknod(void);
extern uint64 sys_unlink(void);
extern uint64 sys_link(void);
extern uint64 sys_mkdir(void);
extern uint64 sys_close(void);
extern uint64 sys_shutdown(void);
extern uint64 sys_reboot(void);
extern uint64 sys_rtcgettime(void);
extern uint64 sys_strace_on(void);


// An array mapping syscall numbers from syscall.h
// to the function that handles the system call.
static uint64 (*syscalls[])(void) = {
[SYS_fork]    sys_fork,
[SYS_exit]    sys_exit,
[SYS_wait]    sys_wait,
[SYS_pipe]    sys_pipe,
[SYS_read]    sys_read,
[SYS_kill]    sys_kill,
[SYS_exec]    sys_exec,
[SYS_fstat]   sys_fstat,
[SYS_chdir]   sys_chdir,
[SYS_dup]     sys_dup,
[SYS_getpid]  sys_getpid,
[SYS_sbrk]    sys_sbrk,
[SYS_pause]   sys_pause,
[SYS_uptime]  sys_uptime,
[SYS_open]    sys_open,
[SYS_write]   sys_write,
[SYS_mknod]   sys_mknod,
[SYS_unlink]  sys_unlink,
[SYS_link]    sys_link,
[SYS_mkdir]   sys_mkdir,
[SYS_close]   sys_close,
[SYS_shutdown] sys_shutdown,
[SYS_reboot]   sys_reboot,
[SYS_rtcgettime] sys_rtcgettime,
[SYS_strace_on] sys_strace_on,
};

// Helper to save syscall arguments before they get overwritten
struct syscall_args {
    uint64 a0, a1, a2, a3, a4, a5;
};

void save_syscall_args(struct proc *p, struct syscall_args *args) {
    if (!p || !p->trapframe || !args) return;
    args->a0 = p->trapframe->a0;
    args->a1 = p->trapframe->a1;
    args->a2 = p->trapframe->a2;
    args->a3 = p->trapframe->a3;
    args->a4 = p->trapframe->a4;
    args->a5 = p->trapframe->a5;
}

void
syscall(void)
{
    struct proc *p = myproc();
    int num;
    num = p->trapframe->a7;  // syscall number (x86/x64 or RISCV variant)
    if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
        int retval = syscalls[num]();  // call handler: it returns int (or uses p->trapframe->a0)
        // If your syscall handlers return their value in p->trapframe->a0, you may need:
        // int retval = p->trapframe->a0; // or capture appropriately.

        // call strace if enabled
        if(p && p->traced) {
            strace(p, num, retval);
        }

        // write return value back to trapframe (if not already)
        p->trapframe->a0 = retval;
    } else {
        printf("%d %s: unknown sys call %d\n",
        p->pid, p->name, num);
        p->trapframe->a0 = -1;
    }
    
    uint64 retval = p->trapframe->a0;
    if(p->tracing){
        strace(p, num, retval);
    }
}
