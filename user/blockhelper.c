struct mem_block {
    /**
     * The name of this memory block. If the user doesn't specify a name for the
     * block, it should be left empty (a single null byte).
     */
    char name[8];

    /** Size of the block */
    uint size;

    /** Links for our doubly-linked list of blocks: */
    struct mem_block *next_block;
    struct mem_block *prev_block;
}

struct memSize() {

}
