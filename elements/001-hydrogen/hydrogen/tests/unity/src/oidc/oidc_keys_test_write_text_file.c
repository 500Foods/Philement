/*
 * Unity Test File: oidc_write_text_file
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_keys.h>
#include <stdio.h>
#include <unistd.h>

void test_write_text_file_null_path(void);
void test_write_text_file_null_contents(void);
void test_write_text_file_fopen_failure(void);
void test_write_text_file_success(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_write_text_file_null_path(void) {
    TEST_ASSERT_FALSE(oidc_write_text_file(NULL, "content"));
}

void test_write_text_file_null_contents(void) {
    TEST_ASSERT_FALSE(oidc_write_text_file("/tmp/test.txt", NULL));
}

void test_write_text_file_fopen_failure(void) {
    TEST_ASSERT_FALSE(oidc_write_text_file("/nonexistent_dir_xyz/test.txt", "content"));
}

void test_write_text_file_success(void) {
    char template[] = "/tmp/oidc_write_test_XXXXXX";
    char *dir = mkdtemp(template);
    TEST_ASSERT_NOT_NULL(dir);

    char path[512];
    snprintf(path, sizeof(path), "%s/test.txt", dir);
    TEST_ASSERT_TRUE(oidc_write_text_file(path, "hello world"));

    FILE *fp = fopen(path, "r");
    TEST_ASSERT_NOT_NULL(fp);
    char buf[64] = {0};
    fread(buf, 1, sizeof(buf) - 1, fp);
    fclose(fp);
    TEST_ASSERT_EQUAL_STRING("hello world", buf);

    unlink(path);
    rmdir(dir);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_write_text_file_null_path);
    RUN_TEST(test_write_text_file_null_contents);
    RUN_TEST(test_write_text_file_fopen_failure);
    RUN_TEST(test_write_text_file_success);
    return UNITY_END();
}