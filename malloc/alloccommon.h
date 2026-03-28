#include <stddef.h>

typedef struct { //the data in the "header"
    size_t size; //size of blocks, w/ allocated bit as LSB (uint8_t?)
}BlockHeader;

typedef struct{
    size_t size;
}BlockFooter;

typedef struct{
    size_t total_size;
    BlockHeader first_block;
}HeapMetadata;

#define ALIGNMENT 8
#define ALLOCATED_BIT 1UL
#define MIN_BLOCK_SIZE (sizeof(BlockHeader) + ALIGNMENT)
#define SIZE_MASK (~ALLOCATED_BIT)

static size_t get_size(BlockHeader *header) {
    return header->size & SIZE_MASK;
}

static int is_allocated(BlockHeader *header) {
    return header->size & ALLOCATED_BIT;
}

static void set_header(BlockHeader *header, size_t size, int allocated) {
    header->size = size & SIZE_MASK | (allocated ? ALLOCATED_BIT : 0);
}

static void set_footer(BlockHeader *header) {
    BlockHeader *footer = (BlockHeader *)((char *)header + get_size(header) - sizeof(BlockHeader));
    footer->size = header -> size;
}

static BlockHeader *get_next_header(BlockHeader *header) {
    size_t size = get_size(header);
    BlockHeader *next = (BlockHeader *)((char *)header + size);
    return next;
}

static BlockHeader *get_prev_header(BlockHeader *header) {
    BlockFooter *prev_footer = (BlockFooter *)((char *)header - sizeof(BlockFooter));
    size_t prev_size = prev_footer->size & SIZE_MASK;
    return (BlockHeader *)((char *)header - prev_size);
}

static size_t align_size(size_t size) {
    return (size + (ALIGNMENT-1)) & ~(ALIGNMENT-1);
}
