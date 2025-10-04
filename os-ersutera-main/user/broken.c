/**
 * @file broken.c
 * @author mmalensek
 *
 * This program contains a series of buggy, broken, or strange C functions for
 * you to ponder. Your job is to analyze each function, fix whatever bugs the
 * functions might have, and then explain what went wrong. Sometimes the
 * compiler will give you a hint.
 *
 *  ____________
 * < Good luck! >
 *  ------------
 *      \   ^__^
 *       \  (oo)\_______
 *          (__)\       )\/\
 *              ||----w |
 *              ||     ||
 */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static int func_counter = 1;
#define FUNC_START() printf("\n\n%d.) %s\n", func_counter++, __func__);

#pragma GCC diagnostic ignored "-Waggressive-loop-optimizations"
#pragma GCC diagnostic ignored "-Wdangling-pointer"
#pragma GCC diagnostic ignored "-Wformat-overflow"

/**
 * This code example was taken from the book 'Mastering C Pointers,' one of the
 * not so good ways to learn pointers. It was trying to demonstrate how to print
 * 'BNGULAR'... with pointers...? Maybe?
 *
 * (1) Fix the problem.
 * (2) Explain what's wrong with this code:
 *   String literals are immutable, so don't try to write into them.
 *   'B' is more readable than the ASCII code 66.
 */
void
angular(void)
{
    FUNC_START();
    char a[] = "ANGULAR";
    a[0] = 'B';
    printf("%s\n", a);
}

/**
 * This function is the next step after 'Hello world' -- it takes user input and
 * prints it back out! (Wow).
 *
 * But, unfortunately, it doesn't work.
 *
 * (1) Fix the problem.
 * (2) Explain what's wrong with this code:
 *  char *name = 0 points at address 0 (NULL). Writing to it causes  segfault.
 */
void
greeter(void)
{
    FUNC_START();
    
    char name[128];

    //char *ptr = name;
      
    printf("Please enter your name: ");

    gets(name, 127);
  
    // Remove newline
    char *p = strchr(name, '\n');
    for( ; *p != '\n' && *p != 0; p++) {}
    if (p) *p = '\0';
  
    printf("Hello, %s!\n", name);
}

/**
 * This code isn't so much broken, but more of an exercise in understanding how
 * C data types are represented in memory.
 *
 * (1) Fill in your guesses below to see if you can get all 6 correct on the
 *     first try.
 * (2) Explain *WHY* you get the sizes you do for each of the 6 entries.
 *    sizeof(char) == 1 (guaranteed by C standard).
 *    sizeof(int) == 4 (typical).
 *    sizeof(float) == 4.
 *    sizeof(things) → array of 12 ints = 12 * 4 = 48.
 *    sizeof('A') → 'A' is an int literal, so usually 4.
 *    sizeof("A") → "A" is a 2-char array ('A' + '\0'), so size = 2.
 *
 */
#define SIZE_CHECK(sz, guess) (sz), sz == guess ? "Right!" : "Wrong!"
void
sizer(void)
{
  FUNC_START();

  int guesses[] = { 1, 4, 4, 48, 4, 2 };
  if (sizeof(guesses) / sizeof(int) != 6) {
    printf("Please enter a guess for each of the sizes below.\n");
    printf("Example: guesses[] = { 1, 4, 0, 0, 0, 0 }\n");
    printf("would mean sizeof(char) == 1, sizeof(int) == 4, and so on");
    return;
  }

  int things[12] = { 0 };
  printf("sizeof(char)   = %ld\t[%s]\n", SIZE_CHECK(sizeof(char), guesses[0]));
  printf("sizeof(int)    = %ld\t[%s]\n", SIZE_CHECK(sizeof(int), guesses[1]));
  printf("sizeof(float)  = %ld\t[%s]\n", SIZE_CHECK(sizeof(float), guesses[2]));
  printf("sizeof(things) = %ld\t[%s]\n", SIZE_CHECK(sizeof(things), guesses[3]));
  printf("sizeof('A')    = %ld\t[%s]\n", SIZE_CHECK(sizeof('A'), guesses[4]));
  printf("sizeof(\"A\")    = %ld\t[%s]\n", SIZE_CHECK(sizeof("A"), guesses[5]));
}

/**
 * This 'useful' function prints out an array of integers with their indexes, or
 * at least tries to. It even has a special feature where it adds '12' to the
 * array.
 *
 * (1) Fix the problem.
 * (2) Explain what's wrong with this code:
 *
 *   14[stuff + 1] is unreadable. It actually means stuff[15].
 *   Looping to 1000 overruns the array (size 100).
 */
void
displayer(void)
{
  FUNC_START();

  int stuff[100] = { 0 };

  /* Can you guess what the following does without running the program? */
  /* Rewrite it so it's easier to read. */
  stuff[15] = 12;

  for (int i = 0; i < 100; ++i) {
    printf("%d: %d\n", i, stuff[i]);
  }
}
 
/**
 * Adds up the contents of an array and prints the total. Unfortunately the
 * total is wrong! See main() for the arguments that were passed in.
 *
 * (1) Fix the problem.
 * (2) Explain what's wrong with this code:
 *     Originally, sizeof(arr) gives the size of the pointer, not the array.
 */
void
adder(int *arr, int len)
{
  FUNC_START();

  int total = 0;

  for (int i = 0; i < len; i++) {
    total += arr[i];
  }

  printf("Total is: %d\n", total);
}

/**
 * This function is supposed to be somewhat like strcat, but it doesn't work.
 *
 * (1) Fix the problem.
 * (2) Explain what's wrong with this code:
 *  Stack memory invalid after function returns.
 * 
 */
char *
suffixer(const char *a, const char *b)
{
    char *buf = malloc(strlen(a) + strlen(b) + 1);
    if (!buf) return 0;
    strcpy(buf + strlen(buf), b);
    strcpy(buf + strlen(buf), a);
    return buf;   // caller must free
}
/**
 * This is an excerpt of Elon Musk's Full Self Driving code. Unfortunately, it
 * keeps telling people to take the wrong turn. Figure out how to fix it, and
 * explain what's going on so Elon can get back to work on leaving Earth for
 * good.
 *
 * (1) Fix the problem.
 * (2) Explain what's wrong with this code:
 *
 *   Overflow for street 4's array
 */
void
driver(void)
{
  FUNC_START();

  char street1[8] = { "fulton" };
  char street2[8] = { "gomery" };
  char street3[9] = { "piedmont" };
  char street4[8] = { "baker" };
  char street5[8] = { "haight" };

  if (strcmp(street1, street2) == 0) {
    char *new_name = "saint agnew ";
    memcpy(street4, new_name, strlen(new_name));
  }

  printf("Welcome to TeslaOS 0.1!\n");
  printf("Calculating route...\n");
  printf("Turn left at the intersection of %s and %s.\n", street5, street3);
}

/**
 * This function tokenizes a string by space, sort of like a basic strtok or
 * strsep. It has two subtle memory bugs for you to find.
 *
 * (1) Fix the problem.
 * (2) Explain what's wrong with this code:
 *
 *   Didn’t allocate space for terminating null.
 *   Freed a pointer that no longer points to start of allocation.
 */
void
tokenizer(void)
{
    FUNC_START();
    char *str = "Hope was a letter I never could send";
    char *line = malloc(strlen(str) + 1);
    if (!line) return;
    strcpy(line, str);

    char *copy = line;   // keep original malloc address
    char *c = line;
    while (*c != '\0') {
        for ( ; *c != ' ' && *c != '\0'; c++) { }

        if (*c == ' ') {
            *c = '\0';
            printf("%s\n", line);
            line = c + 1;
            c = line;
        } else { 
            // *c == '\0' -> last word
            printf("%s\n", line);
            break;
        }
    }
    
    free(copy);
}

/**
* This function should print one thing you like about C, and one thing you
* dislike about it. Assuming you don't mess up, there will be no bugs to find
* here!
*/
void
finisher(void)
{
  FUNC_START();

  printf("Easier to read than assembly.\n");
  printf("I dislike memory allocation.\n");
}

int
main(void)
{
  printf("Starting up!");


  angular();

  greeter();

  sizer();

  displayer();

  int nums[] = { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512 };
  adder(nums, sizeof(nums) / sizeof(nums[0]));

  char *result = suffixer("kayak", "ing");
  printf("Suffixed: %s\n", result);

  driver();

  tokenizer();

  finisher();

  return 0;
}
