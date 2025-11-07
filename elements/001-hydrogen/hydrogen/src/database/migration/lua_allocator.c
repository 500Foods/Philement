/*
 * Custom Memory Allocator for Lua
 *
 * Uses mmap/munmap directly to bypass malloc's corrupted heap structures.
 * This prevents Lua's memory corruption from affecting the process heap.
 */

#include <src/hydrogen.h>
#include <sys/mman.h>
#include <string.h>

// Forward declaration for Lua's allocator function signature
void* lua_mmap_alloc(void* ud, void* ptr, size_t osize, size_t nsize);

/*
 * Custom allocator that uses mmap/munmap instead of malloc/free
 * This completely bypasses the standard heap and prevents corruption propagation
 */
void* lua_mmap_alloc(void* ud, void* ptr, size_t osize, size_t nsize) {
    (void)ud;  // Unused parameter
    
    if (nsize == 0) {
        // Free request
        if (ptr && osize > 0) {
            munmap(ptr, osize);
        }
        return NULL;
    }
    
    if (ptr == NULL) {
        // New allocation request
        void* p = mmap(NULL, nsize, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (p == MAP_FAILED) {
            return NULL;
        }
        return p;
    }
    
    // Reallocation request - mmap doesn't support realloc, so we have to:
    // 1. Allocate new block
    // 2. Copy old data
    // 3. Free old block
    void* newptr = mmap(NULL, nsize, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (newptr == MAP_FAILED) {
        return NULL;
    }
    
    // Copy old data to new location
    size_t copy_size = (osize < nsize) ? osize : nsize;
    if (copy_size > 0) {
        memcpy(newptr, ptr, copy_size);
    }
    
    // Free old block
    if (osize > 0) {
        munmap(ptr, osize);
    }
    
    return newptr;
}