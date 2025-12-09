#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MIN_DATA_SIZE 16        // minimum data size
#define ALIGN 16                // align allocations to 16 bytes
#define MIN_BLOCK_SIZE 48       // header + minimum data
#define PAGE_SIZE 4096

enum fsm_algorithm { FIRST_FIT, BEST_FIT, WORST_FIT };

struct mem_block {
    char name[8];               // optional name
    uint size;                  // last bit = free flag
    struct mem_block *next_block;
    struct mem_block *prev_block;
};

// Free list pointer is stored in data portion of freed block
static struct mem_block *head = 0;         // head of all blocks
static struct mem_block *free_list = 0;    // head of free blocks

static int scribble_enabled = 0;
static enum fsm_algorithm current_fsm = FIRST_FIT;

/* ------------------ Helpers ------------------ */

static uint align_size(uint size) {
    return (size + (ALIGN - 1)) & ~(ALIGN - 1);
}

static void set_used(struct mem_block *b) {
    b->size &= ~0x01;
}

static void set_free(struct mem_block *b) {
    b->size |= 0x01;
}

static int is_free(struct mem_block *b) {
    return b->size & 0x01;
}

static uint get_size(struct mem_block *b) {
    return b->size & ~0x01;
}

/* Add block to free list */
static void add_to_free_list(struct mem_block *b) {
    b->next_block = free_list;
    free_list = b;
}

/* Remove block from free list */
static void remove_from_free_list(struct mem_block *b) {
    struct mem_block **curr = &free_list;
    while (*curr) {
        if (*curr == b) {
            *curr = b->next_block;
            b->next_block = 0;
            return;
        }
        curr = &(*curr)->next_block;
    }
}

/* Merge adjacent free blocks */
static void coalesce(struct mem_block *b) {
    // Merge with previous block if free
    if (b->prev_block && is_free(b->prev_block)) {
        struct mem_block *prev = b->prev_block;
        prev->size += get_size(b);
        prev->size &= ~0x01;   // mark prev as used temporarily
        prev->next_block = b->next_block;
        if (b->next_block) b->next_block->prev_block = prev;
        remove_from_free_list(b);
        b = prev;
        set_free(b);
    }

    // Merge with next block if free
    if (b->next_block && is_free(b->next_block)) {
        struct mem_block *next = b->next_block;
        b->size += get_size(next);
        b->next_block = next->next_block;
        if (next->next_block) next->next_block->prev_block = b;
        remove_from_free_list(next);
        set_free(b);
    }
}

/* ------------------ FSM search ------------------ */

static struct mem_block *find_free_block(uint size) {
    struct mem_block *best = 0;
    struct mem_block *curr = free_list;

    while (curr) {
        if (get_size(curr) >= size) {
            switch (current_fsm) {
                case FIRST_FIT:
                    return curr;
                case BEST_FIT:
                    if (!best || get_size(curr) < get_size(best))
                        best = curr;
                    break;
                case WORST_FIT:
                    if (!best || get_size(curr) > get_size(best))
                        best = curr;
                    break;
            }
        }
        curr = curr->next_block;
    }
    return best;
}

/* ------------------ Split ------------------ */

static void split_block(struct mem_block *b, uint size) {
    uint remaining = get_size(b) - size - sizeof(struct mem_block);
    if (remaining >= MIN_BLOCK_SIZE) {
        struct mem_block *newb = (struct mem_block*)((char*)b + sizeof(struct mem_block) + size);
        newb->size = remaining | 0x01;  // mark free
        newb->prev_block = b;
        newb->next_block = b->next_block;
        if (b->next_block) b->next_block->prev_block = newb;
        b->next_block = newb;
        b->size = size;  // mark used
        add_to_free_list(newb);
    }
}

/* ------------------ malloc ------------------ */

void *malloc(uint size) {
    if (size == 0) return 0;
    size = align_size(size);

    struct mem_block *b = find_free_block(size);
    if (b) {
        remove_from_free_list(b);
        split_block(b, size);
        set_used(b);
        if (scribble_enabled)
            memset((char*)(b + 1), 0xAA, get_size(b));
        return b + 1;
    }

    // Need new memory
    uint total_size = sizeof(struct mem_block) + size;
    uint pages = (total_size + PAGE_SIZE - 1) / PAGE_SIZE;
    uint request = pages * PAGE_SIZE;

    struct mem_block *newb = (struct mem_block*)sbrk(request);
    if (newb == (void*)-1) return 0;

    newb->size = request - sizeof(struct mem_block); // data portion
    newb->prev_block = 0;
    newb->next_block = 0;

    // Add to block list
    if (!head) head = newb;
    else {
        struct mem_block *tmp = head;
        while (tmp->next_block) tmp = tmp->next_block;
        tmp->next_block = newb;
        newb->prev_block = tmp;
    }

    split_block(newb, size);
    set_used(newb);

    if (scribble_enabled)
        memset((char*)(newb + 1), 0xAA, get_size(newb));

    return newb + 1;
}

/* ------------------ free ------------------ */

void free(void *ptr) {
    if (!ptr) return;
    struct mem_block *b = (struct mem_block*)ptr - 1;
    set_free(b);
    add_to_free_list(b);
    coalesce(b);
}

/* ------------------ calloc ------------------ */

void *calloc(uint nmemb, uint size) {
    uint total = nmemb * size;
    char *ptr = malloc(total);
    if (!ptr) return 0;
    memset(ptr, 0, total);
    return ptr;
}

/* ------------------ realloc ------------------ */

void *realloc(void *ptr, uint size) {
    if (!ptr) return malloc(size);
    if (size == 0) {
        free(ptr);
        return 0;
    }

    struct mem_block *b = (struct mem_block*)ptr - 1;
    uint old_size = get_size(b);
    size = align_size(size);

    if (size <= old_size) {
        split_block(b, size);
        return ptr;
    }

    // Check if next block is free and can expand
    if (b->next_block && is_free(b->next_block) &&
        old_size + sizeof(struct mem_block) + get_size(b->next_block) >= size) {
        struct mem_block *next = b->next_block;
        remove_from_free_list(next);
        b->size += sizeof(struct mem_block) + get_size(next);
        b->next_block = next->next_block;
        if (next->next_block) next->next_block->prev_block = b;
        split_block(b, size);
        return ptr;
    }

    // Allocate new block
    char *newptr = malloc(size);
    if (!newptr) return 0;
    memmove(newptr, ptr, old_size);
    free(ptr);
    return newptr;
}

/* ------------------ Scribble ------------------ */

void malloc_scribble(int enable) {
    scribble_enabled = enable;
}

/* ------------------ Set FSM ------------------ */

void malloc_setfsm(int algo) {
    if (algo >= 0 && algo <= 2)
        current_fsm = (enum fsm_algorithm)algo;
}

/* ------------------ Print Memory ------------------ */

void malloc_print(void) {
    struct mem_block *b = head;
    printf("-- Current Memory State --\n");
    while (b) {
        printf("[BLOCK %p-%p] %d [%s] '%s'\n",
            b, (char*)b + get_size(b) + sizeof(struct mem_block),
            get_size(b),
            is_free(b) ? "FREE" : "USED",
            b->name);
        b = b->next_block;
    }

    printf("\n-- Free List --\n");
    b = free_list;
    while (b) {
        printf("[%p] -> ", (char*)(b + 1));
        b = b->next_block;
    }
    printf("NULL\n");
}

/* ------------------ Leak Check ------------------ */

void malloc_leaks(void) {
    struct mem_block *b = head;
    int blocks = 0;
    int bytes = 0;

    printf("-- Leak Check --\n");
    while (b) {
        if (!is_free(b)) {
            printf("[BLOCK %p] %d '%s'\n", b, get_size(b), b->name);
            blocks++;
            bytes += get_size(b);
        }
        b = b->next_block;
    }

    printf("-- Summary --\n");
    printf("%d blocks lost (%d bytes)\n", blocks, bytes);
}
