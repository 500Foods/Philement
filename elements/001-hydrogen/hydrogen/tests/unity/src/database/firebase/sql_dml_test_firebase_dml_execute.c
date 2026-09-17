/*
 * Unity tests for firebase_dml_execute() dispatch and helpers.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <jansson.h>
#include <src/database/database.h>
#include <src/database/firebase/sql_parse.h>
#include <src/database/firebase/sql_expr.h>
#include <src/database/firebase/sql_dml.h>

void test_firebase_dml_execute_null(void);
void test_firebase_dml_document_id_and_match(void);
void test_firebase_dml_rfc3339(void);
void release_query_result(QueryResult* result);

void release_query_result(QueryResult* result) {
    if (!result) {
        return;
    }
    free(result->error_message);
    free(result->data_json);
    free(result);
}

void setUp(void) {
}

void tearDown(void) {
}

void test_firebase_dml_execute_null(void) {
    QueryResult* result = NULL;
    TEST_ASSERT_FALSE(firebase_dml_execute(NULL, NULL, &result));
    TEST_ASSERT_FALSE(result->success);
    release_query_result(result);

    FirebaseSqlStatement* stmt = firebase_sql_parse("CREATE TABLE t (id integer)");
    result = NULL;
    TEST_ASSERT_FALSE(firebase_dml_execute(NULL, stmt, &result));
    TEST_ASSERT_NOT_NULL(strstr(result->error_message, "not a DML"));
    release_query_result(result);
    firebase_sql_statement_free(stmt);
}

void test_firebase_dml_document_id_and_match(void) {
    json_t* pk = json_loads("[\"lookup_id\",\"key_idx\"]", 0, NULL);
    json_t* parts = json_object();
    json_object_set_new(parts, "lookup_id", json_string("30"));
    json_object_set_new(parts, "key_idx", json_string("6"));
    char* id = firebase_dml_document_id(pk, parts);
    TEST_ASSERT_EQUAL_STRING("30_6", id);
    free(id);
    json_decref(pk);
    json_decref(parts);
    TEST_ASSERT_NULL(firebase_dml_document_id(NULL, NULL));

    json_t* pk2 = json_array();
    json_array_append_new(pk2, json_string("a"));
    json_array_append_new(pk2, json_string("b"));
    json_t* parts2 = json_object();
    json_object_set_new(parts2, "a", json_string("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"));
    json_object_set_new(parts2, "b", json_string("yyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy"));
    char* long_id = firebase_dml_document_id(pk2, parts2);
    TEST_ASSERT_NOT_NULL(long_id);
    TEST_ASSERT_TRUE(strlen(long_id) > 64);
    free(long_id);
    json_decref(pk2);
    json_decref(parts2);

    TEST_ASSERT_NULL(firebase_dml_sql_datetime_to_rfc3339("not-a-date"));
    TEST_ASSERT_FALSE(firebase_dml_lookup_field(NULL, NULL, NULL));
    TEST_ASSERT_FALSE(firebase_dml_where_match(NULL, NULL, NULL));

    FirebaseSqlStatement empty_upd = {0};
    empty_upd.kind = FIREBASE_SQL_KIND_UPDATE;
    empty_upd.update.table = (char*)"t";
    QueryResult* r2 = NULL;
    TEST_ASSERT_FALSE(firebase_dml_update(NULL, &empty_upd, &r2));
    release_query_result(r2);
    FirebaseSqlStatement empty_del = {0};
    empty_del.kind = FIREBASE_SQL_KIND_DELETE;
    empty_del.del.table = (char*)"t";
    r2 = NULL;
    TEST_ASSERT_FALSE(firebase_dml_delete(NULL, &empty_del, &r2));
    release_query_result(r2);

    FirebaseValue a;
    FirebaseValue b;
    firebase_value_init(&a);
    firebase_value_init(&b);
    firebase_value_set_int(&a, 30);
    firebase_value_copy_text(&b, "30");
    TEST_ASSERT_TRUE(firebase_dml_values_match(&a, &b));
    firebase_value_set_int(&b, 6);
    TEST_ASSERT_FALSE(firebase_dml_values_match(&a, &b));
    firebase_value_free(&a);
    firebase_value_free(&b);

    FirebaseValue n;
    firebase_value_init(&n);
    char* frag = firebase_dml_id_fragment(&n);
    TEST_ASSERT_NULL(frag);
    firebase_value_set_int(&n, 30);
    frag = firebase_dml_id_fragment(&n);
    TEST_ASSERT_EQUAL_STRING("30", frag);
    free(frag);
    firebase_value_free(&n);
}

void test_firebase_dml_fields_and_unique(void);
void test_firebase_dml_rfc3339(void);

void test_firebase_dml_fields_and_unique(void) {
    FirebaseValue val;
    firebase_value_init(&val);
    firebase_value_set_null(&val);
    json_t* field = firebase_dml_value_to_field(&val, "text");
    TEST_ASSERT_NOT_NULL(json_object_get(field, "nullValue"));
    FirebaseValue back;
    TEST_ASSERT_TRUE(firebase_dml_firestore_field_to_value(field, &back));
    TEST_ASSERT_TRUE(firebase_value_is_null(&back));
    json_decref(field);
    firebase_value_free(&back);

    firebase_value_copy_text(&val, "2023-11-14 22:13:20");
    field = firebase_dml_value_to_field(&val, "timestamp");
    TEST_ASSERT_NOT_NULL(json_object_get(field, "timestampValue"));
    TEST_ASSERT_TRUE(firebase_dml_firestore_field_to_value(field, &back));
    TEST_ASSERT_EQUAL_STRING("2023-11-14T22:13:20Z", back.data);
    json_decref(field);
    firebase_value_free(&back);

    firebase_value_copy_text(&val, "hello");
    field = firebase_dml_value_to_field(&val, "text");
    TEST_ASSERT_TRUE(firebase_dml_firestore_field_to_value(field, &back));
    TEST_ASSERT_EQUAL_STRING("hello", back.data);
    json_decref(field);
    firebase_value_free(&back);

    firebase_value_set_double(&val, 1.25);
    field = firebase_dml_value_to_field(&val, "real");
    TEST_ASSERT_NOT_NULL(json_object_get(field, "doubleValue"));
    TEST_ASSERT_TRUE(firebase_dml_firestore_field_to_value(field, &back));
    TEST_ASSERT_EQUAL(FIREBASE_VAL_DOUBLE, back.kind);
    json_decref(field);
    firebase_value_free(&back);
    firebase_value_free(&val);

    TEST_ASSERT_TRUE(firebase_dml_firestore_field_to_value(NULL, &back));
    TEST_ASSERT_TRUE(firebase_value_is_null(&back));
    firebase_value_free(&back);

    json_t* keys = json_loads("[[\"code\"]]", 0, NULL);
    json_t* new_fields = json_object();
    json_object_set_new(new_fields, "code", json_object());
    json_object_set_new(json_object_get(new_fields, "code"), "stringValue", json_string("x"));
    json_t* docs = json_array();
    json_t* other = json_object();
    json_object_set_new(other, "code", json_object());
    json_object_set_new(json_object_get(other, "code"), "stringValue", json_string("y"));
    json_array_append_new(docs, other);
    TEST_ASSERT_FALSE(firebase_dml_unique_conflict(keys, new_fields, docs));
    json_object_set_new(json_object_get(other, "code"), "stringValue", json_string("x"));
    /* other was moved into docs; rebuild a matching doc */
    json_decref(docs);
    docs = json_array();
    json_t* match = json_object();
    json_t* code = json_object();
    json_object_set_new(code, "stringValue", json_string("x"));
    json_object_set_new(match, "code", code);
    json_array_append_new(docs, match);
    TEST_ASSERT_TRUE(firebase_dml_unique_conflict(keys, new_fields, docs));
    json_decref(keys);
    json_decref(new_fields);
    json_decref(docs);
}

void test_firebase_dml_rfc3339(void) {
    char* rfc = firebase_dml_sql_datetime_to_rfc3339("2023-11-14 22:13:20");
    TEST_ASSERT_EQUAL_STRING("2023-11-14T22:13:20Z", rfc);
    free(rfc);
    rfc = firebase_dml_sql_datetime_to_rfc3339("2023-11-14T22:13:20Z");
    TEST_ASSERT_EQUAL_STRING("2023-11-14T22:13:20Z", rfc);
    free(rfc);
    TEST_ASSERT_NULL(firebase_dml_sql_datetime_to_rfc3339(NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_dml_execute_null);
    RUN_TEST(test_firebase_dml_document_id_and_match);
    RUN_TEST(test_firebase_dml_fields_and_unique);
    RUN_TEST(test_firebase_dml_rfc3339);
    return UNITY_END();
}
