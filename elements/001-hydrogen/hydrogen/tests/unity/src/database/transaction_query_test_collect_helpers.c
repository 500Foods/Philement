/*
 * Unity Test File: transaction_query_test_collect_helpers
 * Unit tests for the result-collection and parameter-filtering helpers
 * used by the runtime transaction-block path
 * (src/database/dbqueue/transaction_query.c).
 */

#include <stdlib.h>
#include <string.h>

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/dbqueue/transaction_query.h>

void test_collector_create_and_free_null(void);
void test_collector_create_without_parameters(void);
void test_collect_entry_fields_and_totals(void);
void test_collect_entry_without_data_json(void);
void test_collect_entry_non_array_data_json(void);
void test_parameters_filter_skips_non_object_sections(void);
void test_parameters_filter_keeps_only_referenced(void);
void test_parameters_filter_without_references(void);
void test_parameters_filter_ignores_type_casts(void);
void test_parameters_filter_mixed_type_sections(void);
void test_parameters_filter_without_supplied_parameters(void);
void test_build_result_success_shape(void);
void test_build_result_failure_shape(void);
void test_filtered_parameters_returns_owned_copy(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_collector_create_and_free_null(void) {
    txn_collector_free(NULL);
    TEST_ASSERT_TRUE(true);
}

void test_collector_create_without_parameters(void) {
    BlockCollector* collector = txn_collector_create(NULL);
    TEST_ASSERT_NOT_NULL(collector);
    TEST_ASSERT_NULL(collector->parameters_root);
    TEST_ASSERT_NOT_NULL(collector->entries);
    TEST_ASSERT_EQUAL_size_t(0, json_array_size(collector->entries));
    txn_collector_free(collector);
}

void test_collect_entry_fields_and_totals(void) {
    BlockCollector* collector = txn_collector_create(NULL);
    TEST_ASSERT_NOT_NULL(collector);
    collector->current_block = 2;

    QueryResult result;
    memset(&result, 0, sizeof(result));
    result.success = true;
    result.data_json = strdup("[{\"a\":1}]");
    result.row_count = 1;
    result.column_count = 1;
    result.affected_rows = 4;

    TEST_ASSERT_TRUE(txn_collect_entry(collector, 1, &result));
    TEST_ASSERT_EQUAL_size_t(1, collector->total_rows);
    TEST_ASSERT_EQUAL_INT(4, collector->total_affected);
    TEST_ASSERT_EQUAL_size_t(1, collector->total_columns);
    TEST_ASSERT_EQUAL_size_t(1, json_array_size(collector->entries));

    json_t* entry = json_array_get(collector->entries, 0);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL_INT(2, json_integer_value(json_object_get(entry, "statement")));
    TEST_ASSERT_EQUAL_INT(2, json_integer_value(json_object_get(entry, "block")));
    TEST_ASSERT_EQUAL_INT(4, json_integer_value(json_object_get(entry, "affected")));
    TEST_ASSERT_TRUE(json_is_true(json_object_get(entry, "success")));
    TEST_ASSERT_TRUE(json_is_array(json_object_get(entry, "rows")));
    TEST_ASSERT_EQUAL_size_t(1, json_array_size(json_object_get(entry, "rows")));

    free(result.data_json);
    txn_collector_free(collector);
}

void test_collect_entry_without_data_json(void) {
    BlockCollector* collector = txn_collector_create(NULL);
    TEST_ASSERT_NOT_NULL(collector);

    QueryResult result;
    memset(&result, 0, sizeof(result));
    result.success = false;
    result.error_message = strdup("boom");
    result.error_class = DB_ERR_OTHER;

    TEST_ASSERT_TRUE(txn_collect_entry(collector, 0, &result));
    json_t* entry = json_array_get(collector->entries, 0);
    TEST_ASSERT_FALSE(json_is_true(json_object_get(entry, "success")));
    TEST_ASSERT_EQUAL_STRING("boom", json_string_value(json_object_get(entry, "error")));
    TEST_ASSERT_TRUE(json_is_array(json_object_get(entry, "rows")));
    TEST_ASSERT_EQUAL_size_t(0, json_array_size(json_object_get(entry, "rows")));

    free(result.error_message);
    txn_collector_free(collector);
}

void test_collect_entry_non_array_data_json(void) {
    BlockCollector* collector = txn_collector_create(NULL);
    TEST_ASSERT_NOT_NULL(collector);
    collector->current_block = 1;

    QueryResult result;
    memset(&result, 0, sizeof(result));
    result.success = true;
    result.data_json = strdup("{\"not\":\"rows\"}");
    result.affected_rows = 1;

    TEST_ASSERT_TRUE(txn_collect_entry(collector, 0, &result));
    json_t* rows = json_object_get(json_array_get(collector->entries, 0), "rows");
    TEST_ASSERT_TRUE(json_is_array(rows));
    TEST_ASSERT_EQUAL_size_t(0, json_array_size(rows));

    free(result.data_json);
    txn_collector_free(collector);
}

void test_parameters_filter_skips_non_object_sections(void) {
    BlockCollector* collector = txn_collector_create("{\"INTEGER\":{\"A\":1},\"NOTE\":\"skip\"}");
    TEST_ASSERT_NOT_NULL(collector);

    const char* filtered = txn_parameters_callback(0, "INSERT INTO t VALUES (:A);", collector);
    TEST_ASSERT_EQUAL_STRING("{\"INTEGER\":{\"A\":1}}", filtered);

    txn_collector_free(collector);
}

void test_parameters_filter_keeps_only_referenced(void) {
    BlockCollector* collector = txn_collector_create("{\"INTEGER\":{\"A\":1,\"B\":2}}");
    TEST_ASSERT_NOT_NULL(collector);

    const char* filtered = txn_parameters_callback(0, "INSERT INTO t VALUES (:A);", collector);
    TEST_ASSERT_EQUAL_STRING("{\"INTEGER\":{\"A\":1}}", filtered);

    filtered = txn_parameters_callback(1, "UPDATE t SET b = :B WHERE a = :A;", collector);
    TEST_ASSERT_EQUAL_STRING("{\"INTEGER\":{\"A\":1,\"B\":2}}", filtered);

    txn_collector_free(collector);
}

void test_parameters_filter_without_references(void) {
    BlockCollector* collector = txn_collector_create("{\"INTEGER\":{\"A\":1}}");
    TEST_ASSERT_NOT_NULL(collector);

    const char* filtered = txn_parameters_callback(0, "DELETE FROM t;", collector);
    TEST_ASSERT_EQUAL_STRING("{}", filtered);

    txn_collector_free(collector);
}

void test_parameters_filter_ignores_type_casts(void) {
    BlockCollector* collector = txn_collector_create("{\"INTEGER\":{\"A\":1,\"B\":2}}");
    TEST_ASSERT_NOT_NULL(collector);

    // PostgreSQL "::" casts are not parameters.
    const char* filtered = txn_parameters_callback(0, "SELECT a::text, :B FROM t;", collector);
    TEST_ASSERT_EQUAL_STRING("{\"INTEGER\":{\"B\":2}}", filtered);

    txn_collector_free(collector);
}

void test_parameters_filter_mixed_type_sections(void) {
    BlockCollector* collector = txn_collector_create(
        "{\"INTEGER\":{\"SHOULD_FAIL\":1},\"STRING\":{\"RUN_LABEL\":\"run-1\"}}");
    TEST_ASSERT_NOT_NULL(collector);

    // A statement bound only the string keeps only the STRING section.
    const char* filtered = txn_parameters_callback(0, "SELECT * FROM t WHERE label = :RUN_LABEL;", collector);
    TEST_ASSERT_EQUAL_STRING("{\"STRING\":{\"RUN_LABEL\":\"run-1\"}}", filtered);

    // A statement referencing both names keeps both sections.
    filtered = txn_parameters_callback(1,
        "INSERT INTO t (id, label) VALUES (CASE WHEN :SHOULD_FAIL = 1 THEN 1 ELSE 2 END, :RUN_LABEL);",
        collector);
    TEST_ASSERT_EQUAL_STRING("{\"INTEGER\":{\"SHOULD_FAIL\":1},\"STRING\":{\"RUN_LABEL\":\"run-1\"}}", filtered);

    txn_collector_free(collector);
}

void test_parameters_filter_without_supplied_parameters(void) {
    BlockCollector* collector = txn_collector_create(NULL);
    TEST_ASSERT_NOT_NULL(collector);

    const char* filtered = txn_parameters_callback(0, "SELECT :A;", collector);
    TEST_ASSERT_EQUAL_STRING("{}", filtered);

    txn_collector_free(collector);
}

void test_build_result_success_shape(void) {
    BlockCollector* collector = txn_collector_create(NULL);
    TEST_ASSERT_NOT_NULL(collector);
    collector->current_block = 1;

    QueryResult result;
    memset(&result, 0, sizeof(result));
    result.success = true;
    result.data_json = strdup("[{\"a\":1}]");
    result.row_count = 2;
    result.column_count = 1;
    result.affected_rows = 5;
    TEST_ASSERT_TRUE(txn_collect_entry(collector, 0, &result));
    free(result.data_json);

    QueryResult* final_result = txn_build_result(collector, true, NULL);
    TEST_ASSERT_NOT_NULL(final_result);
    TEST_ASSERT_TRUE(final_result->success);
    TEST_ASSERT_EQUAL_INT(DB_ERR_NONE, final_result->error_class);
    TEST_ASSERT_EQUAL_size_t(2, final_result->row_count);
    TEST_ASSERT_EQUAL_INT(5, final_result->affected_rows);
    TEST_ASSERT_NOT_NULL(final_result->data_json);

    json_error_t parse_error;
    json_t* parsed = json_loads(final_result->data_json, 0, &parse_error);
    TEST_ASSERT_NOT_NULL(parsed);
    TEST_ASSERT_TRUE(json_is_array(parsed));
    TEST_ASSERT_EQUAL_size_t(1, json_array_size(parsed));
    json_decref(parsed);

    database_engine_cleanup_result(final_result);
    txn_collector_free(collector);
}

void test_build_result_failure_shape(void) {
    BlockCollector* collector = txn_collector_create(NULL);
    TEST_ASSERT_NOT_NULL(collector);

    QueryResult* final_result = txn_build_result(collector, false, "transaction block 1 failed at statement 2");
    TEST_ASSERT_NOT_NULL(final_result);
    TEST_ASSERT_FALSE(final_result->success);
    TEST_ASSERT_EQUAL_INT(DB_ERR_OTHER, final_result->error_class);
    TEST_ASSERT_EQUAL_STRING("transaction block 1 failed at statement 2", final_result->error_message);
    TEST_ASSERT_EQUAL_STRING("[]", final_result->data_json);

    database_engine_cleanup_result(final_result);
    txn_collector_free(collector);
}

void test_filtered_parameters_returns_owned_copy(void) {
    BlockCollector* collector = txn_collector_create("{\"INTEGER\":{\"A\":1}}");
    TEST_ASSERT_NOT_NULL(collector);

    char* filtered = txn_filtered_parameters(collector, "SELECT :A;");
    TEST_ASSERT_NOT_NULL(filtered);
    TEST_ASSERT_EQUAL_STRING("{\"INTEGER\":{\"A\":1}}", filtered);
    free(filtered);

    txn_collector_free(collector);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_collector_create_and_free_null);
    RUN_TEST(test_collector_create_without_parameters);
    RUN_TEST(test_collect_entry_fields_and_totals);
    RUN_TEST(test_collect_entry_without_data_json);
    RUN_TEST(test_collect_entry_non_array_data_json);
    RUN_TEST(test_parameters_filter_skips_non_object_sections);
    RUN_TEST(test_parameters_filter_keeps_only_referenced);
    RUN_TEST(test_parameters_filter_without_references);
    RUN_TEST(test_parameters_filter_ignores_type_casts);
    RUN_TEST(test_parameters_filter_mixed_type_sections);
    RUN_TEST(test_parameters_filter_without_supplied_parameters);
    RUN_TEST(test_build_result_success_shape);
    RUN_TEST(test_build_result_failure_shape);
    RUN_TEST(test_filtered_parameters_returns_owned_copy);

    return UNITY_END();
}
