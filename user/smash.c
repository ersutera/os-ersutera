#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"


#define COLOR_RESET  "\033[0m"
#define COLOR_GREEN  "\033[32m"
#define COLOR_CYAN   "\033[36m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_RED    "\033[31m"

#define MAX_ALIASES 32

struct alias {
    char name[32];
    char value[128];
};

    struct alias aliases[MAX_ALIASES];
    int alias_count = 0;

uint
strspn(const char *str, const char *chars)
{
  uint i, j;
  for (i = 0; str[i] != '\0'; i++) {
    for (j = 0; chars[j] != str[i]; j++) {
      if (chars[j] == '\0')
        return i;
    }
  }
  return i;
}

uint
strcspn(const char *str, const char *chars)
{
  const char *p, *sp;
  char c, sc;
  for (p = str;;) {
    c = *p++;
    sp = chars;
    do {
      if ((sc = *sp++) == c) {
        return (p - 1 - str);
      }
    } while (sc != 0);
  }
}

char
*next_token(char **str_ptr, const char *delim)
{
  if (*str_ptr == 0) {
    return 0;
  }

  uint tok_start = strspn(*str_ptr, delim);
  uint tok_end = strcspn(*str_ptr + tok_start, delim);

  /* Zero length token. We must be finished. */
  if (tok_end  == 0) {
    *str_ptr = 0;
    return 0;
  }

  /* Take note of the start of the current token. We'll return it later. */
  char *current_ptr = *str_ptr + tok_start;

  /* Shift pointer forward (to the end of the current token) */
  *str_ptr += tok_start + tok_end;

  if (**str_ptr == '\0') {
    /* If the end of the current token is also the end of the string, we
         * must be at the last token. */
    *str_ptr = 0;
  } else {
    /* Replace the matching delimiter with a NUL character to terminate the
         * token string. */
    **str_ptr = '\0';

    /* Shift forward one character over the newly-placed NUL so that
         * next_pointer now points at the first character of the next token. */
    (*str_ptr)++;
  }

  return current_ptr;
}

int
main(int argc, char *argv[])
{

  // -1. print a prompt
  //  0. get the user's command (stdin)
  //  1. fork
  //  2. exec
  //


  int cmd_num = 1;
  int last_status = 0;
  char cwd[128];

  while (1) {
    getcwd(cwd, sizeof(cwd));  // syscall you'll need to add in kernel
    printf(COLOR_CYAN "[%d]" COLOR_RESET
           COLOR_YELLOW "-[%d]" COLOR_RESET
           "─" COLOR_GREEN "[%s]" COLOR_RESET
           "$ ", last_status, cmd_num, cwd);
                         

    char buf[128];
    if (gets(buf, sizeof(buf)) == 0) {break;}
    printf("🐚 command> ");

    gets(buf, 128);

    printf("Your command is: %s", buf);
    printf("Nice job!\n");

    // echo hello
    // -> [echo, hello]
    // NOTE: this is destructive and modifies 'buf'
    char *args[10];
    int tokens = 0;
    char *next_tok = buf;
    char *curr_tok;
    /* Tokenize. */
    while ((curr_tok = next_token(&next_tok, " \n\t")) != 0) {
      printf("Token: '%s'\n", curr_tok);
      args[tokens++] = curr_tok;
    }
    args[tokens] = 0;

    //Built-ins:  cd, exit, history, #comments
    if (args[0] == 0)
      continue;
    if (args[0][0] == '#')
      continue; // comment line
    
    if (strcmp(args[0], "exit") == 0)
      exit(0);
    
    if (strcmp(args[0], "cd") == 0) {
      if (args[1] == 0 || chdir(args[1]) < 0)
        fprintf(2, "chdir: no such file or directory: %s\n", args[1]);
      continue;
    }
    
    if (strcmp(args[0], "alias") == 0) {
      if (args[1] == 0) {
        // list all aliases
        for (int i = 0; i < alias_count; i++)
          printf("%s='%s'\n", aliases[i].name, aliases[i].value);
      } else {
        // parse name=value
        char *eq = strchr(args[1], '=');
        if (eq == 0) {
          fprintf(2, "usage: alias name=\"value\"\n");
          continue;
        }
        *eq = '\0';
        char *name = args[1];
        char *value = eq + 1;
        if (*value == '"' || *value == '\'') value++;  // strip quote
        if (value[strlen(value)-1] == '"' || value[strlen(value)-1] == '\'')
          value[strlen(value)-1] = '\0';

        // store or update alias
        int found = 0;
        for (int i = 0; i < alias_count; i++) {
          if (strcmp(aliases[i].name, name) == 0) {
            safestrcpy(aliases[i].value, value, sizeof(aliases[i].value));
            found = 1;
            break;
          }
        }
        if (!found && alias_count < MAX_ALIASES) {
          safestrcpy(aliases[alias_count].name, name, sizeof(aliases[alias_count].name));
          safestrcpy(aliases[alias_count].value, value, sizeof(aliases[alias_count].value));
          alias_count++;
        }
      }
      continue;
    }

    for (int i = 0; args[i]; i++) {
      if (strcmp(args[i], ">") == 0) {
        close(1);
        open(args[i+1], O_WRONLY | O_CREATE | O_TRUNC);
        args[i] = 0;
      } else if (strcmp(args[i], ">>") == 0) {
        close(1);
        open(args[i+1], O_WRONLY | O_CREATE | O_APPEND);
        args[i] = 0;
      } else if (strcmp(args[i], "<") == 0) {
        close(0);
        open(args[i+1], O_RDONLY);
        args[i] = 0;
      }
    }

    int background = 0;
    if (tokens > 0 && strcmp(args[tokens-1], "&") == 0) {
        background = 1;
        args[tokens-1] = 0;
    }

    int pid = fork();
    if (pid == -1) {
      fprintf(2, "Fork failed");
      continue;
    } if (pid == 0) {
        exec(args[0], args);
        fprintf(2, "exec %s failed\n", args[0]);
        exit(1);
    } else if (!background) {
        wait(&last_status);
    } else {
      wait(0);
    }
  }

  return 0;
}
