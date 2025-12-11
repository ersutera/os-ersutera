#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define MIN_DATA_SIZE 16
#define ALIGN 16
#define MIN_BLOCK_SIZE 48
#define PAGE_SIZE 4096

enum fsm_algorithm { FIRST_FIT, BEST_FIT, WORST_FIT };

struct mem_block {
    char name[8];
    uint size;                  // low bit = free flag
    struct mem_block *next_block;
    struct mem_block *prev_block;
};

static struct mem_block *head = 0;
static struct mem_block *free_list = 0;
static int scribble_enabled = 0;
static enum fsm_algorithm current_fsm = FIRST_FIT;

// Helpers

static uint align_size(uint size) {
    return (size + (ALIGN - 1)) & ~(ALIGN - 1);
}

static void set_used(struct mem_block *b) { b->size &= ~0x01; }
static void set_free(struct mem_block *b) { b->size |= 0x01; }
static int is_free(struct mem_block *b) { return (b->size & 0x01) != 0; }
static uint get_size(struct mem_block *b) { return b->size & ~0x01; }

static void set_free_next(struct mem_block *b, struct mem_block *next) {
    struct mem_block **p = (struct mem_block **)(b + 1);
    *p = next;
}

static struct mem_block *get_free_next(struct mem_block *b) {
    struct mem_block **p = (struct mem_block **)(b + 1);
    return *p;
}

static void add_to_free_list(struct mem_block *b) {
    set_free_next(b, 0);
    if (!free_list) {
        free_list = b;
        return;
    }

    if (current_fsm == FIRST_FIT) {
        set_free_next(b, free_list);
        free_list = b;
    } else { // BEST or WORST FIT
        struct mem_block *cur = free_list;
        while (get_free_next(cur)) {cur = get_free_next(cur);}
        set_free_next(cur, b);
    }
}

static void remove_from_free_list(struct mem_block *b) {
    struct mem_block *prev = 0;
    struct mem_block *cur = free_list;
    while (cur) {
        if (cur == b) {
            if (prev)
                set_free_next(prev, get_free_next(cur));
            else
                free_list = get_free_next(cur);
            set_free_next(cur, 0);
            return;
        }
        prev = cur;
        cur = get_free_next(cur);
    }
}

// Coalescing

static void coalesce(struct mem_block *b) {
    remove_from_free_list(b);

    if (b->prev_block && is_free(b->prev_block)) {
        struct mem_block *prev = b->prev_block;
        remove_from_free_list(prev);
        uint new_data = get_size(prev) + sizeof(struct mem_block) + get_size(b);
        prev->size = new_data | 0x01;
        prev->next_block = b->next_block;
        if (b->next_block)
            b->next_block->prev_block = prev;
        b = prev;
    }

    if (b->next_block && is_free(b->next_block)) {
        struct mem_block *next = b->next_block;
        remove_from_free_list(next);
        uint new_data = get_size(b) + sizeof(struct mem_block) + get_size(next);
        b->size = new_data | 0x01;
        b->next_block = next->next_block;
        if (next->next_block)
            next->next_block->prev_block = b;
    }

    add_to_free_list(b);
}

// FSM search

static struct mem_block *find_free_block(uint size) {
    struct mem_block *best = 0;
    struct mem_block *cur = free_list;

    while (cur) {
        if (get_size(cur) >= size) {
            switch (current_fsm) {
                case FIRST_FIT:
                    return cur;
                case BEST_FIT:
                    if (!best || get_size(cur) < get_size(best))
                        best = cur;
                    break;
                case WORST_FIT:
                    if (!best || get_size(cur) > get_size(best))
                        best = cur;
                    break;
            }
        }
        cur = get_free_next(cur);
    }
    return best;
}

// Split

static void split_block(struct mem_block *b, uint size) {
    uint bsize = get_size(b);
    if (bsize < size + sizeof(struct mem_block) + MIN_DATA_SIZE)
        return;

    char *newptr = (char *)b + sizeof(struct mem_block) + size;
    struct mem_block *newb = (struct mem_block *)newptr;

    uint new_data = bsize - size - sizeof(struct mem_block);
    newb->size = new_data | 0x01;  // free
    newb->prev_block = b;
    newb->next_block = b->next_block;
    if (b->next_block)
        b->next_block->prev_block = newb;
    b->next_block = newb;

    b->size = size & ~0x01;  // allocated block
    set_free(newb);
    add_to_free_list(newb);  // tail insertion
}

// malloc/free/calloc/realloc

void *malloc(uint size) {
    if (size == 0) return 0;

    size = align_size(size);
    if (size < MIN_DATA_SIZE) size = MIN_DATA_SIZE;

    struct mem_block *b = find_free_block(size);
    if (b) {
        remove_from_free_list(b);
        split_block(b, size);
        set_used(b);
        if (scribble_enabled)
            memset(b + 1, 0xAA, get_size(b));
        return (void *)(b + 1);
    }

    uint total_size = sizeof(struct mem_block) + size;
    uint pages = (total_size + PAGE_SIZE - 1) / PAGE_SIZE;
    uint request = pages * PAGE_SIZE;

    struct mem_block *newb = (struct mem_block *)sbrk(request);
    if (newb == (void *)-1) return 0;

    newb->size = (request - sizeof(struct mem_block)) & ~0x01;
    newb->prev_block = 0;
    newb->next_block = 0;
    newb->name[0] = '\0';

    if (!head)
        head = newb;
    else {
        struct mem_block *tmp = head;
        while (tmp->next_block)
            tmp = tmp->next_block;
        tmp->next_block = newb;
        newb->prev_block = tmp;
    }

    split_block(newb, size);
    set_used(newb);
    if (scribble_enabled)
        memset(newb + 1, 0xAA, get_size(newb));

    return (void *)(newb + 1);
}

void free(void *ptr) {
    if (!ptr) return;
    struct mem_block *b = (struct mem_block *)ptr - 1;
    set_free(b);
    add_to_free_list(b);
    coalesce(b);
}

void *calloc(uint nmemb, uint size) {
    uint total = nmemb * size;
    void *p = malloc(total);
    if (!p) return 0;
    memset(p, 0, total);
    return p;
}

void *realloc(void *ptr, uint size) {
    if (!ptr) return malloc(size);
    if (size == 0) { free(ptr); return 0; }

    size = align_size(size);
    if (size < MIN_DATA_SIZE) size = MIN_DATA_SIZE;

    struct mem_block *b = (struct mem_block *)ptr - 1;
    uint old = get_size(b);

    if (size <= old) {
        split_block(b, size);
        return ptr;
    }

    if (b->next_block && is_free(b->next_block)) {
        struct mem_block *next = b->next_block;
        uint combined = old + sizeof(struct mem_block) + get_size(next);
        if (combined >= size) {
            remove_from_free_list(next);
            b->size = combined & ~0x01;
            b->next_block = next->next_block;
            if (next->next_block)
                next->next_block->prev_block = b;
            split_block(b, size);
            set_used(b);
            if (scribble_enabled) {
                char *start = (char *)b + sizeof(struct mem_block) + old;
                uint scribble_len = get_size(b) - old;
                if (scribble_len)
                    memset(start, 0xAA, scribble_len);
            }
            return ptr;
        }
    }

    void *newp = malloc(size);
    if (!newp) return 0;
    memmove(newp, ptr, old);
    free(ptr);
    return newp;
}

// Scribble and FSM

void malloc_scribble(int enable) { scribble_enabled = enable ? 1 : 0; }
void malloc_setfsm(int algo) { if (algo >= 0 && algo <= 2) current_fsm = (enum fsm_algorithm)algo; }

// Debug/Leak

void malloc_print(void) {
    struct mem_block *b = head;
    printf("-- Current Memory State --\n");
    while (b) {
        void *start = (void *)b;
        void *end = (void *)((char *)b + sizeof(struct mem_block) + get_size(b));
        printf("[BLOCK %p-%p] %d [%s] '%s'\n",
               start, end, get_size(b),
               is_free(b) ? "FREE" : "USED",
               b->name[0] ? b->name : "");
        b = b->next_block;
    }

    printf("\n-- Free List --\n");
    struct mem_block *f = free_list;
    while (f) {
        void *userptr = (void *)(f + 1);
        printf("[%p] -> ", userptr);
        f = get_free_next(f);
    }
    printf("NULL\n");
}

void malloc_leaks(void) {
    struct mem_block *b = head;
    int blocks = 0, bytes = 0;
    printf("-- Leak Check --\n");
    while (b) {
        if (!is_free(b)) {
            printf("[BLOCK %p] %d '%s'\n", (void *)b, get_size(b), b->name[0] ? b->name : "");
            blocks++;
            bytes += get_size(b);
        }
        b = b->next_block;
    }
    printf("-- Summary --\n");
    printf("%d blocks lost (%d bytes)\n", blocks, bytes);
}

