#define SBRK_ERROR ((char *)-1)

struct stat;

// system calls
int fork(void);
int exit(int) __attribute__((noreturn));
int wait(int*);
int pipe(int*);
int write(int fd, const void* buf, int count);
int read(int fd, void* buf, int count);
int close(int fd);
int kill(int pid);
int exec(const char* pathname, char** argv);
int open(const char* pathname, int mode);
int mknod(const char* pathname, short major, short minor);
int unlink(const char* pathname);
int fstat(int fd, struct stat* st);
int link(const char* oldpath, const char* newpath);
int mkdir(const char* pathname);
int chdir(const char* path);
int dup(int fd);
int getpid(void);
char* sys_sbrk(int increment, int type);
int pause(int ticks);
int uptime(void);
int shutdown(void);
int reboot(void);
uint64 rtcgettime(void);
int strace_on(void);
int wait2(int *status, int *syscall_count);

// ulib.c
int stat(const char*, struct stat*);
char* strcpy(char*, const char*);
void *memmove(void*, const void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
char* gets(char*, int max);
uint strlen(const char*);
void* memset(void*, int, uint);
int atoi(const char*);
int memcmp(const void *, const void *, uint);
void *memcpy(void *, const void *, uint);
char* sbrk(int);
char* sbrklazy(int);

// printf.c
void fprintf(int, const char*, ...) __attribute__ ((format (printf, 2, 3)));
void printf(const char*, ...) __attribute__ ((format (printf, 1, 2)));

// umalloc.c
void* malloc(uint);
void free(void*);
