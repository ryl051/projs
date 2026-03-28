#include <assert.h>
#include <stdio.h>
#include "allocator.h"
#include "alloccommon.h"

int custom_debug(void *heap_handle, int op) {
    if (!heap_handle) {
        printf("Error: Invalid heap handle\n");
        return 0;
    }

    HeapMetadata *metadata = (HeapMetadata *)heap_handle;
    size_t total_heap_size = metadata->total_size;
    
    BlockHeader *current = (BlockHeader *)((char *)heap_handle + sizeof(HeapMetadata));
    void *heap_end = (char *)heap_handle + total_heap_size;
    int block_count = 0;
    size_t total_allocated = 0;
    size_t total_free = 0;
    
    while ((void *)current < heap_end) {
        size_t block_size = get_size(current);
        int is_alloc = is_allocated(current);
        block_count++;

        printf("  Header %d at %p:\n", block_count, (void *)current);
        printf("  Footer %d at %p:\n", block_count, (void *)((char *)current + block_size - sizeof(BlockFooter)));
        printf("  Size: %zu\n", block_size);
        printf("  Allocated: %s\n", is_alloc ? "yes" : "no");
        printf("  Next block: %p\n", (void *)((char *)current + block_size));

        if (block_size < MIN_BLOCK_SIZE) {
            printf("Error: Block size too small\n");
            return 0;
        }
        
        if ((void *)((char *)current + block_size) > heap_end) {
            printf("Error: Block extends beyond heap end\n");
            return 0;
        }

        if (is_alloc) {
            total_allocated += block_size;
        } else {
            total_free += block_size;
        }

        BlockHeader *next = get_next_header(current);
        printf("DEBUG: Moving to next block: %p\n", next);
        
        if (next <= current) {
            printf("Error: Next block address (%p) not after current block (%p)\n", 
                   next, current);
            return 0;
        }
        
        current = next;
    }

    printf("\nHeap Summary:\n");
    printf("Total blocks: %d\n", block_count);
    printf("Total allocated space: %zu bytes\n", total_allocated);
    printf("Total free space: %zu bytes\n", total_free);
    printf("Total heap size: %zu bytes\n", total_heap_size);
    printf("Metadata size: %zu bytes\n", sizeof(HeapMetadata));
    printf("____________________________________________________\n\n");

    return 1;
}