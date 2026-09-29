/*
 * Unity Test File: Firebird Column Label Copy
 * Tests firebird_column_label_copy() — copies a column name from an XSQLVAR
 * (alias name, sql name, or "col" fallback), lowercased, into a buffer.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/firebird/types.h>
#include <src/database/firebird/query_internal.h>

void firebird_column_label_copy(const fb_xsqlvar_min* var, char* buf, size_t buflen);

void test_column_label_copy_null_var(void);
void test_column_label_copy_null_buf(void);
void test_column_label_copy_zero_buflen(void);
void test_column_label_copy_no_name(void);
void test_column_label_copy_aliasname(void);
void test_column_label_copy_sqlname(void);
void test_column_label_copy_aliasname_preferred_over_sqlname(void);
void test_column_label_copy_lowercase(void);
void test_column_label_copy_truncated(void);
void test_column_label_copy_long_aliasname(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_column_label_copy_null_var(void) {
    char buf[33] = "init";
    firebird_column_label_copy(NULL, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("", buf);
}

void test_column_label_copy_null_buf(void) {
    firebird_column_label_copy(NULL, NULL, 32);
}

void test_column_label_copy_zero_buflen(void) {
    fb_xsqlvar_min var = {0};
    char buf[33] = "init";
    firebird_column_label_copy(&var, buf, 0);
    TEST_ASSERT_EQUAL_STRING("init", buf);
}

void test_column_label_copy_no_name(void) {
    fb_xsqlvar_min var = {0};
    char buf[33] = "garbage";
    firebird_column_label_copy(&var, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("col", buf);
}

void test_column_label_copy_aliasname(void) {
    fb_xsqlvar_min var = {0};
    var.aliasname_length = 5;
    memcpy(var.aliasname, "MYCOL", 5);
    char buf[33] = "garbage";
    firebird_column_label_copy(&var, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("mycol", buf);
}

void test_column_label_copy_sqlname(void) {
    fb_xsqlvar_min var = {0};
    var.sqlname_length = 4;
    memcpy(var.sqlname, "data", 4);
    char buf[33] = "garbage";
    firebird_column_label_copy(&var, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("data", buf);
}

void test_column_label_copy_aliasname_preferred_over_sqlname(void) {
    fb_xsqlvar_min var = {0};
    var.aliasname_length = 5;
    memcpy(var.aliasname, "ALIAS", 5);
    var.sqlname_length = 5;
    memcpy(var.sqlname, "SQLNM", 5);
    char buf[33] = "garbage";
    firebird_column_label_copy(&var, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("alias", buf);
}

void test_column_label_copy_lowercase(void) {
    fb_xsqlvar_min var = {0};
    var.aliasname_length = 5;
    memcpy(var.aliasname, "AbCdE", 5);
    char buf[33] = "garbage";
    firebird_column_label_copy(&var, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("abcde", buf);
}

void test_column_label_copy_truncated(void) {
    fb_xsqlvar_min var = {0};
    var.aliasname_length = 32;
    memset(var.aliasname, 'A', 32);
    char buf[6] = "garb";
    firebird_column_label_copy(&var, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("aaaaa", buf);
}

void test_column_label_copy_long_aliasname(void) {
    fb_xsqlvar_min var = {0};
    var.aliasname_length = 20;
    const char* name = "VERY_LONG_NAME";
    memcpy(var.aliasname, name, strlen(name));
    char buf[33] = "garbage";
    firebird_column_label_copy(&var, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("very_long_name", buf);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_column_label_copy_null_var);
    RUN_TEST(test_column_label_copy_null_buf);
    RUN_TEST(test_column_label_copy_zero_buflen);
    RUN_TEST(test_column_label_copy_no_name);
    RUN_TEST(test_column_label_copy_aliasname);
    RUN_TEST(test_column_label_copy_sqlname);
    RUN_TEST(test_column_label_copy_aliasname_preferred_over_sqlname);
    RUN_TEST(test_column_label_copy_lowercase);
    RUN_TEST(test_column_label_copy_truncated);
    RUN_TEST(test_column_label_copy_long_aliasname);

    return UNITY_END();
}
