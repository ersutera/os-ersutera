#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

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

  while (1) {
    printf("🐚 command> ");

    char buf[128];
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

    int pid = fork();
    if (pid == -1) {
      fprintf(2, "Fork failed");
      continue;
    } else if (pid == 0) {

      printf("we are about to run: %s\n", args[0]);
      // child
      // execute the command
      exec(args[0], args);
    } else {
      wait(0);
    }
  }

  return 0;
}
