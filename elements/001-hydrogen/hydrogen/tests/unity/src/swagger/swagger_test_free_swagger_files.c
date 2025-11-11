/*
 * Unity Test File: free_swagger_files Function Tests
 * This file contains unit tests for the free_swagger_files() function
 * from src/swagger/swagger.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <src/swagger/swagger.h>

// Structure to hold in-memory Swagger files (copied from swagger.c)
typedef struct {
    char *name;         // File name (e.g., "index.html")
    uint8_t *data;      // File content
    size_t size;        // Content size
    bool is_compressed; // Whether content is Brotli compressed
} SwaggerFile;

// External declarations for global state
extern SwaggerFile *swagger_files;
extern size_t num_swagger_files;

// Forward declaration for the function being tested
void free_swagger_files(void);

// Function prototypes for test functions
void test_free_swagger_files_null_pointer(void);
void test_free_swagger_files_empty_array(void);
void test_free_swagger_files_with_files(void);
void test_free_swagger_files_with_null_names(void);
void test_free_swagger_files_with_null_data(void);

void setUp(void) {
    // Reset global state
    swagger_files = NULL;
    num_swagger_files = 0;
}

void tearDown(void) {
    // Clean up any remaining files
    if (swagger_files) {
        free_swagger_files();
    }
}

void test_free_swagger_files_null_pointer(void) {
    // Should handle NULL swagger_files gracefully
    swagger_files = NULL;
    num_swagger_files = 0;
    free_swagger_files();
    TEST_ASSERT_NULL(swagger_files);
    TEST_ASSERT_EQUAL(0, num_swagger_files);
}

void test_free_swagger_files_empty_array(void) {
    // Allocate empty array
    swagger_files = calloc(1, sizeof(SwaggerFile));
    if (!swagger_files) return; // cppcheck-suppress[nullPointerOutOfMemory]
    num_swagger_files = 0;

    free_swagger_files();
    TEST_ASSERT_NULL(swagger_files);
    TEST_ASSERT_EQUAL(0, num_swagger_files);
}

void test_free_swagger_files_with_files(void) {
    // Allocate array with files
    num_swagger_files = 2;
    swagger_files = calloc(num_swagger_files, sizeof(SwaggerFile));
    if (!swagger_files) return; // cppcheck-suppress[nullPointerOutOfMemory]

    // Initialize files
    swagger_files[0].name = strdup("swagger.html");
    if (!swagger_files[0].name) return; // cppcheck-suppress[nullPointerOutOfMemory]
    swagger_files[0].data = (uint8_t*)strdup("<html>swagger-ui</html>");
    if (!swagger_files[0].data) return; // cppcheck-suppress[nullPointerOutOfMemory]
    swagger_files[0].size = 15;
    swagger_files[0].is_compressed = false;

    swagger_files[1].name = strdup("style.css");
    if (!swagger_files[1].name) return; // cppcheck-suppress[nullPointerOutOfMemory]
    swagger_files[1].data = (uint8_t*)strdup("body{}");
    if (!swagger_files[1].data) return; // cppcheck-suppress[nullPointerOutOfMemory]
    swagger_files[1].size = 6;
    swagger_files[1].is_compressed = true;

    free_swagger_files();
    TEST_ASSERT_NULL(swagger_files);
    TEST_ASSERT_EQUAL(0, num_swagger_files);
}

void test_free_swagger_files_with_null_names(void) {
    // Test with NULL names (should not crash)
    num_swagger_files = 1;
    swagger_files = calloc(num_swagger_files, sizeof(SwaggerFile));
    if (!swagger_files) return; // cppcheck-suppress[nullPointerOutOfMemory]

    swagger_files[0].name = NULL;
    swagger_files[0].data = (uint8_t*)strdup("test");
    if (!swagger_files[0].data) return; // cppcheck-suppress[nullPointerOutOfMemory]
    swagger_files[0].size = 4;
    swagger_files[0].is_compressed = false;

    free_swagger_files();
    TEST_ASSERT_NULL(swagger_files);
    TEST_ASSERT_EQUAL(0, num_swagger_files);
}

void test_free_swagger_files_with_null_data(void) {
    // Test with NULL data (should not crash)
    num_swagger_files = 1;
    swagger_files = calloc(num_swagger_files, sizeof(SwaggerFile));
    if (!swagger_files) return; // cppcheck-suppress[nullPointerOutOfMemory]

    swagger_files[0].name = strdup("test.txt");
    if (!swagger_files[0].name) return; // cppcheck-suppress[nullPointerOutOfMemory]
    swagger_files[0].data = NULL;
    swagger_files[0].size = 0;
    swagger_files[0].is_compressed = false;

    free_swagger_files();
    TEST_ASSERT_NULL(swagger_files);
    TEST_ASSERT_EQUAL(0, num_swagger_files);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_free_swagger_files_null_pointer);
    RUN_TEST(test_free_swagger_files_empty_array);
    RUN_TEST(test_free_swagger_files_with_files);
    RUN_TEST(test_free_swagger_files_with_null_names);
    RUN_TEST(test_free_swagger_files_with_null_data);

    return UNITY_END();
}