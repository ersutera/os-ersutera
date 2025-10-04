# System Call Tracing Implementation

This document describes the implementation of system call tracing in FogOS.

## Overview

The system call tracing functionality allows developers to monitor and debug system calls made by processes. When enabled for a process, it logs detailed information about each system call including:

- System call name
- Argument names and values  
- Return values
- Process ID and name

## Components Implemented

### 1. Process Tracing State (`kernel/proc.h`)
- `traced` field in `struct proc` to indicate if tracing is enabled for a process
- Automatically inherited by child processes during `fork()`
- Cleared when processes are freed in `freeproc()`

### 2. System Call Interface (`kernel/sysproc.c`)
- `sys_strace_on()` system call to enable tracing for the current process
- Accessible from user space via `strace_on()` function

### 3. Core Tracing Logic (`kernel/syscall.c`)
- Modified `syscall()` function to intercept all system calls
- Saves syscall arguments before handler execution to prevent corruption by return values
- Calls `strace()` function when tracing is enabled for a process

### 4. Auto-Generated Tracing Code (`generate-traces.sh` → `kernel/strace.h`)
- Script that automatically generates tracing code for all system calls
- Extracts syscall signatures from `user/user.h` and `user/usys.pl`
- Generates appropriate printf statements with proper argument formatting
- Includes helper functions for reading different argument types

### 5. Argument Reading Functions (`kernel/strace.h`)
- `read_int(n)` - reads integer arguments from trapframe
- `read_ptr(n)` - reads pointer arguments  
- `read_str(n)` - safely copies string arguments from user space using `copyinstr()`
- Handles failed string copies by displaying hex addresses

### 6. User Space Tracer Program (`user/tracer.c`)
- Forks a child process
- Enables tracing on the child
- Executes the target program with tracing enabled
- Waits for child completion

## Usage

To trace a program:
```bash
/tracer /program [args...]
```

Example output:
```
$ /tracer /cat README.md
[4|tracer] strace_on() = 0
[4|cat] exec(pathname = 0x0000000000003fe0, argv = 0x0000000000003fc0) = 2
[4|cat] open(pathname = README.md, mode = 0) = 3
[4|cat] read(fd = 3, buf = 0x0000000000001010, count = 512) = 75
[4|cat] write(fd = 1, buf = 0x0000000000001010, count = 75) = 75
[4|cat] read(fd = 3, buf = 0x0000000000001010, count = 512) = 0
[4|cat] close(fd = 3) = 0
```

## Build Integration

The tracing code is automatically generated during the build process:

1. `generate-traces.sh` parses system call definitions
2. Generates `kernel/strace.h` with complete tracing implementation
3. `kernel/syscall.c` includes the generated header
4. Makefile rule ensures regeneration when system calls change

## Implementation Details

### Argument Preservation
System calls modify `trapframe->a0` to return values, potentially corrupting the first argument. The implementation saves all arguments before calling the syscall handler and restores them temporarily during tracing.

### String Handling
String arguments are safely copied from user space using `copyinstr()`. If copying fails (e.g., during `exec` when memory layout is changing), the implementation displays the raw pointer address in hexadecimal format.

### Process Inheritance
Child processes automatically inherit the tracing state from their parent during `fork()`, enabling seamless tracing of program execution including spawned subprocesses.

### Auto-Generation Benefits
- Automatically handles new system calls when added
- Consistent formatting across all system calls
- Reduces manual maintenance burden
- Extracts argument names directly from function signatures

## Files Modified

- `kernel/proc.h` - Already had tracing flags
- `kernel/proc.c` - Clear tracing flags in `freeproc()`, inherit in `fork()`
- `kernel/syscall.c` - Argument preservation and strace call integration
- `kernel/sysproc.c` - Already had `sys_strace_on()` implementation
- `generate-traces.sh` - Complete rewrite for code generation
- `user/user.h` - Improved argument names for better tracing output
- `user/tracer.c` - Already implemented correctly
- `Makefile` - Remove `strace.o`, keep auto-generation rule

The implementation provides a complete, robust system call tracing facility that matches the requirements and expected output format specified in the problem statement.