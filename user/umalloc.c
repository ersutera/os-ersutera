#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

#define NULL 0
#define PAGE_SIZE 4096
#define MIN_BLOCK_SIZE 48
#define ALIGN16(x) (((x) + 15) & ~0xF)

enum fsm_algo { FIRST_FIT, BEST_FIT, WORST_FIT };
static enum fsm_algo current_fsm = FIRST_FIT;
static int scribble_enabled = 0;

struct mem_block {
    char name[8];
    uint size; // LSB = free flag
    struct mem_block *next_block;
    struct mem_block *prev_block;
};

static struct mem_block *block_head = NULL;
static struct mem_block *free_head = NULL;

/* ---------- tiny printing helpers using write(2) ---------- */
/* prints string s (must be NUL-terminated) to fd 2 (stderr) */
static void prstr(const char *s) {
    int len = 0;
    while (s[len]) len++;
    if (len > 0) write(2, s, len);
}

/* prints a single char to stderr */
static void prch(char c) {
    write(2, &c, 1);
}

/* print unsigned decimal */
static void pruint(uint x) {
    char buf[16];
    int i = 0;
    if (x == 0) {
        prch('0');
        return;
    }
    while (x > 0 && i < (int)sizeof(buf)) {
        buf[i++] = '0' + (x % 10);
        x /= 10;
    }
    while (i-- > 0) prch(buf[i]);
}

/* print pointer in hex (0x...) */
static void prptr(void *p) {
    unsigned long x = (unsigned long)p;
    char buf[32];
    int i = 0;
    prstr("0x");
    if (x == 0) {
        prch('0');
        return;
    }
    while (x > 0 && i < (int)sizeof(buf)) {
        int d = x & 0xF;
        buf[i++] = (d < 10) ? ('0' + d) : ('a' + (d - 10));
        x >>= 4;
    }
    while (i-- > 0) prch(buf[i]);
}

/* print a fixed-length string (stop at NUL or length) */
static void prstrn(const char *s, int n) {
    int i;
    for (i = 0; i < n && s[i]; ++i) prch(s[i]);
}

/* ---------- helpers for the allocator ---------- */
static void set_used(struct mem_block *b) { b->size &= ~0x1; }
static void set_free(struct mem_block *b) { b->size |= 0x1; }
static int is_free(struct mem_block *b) { return b->size & 0x1; }
static uint get_size(struct mem_block *b) { return b->size & ~0x1; }

/* store free list pointer at start of data area */
static void add_to_free_list(struct mem_block *b) {
    *(struct mem_block **)((char *)(b + 1)) = free_head;
    free_head = b;
}

/* remove b from free list (if present) */
static void remove_from_free_list(struct mem_block *b) {
    struct mem_block *prev = NULL;
    struct mem_block *cur = free_head;
    while (cur) {
        struct mem_block *next = *(struct mem_block **)((char *)(cur + 1));
        if (cur == b) {
            if (prev)
                *(struct mem_block **)((char *)(prev + 1)) = next;
            else
                free_head = next;
            break;
        }
        prev = cur;
        cur = next;
    }
}

static struct mem_block *find_free_block(uint size) {
    struct mem_block *result = NULL;
    struct mem_block *cur = free_head;
    if (!cur) return NULL;

    if (current_fsm == FIRST_FIT) {
        while (cur) {
            if (get_size(cur) >= size) return cur;
            cur = *(struct mem_block **)((char *)(cur + 1));
        }
    } else if (current_fsm == BEST_FIT) {
        uint best = (uint)-1;
        while (cur) {
            uint sz = get_size(cur);
            if (sz >= size && sz < best) { best = sz; result = cur; }
            cur = *(struct mem_block **)((char *)(cur + 1));
        }
    } else if (current_fsm == WORST_FIT) {
        uint worst = 0;
        while (cur) {
            uint sz = get_size(cur);
            if (sz >= size && sz > worst) { worst = sz; result = cur; }
            cur = *(struct mem_block **)((char *)(cur + 1));
        }
    }
    return result;
}

/* Split block b so first piece is `size` bytes of data (aligned). */
static void split_block(struct mem_block *b, uint size) {
    uint bsize = get_size(b);
    if (bsize < size + MIN_BLOCK_SIZE) return; // not enough to split
    char *newb_data = (char *)(b + 1) + size;
    struct mem_block *newb = (struct mem_block *)(newb_data);
    uint newb_data_size = bsize - size - sizeof(struct mem_block);
    /* initialize new block as free */
    newb->size = (newb_data_size & ~0x1) | 0x1;
    newb->prev_block = b;
    newb->next_block = b->next_block;
    if (b->next_block) b->next_block->prev_block = newb;
    b->next_block = newb;
    /* shrink original block to `size` (ensure used bit is cleared here) */
    b->size = (size & ~0x1);
    /* add newb to free list */
    add_to_free_list(newb);
}

/* Merge free neighbors and add resulting block to free list */
static void merge_block(struct mem_block *b) {
    if (!b || !is_free(b)) return;

    /* Merge with next */
    struct mem_block *next = b->next_block;
    if (next && is_free(next)) {
        remove_from_free_list(next);
        b->size = (get_size(b) + sizeof(struct mem_block) + get_size(next)) & ~0x1;
        b->next_block = next->next_block;
        if (b->next_block) b->next_block->prev_block = b;
    }

    /* Merge with prev */
    struct mem_block *prev = b->prev_block;
    if (prev && is_free(prev)) {
        remove_from_free_list(prev);
        prev->size = (get_size(prev) + sizeof(struct mem_block) + get_size(b)) & ~0x1;
        prev->next_block = b->next_block;
        if (b->next_block) b->next_block->prev_block = prev;
        b = prev;
    }

    add_to_free_list(b);
}

/* ---------- API ---------- */
void malloc_setfsm(int algo) {
    current_fsm = algo;
}

void malloc_scribble(int enable) {
    scribble_enabled = enable;
}

void *malloc(uint size) {
    if (size == 0) return NULL;
    size = ALIGN16(size);
    struct mem_block *b = find_free_block(size);
    if (b) {
        remove_from_free_list(b);
        split_block(b, size);
        set_used(b);
        if (scribble_enabled) memset((char *)(b + 1), 0xAA, get_size(b));
        return (void *)(b + 1);
    }

    /* Request pages from kernel. Round total to full pages. */
    uint total = sizeof(struct mem_block) + size;
    uint pages = (total + PAGE_SIZE - 1) / PAGE_SIZE;
    total = pages * PAGE_SIZE;
    b = (struct mem_block *)sbrk(total);
    if (b == (void *)-1) return NULL;

    b->size = (total - sizeof(struct mem_block)) & ~0x1; /* clear free bit */
    b->prev_block = NULL;
    b->next_block = NULL;
    set_used(b);

    /* append to block list (tail insertion) */
    if (block_head) {
        struct mem_block *tail = block_head;
        while (tail->next_block) tail = tail->next_block;
        tail->next_block = b;
        b->prev_block = tail;
    } else {
        block_head = b;
    }

    if (scribble_enabled) memset((char *)(b + 1), 0xAA, get_size(b));
    return (void *)(b + 1);
}

void free(void *ptr) {
    if (!ptr) return;
    struct mem_block *b = (struct mem_block *)ptr - 1;
    set_free(b);
    merge_block(b);

    /* Optionally: shrink program break if last block(s) are fully free and
       a whole page (or pages) at end can be returned. This is an extra step
       and can be implemented later if needed. */
}

void *calloc(uint nmemb, uint size) {
    uint total = nmemb * size;
    void *ptr = malloc(total);
    if (ptr) memset(ptr, 0, total);
    return ptr;
}

void *realloc(void *ptr, uint size) {
    if (!ptr) return malloc(size);
    if (size == 0) { free(ptr); return NULL; }

    struct mem_block *b = (struct mem_block *)ptr - 1;
    uint old_size = get_size(b);
    size = ALIGN16(size);

    /* shrinking in place */
    if (size <= old_size) {
        split_block(b, size);
        return ptr;
    }

    /* try to expand into next free block */
    struct mem_block *next = b->next_block;
    if (next && is_free(next)) {
        uint combined = old_size + sizeof(struct mem_block) + get_size(next);
        if (combined >= size) {
            remove_from_free_list(next);
            /* absorb next into b */
            b->size = (combined & ~0x1);
            b->next_block = next->next_block;
            if (b->next_block) b->next_block->prev_block = b;
            split_block(b, size);
            set_used(b);
            return ptr;
        }
    }

    /* else allocate new block, copy, free old */
    void *newptr = malloc(size);
    if (!newptr) return NULL;
    memmove(newptr, ptr, old_size);
    free(ptr);
    return newptr;
}

/* print memory layout and free list */
void malloc_print(void) {
    prstr("-- Current Memory State --\n");
    struct mem_block *b = block_head;
    while (b) {
        prstr("[BLOCK ");
        prptr((void *)b);
        prstr("-");
        prptr((void *)((char *)(b + 1) + get_size(b)));
        prstr("] ");
        pruint(get_size(b));
        prstr(" [");
        if (is_free(b)) prstr("FREE");
        else prstr("USED");
        prstr("] '");
        prstrn(b->name, sizeof(b->name));
        prstr("'\n");
        b = b->next_block;
    }

    prstr("-- Free List --\n");
    b = free_head;
    while (b) {
        prptr((void *)b);
        prstr(" -> ");
        b = *(struct mem_block **)((char *)(b + 1));
    }
    prstr("NULL\n");
}

/* report leaks (used blocks) */
void malloc_leaks(void) {
    struct mem_block *b = block_head;
    prstr("-- Leak Check --\n");
    while (b) {
        if (!is_free(b)) {
            prstr("[BLOCK ");
            prptr((void *)b);
            prstr("] ");
            pruint(get_size(b));
            prstr(" '");
            prstrn(b->name, sizeof(b->name));
            prstr("'\n");
        }
        b = b->next_block;
    }
}

