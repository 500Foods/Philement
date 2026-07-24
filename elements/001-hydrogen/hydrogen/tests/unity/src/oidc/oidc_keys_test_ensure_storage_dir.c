/*
 * Unity Test File: oidc_ensure_storage_dir
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_keys.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

void test_ensure_storage_dir_null_path(void);
void test_ensure_storage_dir_empty_path(void);
void test_ensure_storage_dir_existing_dir(void);
void test_ensure_storage_dir_create_new_dir(void);
void test_ensure_storage_dir_path_is_file(void);
void test_ensure_storage_dir_create_fails(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_ensure_storage_dir_null_path(void) {
    TEST_ASSERT_FALSE(oidc_ensure_storage_dir(NULL));
}

void test_ensure_storage_dir_empty_path(void) {
    TEST_ASSERT_FALSE(oidc_ensure_storage_dir(""));
}

void test_ensure_storage_dir_existing_dir(void) {
    char template[] = "/tmp/oidc_ensure_dir_XXXXXX";
    char *dir = mkdtemp(template);
    TEST_ASSERT_NOT_NULL(dir);
    TEST_ASSERT_TRUE(oidc_ensure_storage_dir(dir));
    rmdir(dir);
}

void test_ensure_storage_dir_create_new_dir(void) {
    char template[] = "/tmp/oidc_ensure_new_XXXXXX";
    char *dir = mkdtemp(template);
    TEST_ASSERT_NOT_NULL(dir);

    char path[512];
    snprintf(path, sizeof(path), "%s/new_subdir", dir);
    TEST_ASSERT_TRUE(oidc_ensure_storage_dir(path));

    struct stat st;
    TEST_ASSERT_EQUAL_INT(0, stat(path, &st));
    TEST_ASSERT_TRUE(S_ISDIR(st.st_mode));

    rmdir(path);
    rmdir(dir);
}

void test_ensure_storage_dir_path_is_file(void) {
    char template[] = "/tmp/oidc_ensure_file_XXXXXX";
    char *dir = mkdtemp(template);
    TEST_ASSERT_NOT_NULL(dir);

    char path[512];
    snprintf(path, sizeof(path), "%s/not_a_dir", dir);
    FILE *fp = fopen(path, "w");
    TEST_ASSERT_NOT_NULL(fp);
    fputs("x", fp);
    fclose(fp);

    TEST_ASSERT_FALSE(oidc_ensure_storage_dir(path));

    unlink(path);
    rmdir(dir);
}

void test_ensure_storage_dir_create_fails(void) {
    char path[512];
    snprintf(path, sizeof(path), "/proc/oidc_test_dir_no_create");
    TEST_ASSERT_FALSE(oidc_ensure_storage_dir(path));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ensure_storage_dir_null_path);
    RUN_TEST(test_ensure_storage_dir_empty_path);
    RUN_TEST(test_ensure_storage_dir_existing_dir);
    RUN_TEST(test_ensure_storage_dir_create_new_dir);
    RUN_TEST(test_ensure_storage_dir_path_is_file);
    RUN_TEST(test_ensure_storage_dir_create_fails);
    return UNITY_END();
}
