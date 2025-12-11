#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

void print_result(int cond, char *msg) {
    if (cond) printf("PASS: %s\n", msg);
    else      printf("FAIL: %s\n", msg);
}

int main() {

    printf("\n===== BASIC MALLOC/FREE TESTS =====\n");
    void *a = malloc(32);
    print_result(a != 0, "malloc(32) returns non-null");

    ((char*)a)[0] = 'x';
    print_result(((char*)a)[0] == 'x', "malloc'd memory is writable");
    free(a);


    printf("\n===== CALLOC TEST =====\n");
    char *b = calloc(8, 8);   // 64 bytes
    print_result(b != 0, "calloc returns non-null");

    int zeroed = 1;
    for (int i = 0; i < 64; i++)
        if (b[i] != 0) zeroed = 0;
    print_result(zeroed, "calloc returns zeroed memory");
    free(b);


    printf("\n===== FIRST FIT + REUSE TEST =====\n");
    malloc_setfsm(0);  // FIRST FIT

    void *c1 = malloc(100);
    void *c2 = malloc(100);
    free(c1);

    void *c3 = malloc(50);
    print_result(c3 == c1, "Freed block reused (first fit)");
    free(c2);
    free(c3);


    printf("\n===== SPLITTING TEST =====\n");
    void *s1 = malloc(200);
    void *s2 = malloc(50);
    print_result(s1 && s2, "malloc returned valid pointers");
    free(s1);
    free(s2);


    printf("\n===== BEST/WORST FIT TEST =====\n");

    void *x1 = malloc(100);
    void *x2 = malloc(200);
    void *x3 = malloc(300);

    free(x1);
    free(x2);
    free(x3);

    malloc_setfsm(1); // BEST FIT
    void *bfit = malloc(90);
    print_result(bfit == x1, "best-fit selected smallest block");
    free(bfit);

    malloc_setfsm(2); // WORST FIT
    void *wfit = malloc(90);
    print_result(wfit == x3, "worst-fit selected largest block");
    free(wfit);


    printf("\n===== COALESCING TEST =====\n");
    void *m1 = malloc(100);
    void *m2 = malloc(100);
    void *m3 = malloc(100);

    free(m1);
    free(m2);
    free(m3);

    void *m_big = malloc(250);
    print_result(m_big == m1, "coalescing merged neighbors");
    free(m_big);


    printf("\n===== REALLOC TEST =====\n");
    char *r = malloc(20);
    strcpy(r, "hello");

    r = realloc(r, 100); // grow
    print_result(strcmp(r, "hello") == 0, "realloc grow preserves data");

    r = realloc(r, 10);  // shrink
    print_result(strcmp(r, "hello") == 0, "realloc shrink preserves data");
    free(r);


    printf("\n===== SCRIBBLE TEST =====\n");
    malloc_scribble(1);

    char *s = malloc(64);
    int scrib_ok = 1;
    for (int i = 0; i < 64; i++)
        if (s[i] != 0xAA) scrib_ok = 0;
    print_result(scrib_ok, "malloc_scribble fills memory");
    free(s);

    malloc_scribble(0);


    printf("\n===== LEAK CHECK TEST =====\n");
    void *L1 = malloc(100);
    void *L2 = malloc(200);
    free(L1);

    printf("(malloc_leaks should report exactly 1 leak)\n");
    malloc_leaks();

    free(L2);


    printf("\n===== ALL TESTS COMPLETE =====\n");
    exit(0);
}

