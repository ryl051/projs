#include <stdlib.h>
#include <string.h>
#include "allocator.h"
#include "alloccommon.h"

static void coalesce(HeapMetadata *metadata, BlockHeader *block){
    if(!metadata || !block)
        return;

    void* heap_end = (char *)metadata + metadata->total_size;
    void* heap_start = (char *)metadata + sizeof(HeapMetadata);
    BlockHeader* next_block = get_next_header(block);

    // Coalesce with previous block if it exists and is free
    if((void *)block > heap_start) {
        BlockHeader *prev_block = get_prev_header(block);
        if(!is_allocated(prev_block)) {
            size_t combined_size = get_size(prev_block) + get_size(block);
            set_header(prev_block, combined_size, 0);
            set_footer(prev_block);
            block = prev_block; // Update block pointer for potential next block coalescing
        }
    }

    // Coalesce with next block
    if((void *)next_block < heap_end && !is_allocated(next_block)) {
        size_t combined_size = get_size(block) + get_size(next_block);
        set_header(block, combined_size, 0);
        set_footer(block);
    }
}

void *custom_init(void *heap_start, void *heap_end) {
    if (heap_start >= heap_end) {
        return NULL;
    }

    HeapMetadata *metadata = (HeapMetadata *)heap_start;
    metadata->total_size = (char *)heap_end - (char *)heap_start;
    size_t available_size = metadata->total_size - sizeof(HeapMetadata);
    
    // Initialize the first block after the metadata
    BlockHeader *first_block = (BlockHeader *)((char *)heap_start + sizeof(HeapMetadata));
    set_header(first_block, available_size, 0);
    set_footer(first_block);
    
    return heap_start;
}

//allocate based on a first fit strategy
void *custom_alloc(void *heap_handle, size_t nbytes) {
    if (!heap_handle || nbytes == 0)
        return NULL;

    HeapMetadata *metadata = (HeapMetadata *)heap_handle; //using this to keep track of entire heap size, with no static variables
    
    // Calculate the total size needed including header
    size_t required_size = align_size(sizeof(BlockHeader) + nbytes + sizeof(BlockFooter));
    
    BlockHeader *current = (BlockHeader *)((char *)heap_handle + sizeof(HeapMetadata));
    void *heap_end = (char *)heap_handle + metadata->total_size;

    while ((void *)current < heap_end) {
        size_t current_size = get_size(current);
        if (!is_allocated(current) && current_size >= required_size) {
            
            if (current_size >= required_size + MIN_BLOCK_SIZE) {
                // Split the block
                BlockHeader *next_block = (BlockHeader *)((char *)current + required_size);
                size_t remaining_size = current_size - required_size;
                
                // Set headers for both blocks
                set_header(current, required_size, 1);
                set_footer(current);
                set_header(next_block, remaining_size, 0);
                set_footer(next_block);
                
            } else {
                set_header(current, current_size, 1);
                set_footer(current);
            }
            
            return (void *)((char *)current + sizeof(BlockHeader));
        }
        
        current = get_next_header(current);
    }
    
    return NULL;
}


void custom_free(void *s, void *p) {
    if (!s || !p)
        return;
    
    BlockHeader *header = (BlockHeader *)((char *)p - sizeof(BlockHeader));
    set_header(header, get_size(header), 0);
    set_footer(header);
    coalesce((HeapMetadata *)s, header);
}


void *custom_realloc(void *s, void *prev, size_t nbytes) {
    if (!prev) {
        return custom_alloc(s, nbytes);
    }

    if (nbytes == 0) {
        custom_free(s, prev);
        return NULL;
    }

    HeapMetadata *metadata = (HeapMetadata *)s;
    void *heap_start = (char *)s + sizeof(HeapMetadata);
    void *heap_end = (char *)s + metadata->total_size;
    
    BlockHeader *curr_block = (BlockHeader *)((char *)prev - sizeof(BlockHeader));
    size_t curr_size = get_size(curr_block);
    size_t required_size = align_size(sizeof(BlockHeader) + nbytes + sizeof(BlockFooter));
    
    // If shrinking or same size
    if (required_size <= curr_size) {
        if (curr_size - required_size >= MIN_BLOCK_SIZE) {
            BlockHeader *split_block = (BlockHeader *)((char *)curr_block + required_size);
            size_t remaining_size = curr_size - required_size;
            
            set_header(curr_block, required_size, 1);
            set_footer(curr_block);
            
            set_header(split_block, remaining_size, 0);
            set_footer(split_block);
        }
        return prev;
    }
    
    // Try to expand using next block if it's free
    BlockHeader *next_block = get_next_header(curr_block);
    if ((void *)next_block < heap_end && !is_allocated(next_block)) {
        size_t combined_size = curr_size + get_size(next_block);
        
        if (combined_size >= required_size) {
            // We can expand into next block
            if (combined_size - required_size >= MIN_BLOCK_SIZE) {
                // Split the expanded block
                BlockHeader *split_block = (BlockHeader *)((char *)curr_block + required_size);
                size_t remaining_size = combined_size - required_size;
                
                set_header(curr_block, required_size, 1);
                set_footer(curr_block);
                
                set_header(split_block, remaining_size, 0);
                set_footer(split_block);
            } else {
                // Use entire combined space
                set_header(curr_block, combined_size, 1);
                set_footer(curr_block);
            }
            return prev;
        }
    }
    
    // Try to expand using previous block if it's free
    if ((void *)curr_block > heap_start) {
        BlockHeader *prev_block = get_prev_header(curr_block);
        if (!is_allocated(prev_block)) {
            size_t prev_size = get_size(prev_block);
            size_t combined_size = curr_size + prev_size;
            
            // Also include next block if it's free
            if ((void *)next_block < heap_end && !is_allocated(next_block)) {
                combined_size += get_size(next_block);
            }
            
            if (combined_size >= required_size) {
                // Copy data to new location
                void *new_data = (void *)((char *)prev_block + sizeof(BlockHeader));
                memmove(new_data, prev, curr_size - sizeof(BlockHeader) - sizeof(BlockFooter));
                
                if (combined_size - required_size >= MIN_BLOCK_SIZE) {
                    // Split the expanded block
                    BlockHeader *split_block = (BlockHeader *)((char *)prev_block + required_size);
                    size_t remaining_size = combined_size - required_size;
                    
                    set_header(prev_block, required_size, 1);
                    set_footer(prev_block);
                    
                    set_header(split_block, remaining_size, 0);
                    set_footer(split_block);
                } else {
                    // Use entire combined space
                    set_header(prev_block, combined_size, 1);
                    set_footer(prev_block);
                }
                return new_data;
            }
        }
    }
    
    // If we can't expand in place, allocate new block
    void *new_ptr = custom_alloc(s, nbytes);
    if (!new_ptr) {
        return NULL;
    }
    
    // Copy old data to new location
    size_t old_data_size = curr_size - sizeof(BlockHeader) - sizeof(BlockFooter);
    size_t copy_size = (old_data_size < nbytes) ? old_data_size : nbytes;
    memcpy(new_ptr, prev, copy_size);
    
    custom_free(s, prev);
    return new_ptr;
}
