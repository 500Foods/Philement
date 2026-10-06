/*
 * Unity Test File: query_result_cache_invalidate_template
 *
 * Drops every parameter variant of one SQL template in one database.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/dbqueue/query_result_cache.h>

static QueryResultCache* g_cache = NULL;

static const char* k_accounts = "SELECT id FROM accounts WHERE id = :id";
static const char* k_sessions = "SELECT id FROM sessions WHERE id = :id";

static bool put_row(const char* database, const char* sql, const char* params) {
    return query_result_cache_put(g_cache, database, sql, params,
                                  "[{\"id\":1}]", 1, 1, 0, 1);
}

static bool cached(const char* database, const char* sql, const char* params) {
    json_t* data = NULL;
    bool hit = query_result_cache_get(g_cache, database, sql, params,
                                       &data, NULL, NULL, NULL, NULL);

    json_decref(data);
    return hit;
}

void test_query_result_cache_invalidate_template_null(void);
void test_query_result_cache_invalidate_template_variants(void);
void test_query_result_cache_invalidate_template_colon_name(void);
void test_query_result_cache_invalidate_template_null_database(void);

void setUp(void) {
    g_cache = query_result_cache_create(16, "test");
    TEST_ASSERT_NOT_NULL(g_cache);
}

void tearDown(void) {
    query_result_cache_destroy(g_cache);
    g_cache = NULL;
}

void test_query_result_cache_invalidate_template_null(void) {
    TEST_ASSERT_TRUE(put_row("Acuranzo", k_accounts, "{\"id\":1}"));
    TEST_ASSERT_EQUAL_size_t(0, query_result_cache_invalidate_template(NULL, "Acuranzo", k_accounts));
    TEST_ASSERT_EQUAL_size_t(0, query_result_cache_invalidate_template(g_cache, "Acuranzo", NULL));
    TEST_ASSERT_EQUAL_size_t(1, query_result_cache_entry_count(g_cache));
    TEST_ASSERT_TRUE(cached("Acuranzo", k_accounts, "{\"id\":1}"));
}

void test_query_result_cache_invalidate_template_variants(void) {
    TEST_ASSERT_TRUE(put_row("Acuranzo", k_accounts, "{\"id\":1}"));
    TEST_ASSERT_TRUE(put_row("Acuranzo", k_accounts, "{\"id\":2}"));
    TEST_ASSERT_TRUE(put_row("Acuranzo", k_sessions, "{\"id\":1}"));
    TEST_ASSERT_TRUE(put_row("OtherDB", k_accounts, "{\"id\":1}"));
    {
        size_t removed = query_result_cache_invalidate_template(g_cache, "Acuranzo", k_accounts);

        TEST_ASSERT_EQUAL_size_t(2, removed);
        TEST_ASSERT_EQUAL_size_t(2, query_result_cache_entry_count(g_cache));
        TEST_ASSERT_FALSE(cached("Acuranzo", k_accounts, "{\"id\":1}"));
        TEST_ASSERT_FALSE(cached("Acuranzo", k_accounts, "{\"id\":2}"));
        TEST_ASSERT_TRUE(cached("Acuranzo", k_sessions, "{\"id\":1}"));
        TEST_ASSERT_TRUE(cached("OtherDB", k_accounts, "{\"id\":1}"));
    }
}

void test_query_result_cache_invalidate_template_colon_name(void) {
    TEST_ASSERT_TRUE(put_row("a:b", k_accounts, "{\"id\":1}"));
    TEST_ASSERT_TRUE(put_row("a", k_accounts, "{\"id\":1}"));
    TEST_ASSERT_EQUAL_size_t(1, query_result_cache_invalidate_template(g_cache, "a", k_accounts));
    TEST_ASSERT_FALSE(cached("a", k_accounts, "{\"id\":1}"));
    TEST_ASSERT_TRUE(cached("a:b", k_accounts, "{\"id\":1}"));
    TEST_ASSERT_EQUAL_size_t(1, query_result_cache_invalidate_template(g_cache, "a:b", k_accounts));
    TEST_ASSERT_FALSE(cached("a:b", k_accounts, "{\"id\":1}"));
    TEST_ASSERT_EQUAL_size_t(0, query_result_cache_entry_count(g_cache));
}

void test_query_result_cache_invalidate_template_null_database(void) {
    TEST_ASSERT_TRUE(put_row(NULL, k_accounts, "{\"id\":1}"));
    TEST_ASSERT_TRUE(put_row("Acuranzo", k_accounts, "{\"id\":1}"));
    TEST_ASSERT_EQUAL_size_t(1, query_result_cache_invalidate_template(g_cache, NULL, k_accounts));
    TEST_ASSERT_FALSE(cached(NULL, k_accounts, "{\"id\":1}"));
    TEST_ASSERT_TRUE(cached("Acuranzo", k_accounts, "{\"id\":1}"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_query_result_cache_invalidate_template_null);
    RUN_TEST(test_query_result_cache_invalidate_template_variants);
    RUN_TEST(test_query_result_cache_invalidate_template_colon_name);
    RUN_TEST(test_query_result_cache_invalidate_template_null_database);
    return UNITY_END();
}
