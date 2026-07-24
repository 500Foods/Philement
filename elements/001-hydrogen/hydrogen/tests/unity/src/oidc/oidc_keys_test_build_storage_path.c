/*
 * Unity Test File: oidc_build_storage_path
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_keys.h>
#include <unity/mocks/mock_system.h>
#include <stdlib.h>

void test_build_storage_path_null_dir(void);
void test_build_storage_path_null_filename(void);
void test_build_storage_path_normal(void);
void test_build_storage_path_malloc_failure(void);

void setUp(void) {
    mock_system_reset_all();
}

void tearDown(void) {
    mock_system_reset_all();
}

void test_build_storage_path_null_dir(void) {
    TEST_ASSERT_NULL(oidc_build_storage_path(NULL, "file.txt"));
}

void test_build_storage_path_null_filename(void) {
    TEST_ASSERT_NULL(oidc_build_storage_path("/tmp", NULL));
}

void test_build_storage_path_normal(void) {
    char *path = oidc_build_storage_path("/tmp", "file.txt");
    TEST_ASSERT_NOT_NULL(path);
    TEST_ASSERT_EQUAL_STRING("/tmp/file.txt", path);
    free(path);
}

void test_build_storage_path_malloc_failure(void) {
    mock_system_set_malloc_failure(1);
    TEST_ASSERT_NULL(oidc_build_storage_path("/tmp", "file.txt"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_build_storage_path_null_dir);
    RUN_TEST(test_build_storage_path_null_filename);
    RUN_TEST(test_build_storage_path_normal);
    RUN_TEST(test_build_storage_path_malloc_failure);
    return UNITY_END();
}
