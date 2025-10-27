# This script is designed to be run on a Linux host (i.e., not in xv6)
# You should probably run this script to see what it does first. To do that, you
# may need to make it executable:
#
# chmod +x generate-traces.sh
#
# And then run it from the *root* of your OS directory.
#
#
# For each user space system call stub, this script will print out information
# about its name, arguments, and return values.
#
# You can add information to the stubs that this script will extract. For
# example, instead of:
#
# int write(int, const void *, int);
#
# You can add argument names:
#
# int write(int fd, const void *buf, int count);
#
# And this script will extract them. By default, pointers are assumed to be
# addresses, but if you want the script to treat them like strings you can add a
# bit of metadata with a C comment:
#
# int mkdir(const char* /*str*/ path);
#
# Here's a few examples from a user/user.h file:
# int exit(int status) __attribute__((noreturn));
# int wait(int *status);
# int pipe(int *pipefd);
# int mkdir(const char* /*str*/ path);


# The following code extracts function information from user/user.h based on the
# system calls found in user/usys.pl.

# Retrieves the type of the argument provided as a printf format specifier
get_type() {
    case "${1}" in
    *'/*str*/'*) echo "%s" ;;
    *"*"*) echo "%p" ;;
    *) echo "%ld" ;; # default to int
    esac
}

# Determines which function to call for a given format specifier.
get_conversion() {
    case "${1}" in
        '%p') echo "read_ptr(${2})" ;;
        '%s') echo "read_str(p, ${2})" ;;
        '%ld') echo "read_int(${2})" ;;
    esac
}

# Inspects the user/user.h file to extract system call information
syscall_output=$(
for i in $(sed -n 's/entry("\(.*\)");/\1/p' user/usys.pl); do
    return_value=$(sed -nE \
        "s:\s*([A-Za-z_][A-Za-z0-9_\* ]+)\s+${i}.*:\1:p;"  user/user.h)
    args=$(sed 's/const//g; s/void//g; s/struct//g; s/((noreturn))//g' user/user.h \
        | sed -nE "s:.*\s${i}\((.*)\).*:\1:p")

    # Extract argument names and types for printing
    arg_list=""
    arg_convs=""
    arg_count=0
    export IFS=","
    for arg in $args; do
        arg_name=$(echo "${arg}" | sed -nE 's:.*\b([A-Za-z0-9_]+)\s*:\1:p')
        arg_type=$(get_type "${arg}")
        arg_conv=$(get_conversion "${arg_type}" "${arg_count}")
        arg_list="${arg_list}${arg_name} = ${arg_type}, "
        arg_convs="${arg_convs}${arg_conv}, "
        (( arg_count++ ))
    done

    # Remove extra ', '
    [[ "${arg_count}" -gt 0 ]] && arg_list="${arg_list::-2}"

    # cast the return type, if necessary
    return_type="$(get_type "${return_value}")"
    cast_ret=""
    if [[ "${return_type}" == "%p" ]]; then
        cast_ret="(void *)"
    fi

    # Print out the C code to display system call information. You will probably
    # need to modify this so it can be used to conditionally print out the
    # appropriate info.
    echo -n "SYS_${i} => "
    echo -n "printf(\"[%d|%s] ${i}(${arg_list}) = $(get_type "${return_value}")\n\","
    echo " p->pid, p->name, ${arg_convs} ${cast_ret} ret_val);"
done
)

# The information we want has been extracted and stored in $syscall_output.
# Using this information, we'll generate C code below. When we run 'cat <<EOM'
# it tells cat to output everything that follows until the next 'EOM' is found.

cat <<EOM
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
EOM
