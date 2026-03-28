#include <stdlib.h>
#include <stdbool.h>

// - initialize an allocator
void *custom_init(void *heap_start, void *heap_end);

// - allocate a block of memory
void *custom_alloc(void *heap_handle, size_t nbytes);

// - free a previously allocated a block of memory
void custom_free(void *heap_handle, void *p);

// - extend or shrink allocated block size, moving the block address and contents if necessary
void *custom_realloc(void *heap_handle, void *prev, size_t nbytes);

// - checks the heap for consistency and/or implements other debug functionality
int custom_debug(void *heap_handle, int op);