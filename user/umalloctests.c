#include "user.h"
#include "types.h"

/* A simple helper for test output */
static void check(char *desc, int cond) {
    if (cond) {
        printf("[PASS] %s\n", desc);
    } else {
        printf("[FAIL] %s\n", desc);
    }
}

int main(void) {
    printf("=== XV6 MALLOC TEST PROGRAM ===\n");

    /* ---------------- 70%: Basic allocation ---------------- */
    char *a = malloc(32);
    char *b = malloc(64);
    char *c = calloc(4, 16);  // xv6's calloc uses our umalloc

    check("malloc returned non-NULL", a != 0 && b != 0);
    check("calloc returned zeroed memory", c != 0 && c[0] == 0 && c[63] == 0);

    malloc_print();  // print memory state

    free(a);
    free(c);

    char *reuse_test = malloc(16);
    check("Free reuses freed blocks (first fit)", reuse_test != 0);

    malloc_print();  // inspect free list

    /* ---------------- 80%: Splitting & FSM ---------------- */
    malloc_setfsm(1); // best fit
    char *d = malloc(16);
    char *e = malloc(16);
    malloc_print();

    malloc_setfsm(2); // worst fit
    char *f = malloc(16);
    malloc_print();

    /* ---------------- 90%: Merging & realloc ---------------- */
    free(d);
    free(e);
    malloc_print(); // d+e should merge

    char *g = malloc(64);
    memset(g, 'X', 64);

    g = realloc(g, 32); // shrink
    check("realloc shrink preserves data", g[0] == 'X' && g[31] == 'X');

    char *h = malloc(16);
    h[0] = 'H';
    h[1] = 'i';
    char *h2 = realloc(h, 64); // grow
    check("realloc grow preserves data", h2[0] == 'H' && h2[1] == 'i');

    /* ---------------- 95%: Scribbling & leak check ---------------- */
    malloc_scribble(1);
    char *i = malloc(16);
    check("scribble fills with 0xAA", (unsigned char)i[0] == 0xAA);

    malloc_leaks(); // show leaks

    free(b);
    free(f);
    free(g);
    free(h2);
    free(i);

    malloc_print(); // should be all free
    malloc_leaks();  // final leak check

    printf("=== MALLOC TEST COMPLETE ===\n");
    exit(0);
}

