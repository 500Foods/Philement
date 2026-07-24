/*
 * Unity Test File: oidc_read_text_file
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_keys.h>
#include <stdio.h>
#include <unistd.h>

void test_read_text_file_null_path(void);
void test_read_text_file_nonexistent(void);
void test_read_text_file_success(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_read_text_file_null_path(void) {
    TEST_ASSERT_NULL(oidc_read_text_file(NULL));
}

void test_read_text_file_nonexistent(void) {
    TEST_ASSERT_NULL(oidc_read_text_file("/nonexistent_file_xyz.txt"));
}

void test_read_text_file_success(void) {
    char template[] = "/tmp/oidc_read_test_XXXXXX";
    char *dir = mkdtemp(template);
    TEST_ASSERT_NOT_NULL(dir);

    char path[512];
    snprintf(path, sizeof(path), "%s/test.txt", dir);
    TEST_ASSERT_TRUE(oidc_write_text_file(path, "test content"));

    char *content = oidc_read_text_file(path);
    TEST_ASSERT_NOT_NULL(content);
    TEST_ASSERT_EQUAL_STRING("test content", content);
    free(content);

    unlink(path);
    rmdir(dir);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_read_text_file_null_path);
    RUN_TEST(test_read_text_file_nonexistent);
    RUN_TEST(test_read_text_file_success);
    return UNITY_END();
}