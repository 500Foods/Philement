/*
 * Unity Test File: lua_mmap_alloc Function Tests
 * This file contains unit tests for the Lua custom memory allocator
 * that uses mmap/munmap directly to bypass malloc's heap structures
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <sys/mman.h>
#include <string.h>

// Forward declaration for function being tested
void* lua_mmap_alloc(void* ud, void* ptr, size_t osize, size_t nsize);

// Forward declarations for test functions
void test_lua_mmap_alloc_free_null_pointer(void);
void test_lua_mmap_alloc_free_zero_original_size(void);
void test_lua_mmap_alloc_free_valid_pointer(void);
void test_lua_mmap_alloc_new_allocation(void);
void test_lua_mmap_alloc_multiple_allocations(void);
void test_lua_mmap_alloc_reallocation_grow(void);
void test_lua_mmap_alloc_reallocation_shrink(void);
void test_lua_mmap_alloc_reallocation_same_size(void);
void test_lua_mmap_alloc_data_preservation(void);
void test_lua_mmap_alloc_large_reallocation(void);
void test_lua_mmap_alloc_ignores_ud_parameter(void);
void test_lua_mmap_alloc_sequential_operations(void);
void test_lua_mmap_alloc_realloc_to_zero(void);
void test_lua_mmap_alloc_realloc_zero_original_size(void);

void setUp(void) {
    // Set up test fixtures if needed
}

void tearDown(void) {
    // Clean up test fixtures if needed
}

/*
 * Test: Free request with NULL pointer
 * Purpose: Verify that freeing a NULL pointer is handled gracefully
 */
void test_lua_mmap_alloc_free_null_pointer(void) {
    void* result = lua_mmap_alloc(NULL, NULL, 0, 0);
    TEST_ASSERT_NULL(result);
}

/*
 * Test: Free request with valid pointer but zero size
 * Purpose: Verify that freeing with osize=0 doesn't attempt munmap
 */
void test_lua_mmap_alloc_free_zero_original_size(void) {
    // First allocate some memory
    void* ptr = lua_mmap_alloc(NULL, NULL, 0, 64);
    TEST_ASSERT_NOT_NULL(ptr);
    
    // Now free it with osize=0 (shouldn't actually free)
    void* result = lua_mmap_alloc(NULL, ptr, 0, 0);
    TEST_ASSERT_NULL(result);
    
    // Clean up properly
    lua_mmap_alloc(NULL, ptr, 64, 0);
}

/*
 * Test: Free request with valid pointer and valid size
 * Purpose: Verify normal free operation
 */
void test_lua_mmap_alloc_free_valid_pointer(void) {
    // First allocate some memory
    void* ptr = lua_mmap_alloc(NULL, NULL, 0, 128);
    TEST_ASSERT_NOT_NULL(ptr);
    
    // Free it
    void* result = lua_mmap_alloc(NULL, ptr, 128, 0);
    TEST_ASSERT_NULL(result);
}

/*
 * Test: New allocation request
 * Purpose: Verify basic memory allocation works
 */
void test_lua_mmap_alloc_new_allocation(void) {
    void* ptr = lua_mmap_alloc(NULL, NULL, 0, 256);
    TEST_ASSERT_NOT_NULL(ptr);
    
    // Verify we can write to the allocated memory
    memset(ptr, 0xAB, 256);
    
    // Clean up
    lua_mmap_alloc(NULL, ptr, 256, 0);
}

/*
 * Test: Multiple allocations
 * Purpose: Verify multiple independent allocations work
 */
void test_lua_mmap_alloc_multiple_allocations(void) {
    void* ptr1 = lua_mmap_alloc(NULL, NULL, 0, 64);
    void* ptr2 = lua_mmap_alloc(NULL, NULL, 0, 128);
    void* ptr3 = lua_mmap_alloc(NULL, NULL, 0, 256);
    
    TEST_ASSERT_NOT_NULL(ptr1);
    TEST_ASSERT_NOT_NULL(ptr2);
    TEST_ASSERT_NOT_NULL(ptr3);
    
    // Verify they're different addresses
    TEST_ASSERT_NOT_EQUAL(ptr1, ptr2);
    TEST_ASSERT_NOT_EQUAL(ptr2, ptr3);
    TEST_ASSERT_NOT_EQUAL(ptr1, ptr3);
    
    // Clean up
    lua_mmap_alloc(NULL, ptr1, 64, 0);
    lua_mmap_alloc(NULL, ptr2, 128, 0);
    lua_mmap_alloc(NULL, ptr3, 256, 0);
}

/*
 * Test: Reallocation to larger size
 * Purpose: Verify memory can be grown and data is preserved
 */
void test_lua_mmap_alloc_reallocation_grow(void) {
    // Allocate initial memory
    void* ptr = lua_mmap_alloc(NULL, NULL, 0, 64);
    TEST_ASSERT_NOT_NULL(ptr);
    
    // Write test pattern
    const char test_data[] = "Test data for reallocation";
    memcpy(ptr, test_data, sizeof(test_data));
    
    // Reallocate to larger size
    void* new_ptr = lua_mmap_alloc(NULL, ptr, 64, 256);
    TEST_ASSERT_NOT_NULL(new_ptr);
    
    // Verify data was preserved
    TEST_ASSERT_EQUAL_MEMORY(test_data, new_ptr, sizeof(test_data));
    
    // Clean up
    lua_mmap_alloc(NULL, new_ptr, 256, 0);
}

/*
 * Test: Reallocation to smaller size
 * Purpose: Verify memory can be shrunk and data is preserved (up to new size)
 */
void test_lua_mmap_alloc_reallocation_shrink(void) {
    // Allocate initial memory
    void* ptr = lua_mmap_alloc(NULL, NULL, 0, 256);
    TEST_ASSERT_NOT_NULL(ptr);
    
    // Write test pattern
    const char test_data[] = "Preserved data";
    memcpy(ptr, test_data, sizeof(test_data));
    
    // Reallocate to smaller size
    void* new_ptr = lua_mmap_alloc(NULL, ptr, 256, 64);
    TEST_ASSERT_NOT_NULL(new_ptr);
    
    // Verify data was preserved (up to new size)
    TEST_ASSERT_EQUAL_MEMORY(test_data, new_ptr, sizeof(test_data));
    
    // Clean up
    lua_mmap_alloc(NULL, new_ptr, 64, 0);
}

/*
 * Test: Reallocation to same size
 * Purpose: Verify reallocation to same size still creates new memory
 */
void test_lua_mmap_alloc_reallocation_same_size(void) {
    // Allocate initial memory
    void* ptr = lua_mmap_alloc(NULL, NULL, 0, 128);
    TEST_ASSERT_NOT_NULL(ptr);
    
    // Write test pattern
    const char test_data[] = "Same size realloc";
    memcpy(ptr, test_data, sizeof(test_data));
    
    // Reallocate to same size
    void* new_ptr = lua_mmap_alloc(NULL, ptr, 128, 128);
    TEST_ASSERT_NOT_NULL(new_ptr);
    
    // Verify data was preserved
    TEST_ASSERT_EQUAL_MEMORY(test_data, new_ptr, sizeof(test_data));
    
    // Clean up
    lua_mmap_alloc(NULL, new_ptr, 128, 0);
}

/*
 * Test: Reallocation with full data preservation
 * Purpose: Verify complete data integrity during reallocation
 */
void test_lua_mmap_alloc_data_preservation(void) {
    const size_t initial_size = 64;
    const size_t new_size = 128;
    
    // Allocate and fill with pattern
    void* ptr = lua_mmap_alloc(NULL, NULL, 0, initial_size);
    TEST_ASSERT_NOT_NULL(ptr);
    
    // Fill with recognizable pattern
    for (size_t i = 0; i < initial_size; i++) {
        ((unsigned char*)ptr)[i] = (unsigned char)(i & 0xFF);
    }
    
    // Reallocate
    void* new_ptr = lua_mmap_alloc(NULL, ptr, initial_size, new_size);
    TEST_ASSERT_NOT_NULL(new_ptr);
    
    // Verify all original data is intact
    for (size_t i = 0; i < initial_size; i++) {
        TEST_ASSERT_EQUAL_UINT8((unsigned char)(i & 0xFF), ((unsigned char*)new_ptr)[i]);
    }
    
    // Clean up
    lua_mmap_alloc(NULL, new_ptr, new_size, 0);
}

/*
 * Test: Reallocation from small to very large
 * Purpose: Verify large reallocations work correctly
 */
void test_lua_mmap_alloc_large_reallocation(void) {
    const size_t small_size = 32;
    const size_t large_size = 4096;
    
    // Allocate small memory
    void* ptr = lua_mmap_alloc(NULL, NULL, 0, small_size);
    TEST_ASSERT_NOT_NULL(ptr);
    
    // Write test data
    memset(ptr, 0xCD, small_size);
    
    // Reallocate to large size
    void* new_ptr = lua_mmap_alloc(NULL, ptr, small_size, large_size);
    TEST_ASSERT_NOT_NULL(new_ptr);
    
    // Verify original data preserved
    for (size_t i = 0; i < small_size; i++) {
        TEST_ASSERT_EQUAL_UINT8(0xCD, ((unsigned char*)new_ptr)[i]);
    }
    
    // Clean up
    lua_mmap_alloc(NULL, new_ptr, large_size, 0);
}

/*
 * Test: ud parameter is ignored
 * Purpose: Verify user data parameter doesn't affect allocator behavior
 */
void test_lua_mmap_alloc_ignores_ud_parameter(void) {
    void* fake_ud = (void*)0xDEADBEEF;
    
    // Test allocation with non-NULL ud
    void* ptr = lua_mmap_alloc(fake_ud, NULL, 0, 128);
    TEST_ASSERT_NOT_NULL(ptr);
    
    // Test free with non-NULL ud
    void* result = lua_mmap_alloc(fake_ud, ptr, 128, 0);
    TEST_ASSERT_NULL(result);
}

/*
 * Test: Sequential allocations and frees
 * Purpose: Verify allocator maintains consistency across multiple operations
 */
void test_lua_mmap_alloc_sequential_operations(void) {
    void* ptr1 = lua_mmap_alloc(NULL, NULL, 0, 64);
    TEST_ASSERT_NOT_NULL(ptr1);
    
    void* ptr2 = lua_mmap_alloc(NULL, NULL, 0, 128);
    TEST_ASSERT_NOT_NULL(ptr2);
    
    // Free first
    lua_mmap_alloc(NULL, ptr1, 64, 0);
    
    // Allocate again
    void* ptr3 = lua_mmap_alloc(NULL, NULL, 0, 256);
    TEST_ASSERT_NOT_NULL(ptr3);
    
    // Free remaining
    lua_mmap_alloc(NULL, ptr2, 128, 0);
    lua_mmap_alloc(NULL, ptr3, 256, 0);
}

/*
 * Test: Zero-byte reallocation (edge case)
 * Purpose: Verify reallocation to zero size is handled as free
 */
void test_lua_mmap_alloc_realloc_to_zero(void) {
    // Allocate memory
    void* ptr = lua_mmap_alloc(NULL, NULL, 0, 128);
    TEST_ASSERT_NOT_NULL(ptr);
    
    // Reallocate to zero (should free)
    void* result = lua_mmap_alloc(NULL, ptr, 128, 0);
    TEST_ASSERT_NULL(result);
}

/*
 * Test: Reallocation with zero original size
 * Purpose: Verify edge case of realloc with osize=0
 */
void test_lua_mmap_alloc_realloc_zero_original_size(void) {
    // Allocate initial memory
    void* ptr = lua_mmap_alloc(NULL, NULL, 0, 64);
    TEST_ASSERT_NOT_NULL(ptr);
    
    // Reallocate with osize=0 (edge case)
    void* new_ptr = lua_mmap_alloc(NULL, ptr, 0, 128);
    TEST_ASSERT_NOT_NULL(new_ptr);
    
    // Note: Original ptr may be leaked in this edge case, but the function
    // should still return valid new memory
    
    // Clean up new allocation
    lua_mmap_alloc(NULL, new_ptr, 128, 0);
}

int main(void) {
    UNITY_BEGIN();
    
    // Free operation tests
    RUN_TEST(test_lua_mmap_alloc_free_null_pointer);
    RUN_TEST(test_lua_mmap_alloc_free_zero_original_size);
    RUN_TEST(test_lua_mmap_alloc_free_valid_pointer);
    
    // Allocation tests
    RUN_TEST(test_lua_mmap_alloc_new_allocation);
    RUN_TEST(test_lua_mmap_alloc_multiple_allocations);
    
    // Reallocation tests
    RUN_TEST(test_lua_mmap_alloc_reallocation_grow);
    RUN_TEST(test_lua_mmap_alloc_reallocation_shrink);
    RUN_TEST(test_lua_mmap_alloc_reallocation_same_size);
    RUN_TEST(test_lua_mmap_alloc_data_preservation);
    RUN_TEST(test_lua_mmap_alloc_large_reallocation);
    RUN_TEST(test_lua_mmap_alloc_realloc_to_zero);
    RUN_TEST(test_lua_mmap_alloc_realloc_zero_original_size);
    
    // Parameter and behavior tests
    RUN_TEST(test_lua_mmap_alloc_ignores_ud_parameter);
    RUN_TEST(test_lua_mmap_alloc_sequential_operations);
    
    return UNITY_END();
}