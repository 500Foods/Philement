/*
 * Unity Test File: query_result_cache
 *
 * Unit tests for the query-result cache module.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/dbqueue/query_result_cache.h>

// Forward declarations for test functions
void test_query_result_cache_create_destroy(void);
void test_query_result_cache_put_and_get(void);
void test_query_result_cache_miss_returns_false(void);
void test_query_result_cache_separate_params(void);
void test_query_result_cache_normalizes_params(void);
void test_query_result_cache_separate_templates(void);
void test_query_result_cache_clear(void);
void test_query_result_cache_entry_count(void);

static QueryResultCache* g_cache = NULL;

void setUp(void) {
    g_cache = query_result_cache_create(16, "test");
    TEST_ASSERT_NOT_NULL(g_cache);
}

void tearDown(void) {
    query_result_cache_destroy(g_cache);
    g_cache = NULL;
}

void test_query_result_cache_create_destroy(void) {
    TEST_ASSERT_EQUAL_size_t(0, query_result_cache_entry_count(g_cache));
}

void test_query_result_cache_put_and_get(void) {
    const char* data_json = "[{\"id\":1,\"name\":\"alice\"}]";

    bool put_result = query_result_cache_put(g_cache,
                                              "Acuranzo",
                                              "SELECT * FROM accounts WHERE id = :id",
                                              "{\"id\":1}",
                                              data_json,
                                              1, 2, 3, 42);
    TEST_ASSERT_TRUE(put_result);

    json_t* data = NULL;
    size_t row_count = 0;
    size_t column_count = 0;
    int affected_rows = 0;
    time_t execution_time_ms = 0;

    bool get_result = query_result_cache_get(g_cache,
                                               "Acuranzo",
                                               "SELECT * FROM accounts WHERE id = :id",
                                               "{\"id\":1}",
                                               &data,
                                               &row_count,
                                               &column_count,
                                               &affected_rows,
                                               &execution_time_ms);
    TEST_ASSERT_TRUE(get_result);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQUAL_size_t(1, row_count);
    TEST_ASSERT_EQUAL_size_t(2, column_count);
    TEST_ASSERT_EQUAL(3, affected_rows);
    TEST_ASSERT_EQUAL(42, (int)execution_time_ms);

    json_t* expected = json_loads(data_json, 0, NULL);
    TEST_ASSERT_NOT_NULL(expected);
    TEST_ASSERT_TRUE(json_equal(expected, data));
    json_decref(expected);
    json_decref(data);
}

void test_query_result_cache_miss_returns_false(void) {
    json_t* data = NULL;
    bool get_result = query_result_cache_get(g_cache,
                                             "Acuranzo",
                                             "SELECT 1",
                                             NULL,
                                             &data,
                                             NULL,
                                             NULL,
                                             NULL,
                                             NULL);
    TEST_ASSERT_FALSE(get_result);
    TEST_ASSERT_NULL(data);
}

void test_query_result_cache_separate_params(void) {
    const char* sql = "SELECT * FROM accounts WHERE id = :id";

    TEST_ASSERT_TRUE(query_result_cache_put(g_cache, "Acuranzo", sql, "{\"id\":1}",
                                             "[{\"id\":1}]", 1, 1, 0, 1));

    json_t* data = NULL;
    bool get_result = query_result_cache_get(g_cache, "Acuranzo", sql, "{\"id\":2}",
                                              &data, NULL, NULL, NULL, NULL);
    TEST_ASSERT_FALSE(get_result);
    TEST_ASSERT_NULL(data);
}

void test_query_result_cache_normalizes_params(void) {
    const char* sql = "SELECT * FROM accounts WHERE id = :id AND name = :name";
    const char* data_json = "[{\"id\":1,\"name\":\"alice\"}]";

    TEST_ASSERT_TRUE(query_result_cache_put(g_cache, "Acuranzo", sql,
                                             "{\"name\":\"alice\",\"id\":1}",
                                             data_json, 1, 2, 0, 1));

    json_t* data = NULL;
    bool get_result = query_result_cache_get(g_cache, "Acuranzo", sql,
                                              "{\"id\":1,\"name\":\"alice\"}",
                                              &data, NULL, NULL, NULL, NULL);
    TEST_ASSERT_TRUE(get_result);
    TEST_ASSERT_NOT_NULL(data);

    json_t* expected = json_loads(data_json, 0, NULL);
    TEST_ASSERT_NOT_NULL(expected);
    TEST_ASSERT_TRUE(json_equal(expected, data));
    json_decref(expected);
    json_decref(data);
}

void test_query_result_cache_separate_templates(void) {
    TEST_ASSERT_TRUE(query_result_cache_put(g_cache, "Acuranzo",
                                             "SELECT * FROM accounts",
                                             "{}",
                                             "[{\"id\":1}]", 1, 1, 0, 1));

    json_t* data = NULL;
    bool get_result = query_result_cache_get(g_cache, "Acuranzo",
                                              "SELECT * FROM users",
                                              "{}",
                                              &data, NULL, NULL, NULL, NULL);
    TEST_ASSERT_FALSE(get_result);
    TEST_ASSERT_NULL(data);
}

void test_query_result_cache_clear(void) {
    const char* sql = "SELECT 1";
    TEST_ASSERT_TRUE(query_result_cache_put(g_cache, "Acuranzo", sql, NULL,
                                             "[{\"1\":1}]", 1, 1, 0, 1));
    TEST_ASSERT_EQUAL_size_t(1, query_result_cache_entry_count(g_cache));

    query_result_cache_clear(g_cache);

    TEST_ASSERT_EQUAL_size_t(0, query_result_cache_entry_count(g_cache));

    json_t* data = NULL;
    TEST_ASSERT_FALSE(query_result_cache_get(g_cache, "Acuranzo", sql, NULL,
                                              &data, NULL, NULL, NULL, NULL));
    TEST_ASSERT_NULL(data);
}

void test_query_result_cache_entry_count(void) {
    TEST_ASSERT_TRUE(query_result_cache_put(g_cache, "Acuranzo",
                                             "SELECT * FROM a", NULL,
                                             "[{\"id\":1}]", 1, 1, 0, 1));
    TEST_ASSERT_TRUE(query_result_cache_put(g_cache, "Acuranzo",
                                             "SELECT * FROM b", NULL,
                                             "[{\"id\":2}]", 1, 1, 0, 1));
    TEST_ASSERT_TRUE(query_result_cache_put(g_cache, "Lithium",
                                             "SELECT * FROM a", NULL,
                                             "[{\"id\":3}]", 1, 1, 0, 1));

    TEST_ASSERT_EQUAL_size_t(3, query_result_cache_entry_count(g_cache));

    // Update an existing entry; count should not change.
    TEST_ASSERT_TRUE(query_result_cache_put(g_cache, "Acuranzo",
                                             "SELECT * FROM a", NULL,
                                             "[{\"id\":9}]", 1, 1, 0, 1));
    TEST_ASSERT_EQUAL_size_t(3, query_result_cache_entry_count(g_cache));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_query_result_cache_create_destroy);
    RUN_TEST(test_query_result_cache_put_and_get);
    RUN_TEST(test_query_result_cache_miss_returns_false);
    RUN_TEST(test_query_result_cache_separate_params);
    RUN_TEST(test_query_result_cache_normalizes_params);
    RUN_TEST(test_query_result_cache_separate_templates);
    RUN_TEST(test_query_result_cache_clear);
    RUN_TEST(test_query_result_cache_entry_count);

    return UNITY_END();
}
