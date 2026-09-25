/*
 * Unity Test File: firebird free prepared statement
 * Tests firebird_free_prepared_statement() — cleans up PreparedStatement
 * struct and its associated resources (sql_template, name).
 *
 * Build version: 1.0.0
 * TEST_VERSION: 20260925
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/firebird/types.h>
#include <src/database/firebird/prepared.h>

/* USE_MOCK_SYSTEM is passed by CMake for database/firebird test sources.
 * Declare the extern control functions so we can interact with mock state. */
extern void mock_system_reset_all(void);

/* Forward declaration for function under test */
void firebird_free_prepared_statement(PreparedStatement* stmt);

/* Function prototypes */
void test_free_null_stmt(void);
void test_free_with_sql_template_only(void);
void test_free_with_name_only(void);
void test_free_with_both_name_and_sql_template(void);
void test_free_with_neither_name_nor_sql_template(void);
void test_free_idempotent_after_manual_cleanup(void);

void setUp(void) {
    mock_system_reset_all();
}

void tearDown(void) {
    mock_system_reset_all();
}

/* ---- NULL safety (line 77-78) ---- */

void test_free_null_stmt(void) {
    firebird_free_prepared_statement(NULL);
    TEST_ASSERT_TRUE(true);
}

/* ---- Free with sql_template only (lines 81-83) ---- */

void test_free_with_sql_template_only(void) {
    PreparedStatement* stmt = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt);
    stmt->sql_template = strdup("SELECT * FROM users");

    firebird_free_prepared_statement(stmt);
    TEST_ASSERT_TRUE(true);
}

/* ---- Free with name only (lines 84-85) ---- */

void test_free_with_name_only(void) {
    PreparedStatement* stmt = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt);
    stmt->name = strdup("named_query");

    firebird_free_prepared_statement(stmt);
    TEST_ASSERT_TRUE(true);
}

/* ---- Free with both name and sql_template (lines 81-87) ---- */

void test_free_with_both_name_and_sql_template(void) {
    PreparedStatement* stmt = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt);
    stmt->sql_template = strdup("SELECT id, name FROM products WHERE id = ?");
    stmt->name = strdup("product_lookup");

    firebird_free_prepared_statement(stmt);
    TEST_ASSERT_TRUE(true);
}

/* ---- Free with neither name nor sql_template (lines 81-87, no frees taken) ---- */

void test_free_with_neither_name_nor_sql_template(void) {
    PreparedStatement* stmt = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt);
    /* Both name and sql_template are NULL from calloc */

    firebird_free_prepared_statement(stmt);
    TEST_ASSERT_TRUE(true);
}

/* ---- Verify free does not crash with partially populated stmt ---- */

void test_free_idempotent_after_manual_cleanup(void) {
    /* Verify that calling free on a stmt that has had its fields
       manually freed does not crash (simulates double-call safety) */
    PreparedStatement* stmt = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(stmt);
    stmt->sql_template = strdup("SELECT 1");
    stmt->name = strdup("test");

    free((void*)stmt->sql_template);
    free((void*)stmt->name);
    stmt->sql_template = NULL;
    stmt->name = NULL;

    firebird_free_prepared_statement(stmt);
    TEST_ASSERT_TRUE(true);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_free_null_stmt);
    RUN_TEST(test_free_with_sql_template_only);
    RUN_TEST(test_free_with_name_only);
    RUN_TEST(test_free_with_both_name_and_sql_template);
    RUN_TEST(test_free_with_neither_name_nor_sql_template);
    RUN_TEST(test_free_idempotent_after_manual_cleanup);

    return UNITY_END();
}
