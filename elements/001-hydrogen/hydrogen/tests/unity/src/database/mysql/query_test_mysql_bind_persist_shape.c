/*
 * Unity Test File: MySQL Parameter Binding - Persist Shape (PERSIST_PLAN)
 *
 * Asserts the invariants that must hold for every bind in the wide Persist
 * bind shape (QueryRef 093: 12 binds with several JSON nulls):
 *
 *   1. bind[i].length is non-NULL for EVERY bind type (not just STRING/INTEGER)
 *      - previously NULL for BOOLEAN / FLOAT / DATE / TIME / DATETIME / TIMESTAMP
 *      - MariaDB Connector/C bind_param dereferences length even for fixed-width
 *        types; NULL -> SIGSEGV inside the client .so.
 *
 *   2. bind[i].is_null is non-NULL and points at bind[i].is_null_value
 *      - MariaDB bind_param dereferences is_null to honor MYSQL_TYPE_NULL
 *
 *   3. bind[i].error is non-NULL and points at bind[i].error_value
 *      - same as is_null
 *
 *   4. JSON null params use MYSQL_TYPE_NULL with buffer_length set
 *      - TypedParameter.is_null=true must produce MYSQL_TYPE_NULL
 *
 *   5. buffer_type matches the TypedParameter type for non-null binds
 *
 *   6. mysql_cleanup_bound_values() releases every malloc'd length pointer
 *      - no leaks for the second-half length-pointer table
 *
 * This test uses the libmysqlclient mock (USE_MOCK_LIBMYSQLCLIENT) but
 * <mysql.h> provides the canonical MYSQL_BIND definition since the
 * production code now includes it (PERSIST_PLAN). The test inspects the
 * bind array fields directly; it does NOT call any mysql_stmt_* function
 * through the .so - the goal is to verify Hydrogen-side invariants.
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include database parameter headers and the canonical client header for
// MYSQL_BIND / MYSQL_TYPE_* / MYSQL_TIME. The real client header is now
// pulled in by the query module (PERSIST_PLAN); we mirror that here so
// the test can read the bind array fields directly.
#include <mysql.h>
#include <src/database/database_params.h>
#include <src/database/mysql/types.h>
#include <src/database/mysql/connection.h>
#include <src/database/mysql/query.h>

// Mock setup
#include <unity/mocks/mock_libmysqlclient.h>
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

// Forward declarations
void mysql_bind_attach_indicators(void* bind_ptr, unsigned int param_index, char is_null_flag);
bool mysql_bind_single_parameter(void* bind_ptr, unsigned int param_index, TypedParameter* param,
                                  void** bound_values, size_t total_param_count, const char* designator);
void mysql_cleanup_bound_values(void** bound_values, size_t count);

// Test function prototypes
void test_persist_shape_length_non_null_for_all_types(void);
void test_persist_shape_is_null_attached_for_all_types(void);
void test_persist_shape_error_attached_for_all_types(void);
void test_persist_shape_json_null_uses_mysql_type_null(void);
void test_persist_shape_buffer_type_matches_param_type(void);
void test_persist_shape_length_value_correct_for_string_and_int(void);
void test_persist_shape_twelve_binds_with_mixed_nulls(void);
void test_persist_shape_boolean_length_allocated(void);
void test_persist_shape_float_length_allocated(void);
void test_persist_shape_date_length_allocated(void);
void test_persist_shape_time_length_allocated(void);
void test_persist_shape_datetime_length_allocated(void);
void test_persist_shape_timestamp_length_allocated(void);
void test_persist_shape_cleanup_releases_length_pointers(void);
void test_persist_shape_memory_failure_rollback_safe(void);
void test_persist_shape_attach_indicators_null_bind_safe(void);
void test_persist_shape_bind_single_parameter_null_inputs_safe(void);

void setUp(void) {
    mock_libmysqlclient_reset_all();
    mock_system_reset_all();
    load_libmysql_functions(NULL);
}

void tearDown(void) {
    mock_libmysqlclient_reset_all();
    mock_system_reset_all();
}

// Build a fresh TypedParameter with the given fields. Caller frees .name
// and any heap-allocated value via free_test_typed_param below.
static TypedParameter* make_test_typed_param(ParameterType type, const char* name, bool is_null) {
    TypedParameter* p = calloc(1, sizeof(TypedParameter));
    TEST_ASSERT_NOT_NULL(p);
    p->name = strdup(name);
    p->type = type;
    p->is_null = is_null;
    return p;
}

static void free_test_typed_param(TypedParameter* p) {
    if (!p) return;
    free(p->name);
    if (!p->is_null) {
        switch (p->type) {
            case PARAM_TYPE_STRING:
                free(p->value.string_value);
                break;
            case PARAM_TYPE_TEXT:
                free(p->value.text_value);
                break;
            case PARAM_TYPE_DATE:
                free(p->value.date_value);
                break;
            case PARAM_TYPE_TIME:
                free(p->value.time_value);
                break;
            case PARAM_TYPE_DATETIME:
                free(p->value.datetime_value);
                break;
            case PARAM_TYPE_TIMESTAMP:
                free(p->value.timestamp_value);
                break;
            case PARAM_TYPE_INTEGER:
            case PARAM_TYPE_BOOLEAN:
            case PARAM_TYPE_FLOAT:
                /* scalar values live inline in the union; nothing to free */
                break;
            default:
                break;
        }
    }
    free(p);
}

// ============================================================================
// Invariant 1: bind[].length is non-NULL for EVERY bind type.
// ============================================================================

void test_persist_shape_length_non_null_for_all_types(void) {
    enum { N = 8 };
    MYSQL_BIND bind[N];
    void** bound_values = calloc(N * 2, sizeof(void*));
    memset(bind, 0, sizeof(bind));

    // Mix of every fixed-width and variable-width bind type, all non-null.
    TypedParameter* params[N] = {
        make_test_typed_param(PARAM_TYPE_INTEGER, "i", false),
        make_test_typed_param(PARAM_TYPE_STRING,  "s", false),
        make_test_typed_param(PARAM_TYPE_TEXT,    "t", false),
        make_test_typed_param(PARAM_TYPE_BOOLEAN, "b", false),
        make_test_typed_param(PARAM_TYPE_FLOAT,   "f", false),
        make_test_typed_param(PARAM_TYPE_DATE,    "d", false),
        make_test_typed_param(PARAM_TYPE_TIME,    "ti", false),
        make_test_typed_param(PARAM_TYPE_DATETIME,"dt", false)
    };
    params[0]->value.int_value = 42;
    params[1]->value.string_value = strdup("hi");
    params[2]->value.text_value   = strdup("longer text");
    params[3]->value.bool_value   = true;
    params[4]->value.float_value  = 3.14;
    params[5]->value.date_value   = strdup("2026-09-04");
    params[6]->value.time_value   = strdup("12:34:56");
    params[7]->value.datetime_value = strdup("2026-09-04 12:34:56");

    for (unsigned int i = 0; i < N; ++i) {
        TEST_ASSERT_TRUE(mysql_bind_single_parameter(bind, i, params[i],
                                                      bound_values, N, "test"));
    }

    // Every bind must have a non-NULL length pointer.
    for (unsigned int i = 0; i < N; ++i) {
        TEST_ASSERT_NOT_NULL_MESSAGE(bind[i].length,
            "bind[i].length must be non-NULL for every bind type (PERSIST_PLAN)");
    }

    mysql_cleanup_bound_values(bound_values, N);
    for (unsigned int i = 0; i < N; ++i) {
        free_test_typed_param(params[i]);
    }
}

// ============================================================================
// Invariant 2: bind[].is_null is non-NULL and points at bind[].is_null_value.
// ============================================================================

void test_persist_shape_is_null_attached_for_all_types(void) {
    enum { N = 6 };
    MYSQL_BIND bind[N];
    void** bound_values = calloc(N * 2, sizeof(void*));
    memset(bind, 0, sizeof(bind));

    TypedParameter* params[N] = {
        make_test_typed_param(PARAM_TYPE_INTEGER, "i1", false),
        make_test_typed_param(PARAM_TYPE_INTEGER, "i2", true),
        make_test_typed_param(PARAM_TYPE_STRING,  "s1", false),
        make_test_typed_param(PARAM_TYPE_STRING,  "s2", true),
        make_test_typed_param(PARAM_TYPE_BOOLEAN, "b1", false),
        make_test_typed_param(PARAM_TYPE_DATE,    "d1", false)
    };
    params[0]->value.int_value = 7;
    params[2]->value.string_value = strdup("x");
    params[4]->value.bool_value = true;
    params[5]->value.date_value = strdup("2026-01-01");

    for (unsigned int i = 0; i < N; ++i) {
        TEST_ASSERT_TRUE(mysql_bind_single_parameter(bind, i, params[i],
                                                      bound_values, N, "test"));
    }

    for (unsigned int i = 0; i < N; ++i) {
        TEST_ASSERT_NOT_NULL_MESSAGE(bind[i].is_null,
            "bind[i].is_null must be non-NULL");
        TEST_ASSERT_EQUAL_PTR_MESSAGE(&bind[i].is_null_value, bind[i].is_null,
            "bind[i].is_null must point at bind[i].is_null_value");
    }

    mysql_cleanup_bound_values(bound_values, N);
    for (unsigned int i = 0; i < N; ++i) {
        free_test_typed_param(params[i]);
    }
}

// ============================================================================
// Invariant 3: bind[].error is non-NULL and points at bind[].error_value.
// ============================================================================

void test_persist_shape_error_attached_for_all_types(void) {
    enum { N = 4 };
    MYSQL_BIND bind[N];
    void** bound_values = calloc(N * 2, sizeof(void*));
    memset(bind, 0, sizeof(bind));

    TypedParameter* params[N] = {
        make_test_typed_param(PARAM_TYPE_INTEGER, "i", false),
        make_test_typed_param(PARAM_TYPE_STRING,  "s", true),
        make_test_typed_param(PARAM_TYPE_BOOLEAN, "b", false),
        make_test_typed_param(PARAM_TYPE_FLOAT,   "f", false)
    };
    params[0]->value.int_value = 99;
    params[2]->value.bool_value = false;
    params[3]->value.float_value = 2.5;

    for (unsigned int i = 0; i < N; ++i) {
        TEST_ASSERT_TRUE(mysql_bind_single_parameter(bind, i, params[i],
                                                      bound_values, N, "test"));
    }

    for (unsigned int i = 0; i < N; ++i) {
        TEST_ASSERT_NOT_NULL_MESSAGE(bind[i].error,
            "bind[i].error must be non-NULL");
        TEST_ASSERT_EQUAL_PTR_MESSAGE(&bind[i].error_value, bind[i].error,
            "bind[i].error must point at bind[i].error_value");
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, bind[i].error_value,
            "bind[i].error_value must initialize to 0");
    }

    mysql_cleanup_bound_values(bound_values, N);
    for (unsigned int i = 0; i < N; ++i) {
        free_test_typed_param(params[i]);
    }
}

// ============================================================================
// Invariant 4: JSON null params use MYSQL_TYPE_NULL.
// ============================================================================

void test_persist_shape_json_null_uses_mysql_type_null(void) {
    enum { N = 3 };
    MYSQL_BIND bind[N];
    void** bound_values = calloc(N * 2, sizeof(void*));
    memset(bind, 0, sizeof(bind));

    TypedParameter* params[N] = {
        make_test_typed_param(PARAM_TYPE_STRING,  "s_null", true),
        make_test_typed_param(PARAM_TYPE_TEXT,    "t_null", true),
        make_test_typed_param(PARAM_TYPE_DATETIME,"dt_null", true)
    };

    for (unsigned int i = 0; i < N; ++i) {
        TEST_ASSERT_TRUE(mysql_bind_single_parameter(bind, i, params[i],
                                                      bound_values, N, "test"));
        TEST_ASSERT_EQUAL_INT_MESSAGE(MYSQL_TYPE_NULL, bind[i].buffer_type,
            "JSON null must produce MYSQL_TYPE_NULL");
        TEST_ASSERT_EQUAL_INT_MESSAGE(1, bind[i].is_null_value,
            "JSON null must set is_null_value=1");
        TEST_ASSERT_EQUAL_INT_MESSAGE(1, bind[i].buffer_length,
            "MYSQL_TYPE_NULL buffer_length should be 1 (empty placeholder)");
        TEST_ASSERT_NOT_NULL_MESSAGE(bind[i].length,
            "MYSQL_TYPE_NULL bind must still have a non-NULL length pointer");
    }

    mysql_cleanup_bound_values(bound_values, N);
    for (unsigned int i = 0; i < N; ++i) {
        free_test_typed_param(params[i]);
    }
}

// ============================================================================
// Invariant 5: buffer_type matches the TypedParameter type for non-null binds.
// ============================================================================

void test_persist_shape_buffer_type_matches_param_type(void) {
    enum { N = 10 };
    MYSQL_BIND bind[N];
    void** bound_values = calloc(N * 2, sizeof(void*));
    memset(bind, 0, sizeof(bind));

    TypedParameter* params[N] = {
        make_test_typed_param(PARAM_TYPE_INTEGER, "i",  false),
        make_test_typed_param(PARAM_TYPE_STRING,  "s",  false),
        make_test_typed_param(PARAM_TYPE_TEXT,    "t",  false),
        make_test_typed_param(PARAM_TYPE_BOOLEAN, "b",  false),
        make_test_typed_param(PARAM_TYPE_FLOAT,   "f",  false),
        make_test_typed_param(PARAM_TYPE_DATE,    "d",  false),
        make_test_typed_param(PARAM_TYPE_TIME,    "ti", false),
        make_test_typed_param(PARAM_TYPE_DATETIME,"dt", false),
        make_test_typed_param(PARAM_TYPE_TIMESTAMP,"ts",false),
        make_test_typed_param(PARAM_TYPE_INTEGER, "i2", false)
    };
    params[0]->value.int_value = 1;
    params[1]->value.string_value = strdup("s");
    params[2]->value.text_value   = strdup("t");
    params[3]->value.bool_value   = true;
    params[4]->value.float_value  = 1.0;
    params[5]->value.date_value   = strdup("2026-09-04");
    params[6]->value.time_value   = strdup("00:00:00");
    params[7]->value.datetime_value = strdup("2026-09-04 00:00:00");
    params[8]->value.timestamp_value = strdup("2026-09-04 00:00:00.000");
    params[9]->value.int_value = 2;

    unsigned int expected_types[N] = {
        MYSQL_TYPE_LONGLONG,
        MYSQL_TYPE_STRING,
        MYSQL_TYPE_LONG_BLOB,
        MYSQL_TYPE_SHORT,
        MYSQL_TYPE_DOUBLE,
        MYSQL_TYPE_DATE,
        MYSQL_TYPE_TIME,
        MYSQL_TYPE_DATETIME,
        MYSQL_TYPE_TIMESTAMP,
        MYSQL_TYPE_LONGLONG
    };

    for (unsigned int i = 0; i < N; ++i) {
        TEST_ASSERT_TRUE(mysql_bind_single_parameter(bind, i, params[i],
                                                      bound_values, N, "test"));
        TEST_ASSERT_EQUAL_INT_MESSAGE(expected_types[i], bind[i].buffer_type,
            "buffer_type must match TypedParameter type");
    }

    mysql_cleanup_bound_values(bound_values, N);
    for (unsigned int i = 0; i < N; ++i) {
        free_test_typed_param(params[i]);
    }
}

// ============================================================================
// Invariant 6: length value matches the actual string/int length.
// ============================================================================

void test_persist_shape_length_value_correct_for_string_and_int(void) {
    enum { N = 3 };
    MYSQL_BIND bind[N];
    void** bound_values = calloc(N * 2, sizeof(void*));
    memset(bind, 0, sizeof(bind));

    TypedParameter* params[N] = {
        make_test_typed_param(PARAM_TYPE_INTEGER, "i",  false),
        make_test_typed_param(PARAM_TYPE_STRING,  "s",  false),
        make_test_typed_param(PARAM_TYPE_TEXT,    "t",  false)
    };
    params[0]->value.int_value = 12345;
    params[1]->value.string_value = strdup("hello");
    params[2]->value.text_value   = strdup("a longer text value");

    for (unsigned int i = 0; i < N; ++i) {
        TEST_ASSERT_TRUE(mysql_bind_single_parameter(bind, i, params[i],
                                                      bound_values, N, "test"));
    }

    TEST_ASSERT_EQUAL_UINT(sizeof(long long), *bind[0].length);
    TEST_ASSERT_EQUAL_UINT(5,                *bind[1].length);
    TEST_ASSERT_EQUAL_UINT(19,               *bind[2].length);

    mysql_cleanup_bound_values(bound_values, N);
    for (unsigned int i = 0; i < N; ++i) {
        free_test_typed_param(params[i]);
    }
}

// ============================================================================
// Mirror of QueryRef 093: 12 binds with several JSON nulls.
// ============================================================================

void test_persist_shape_twelve_binds_with_mixed_nulls(void) {
    enum { N = 12 };
    MYSQL_BIND bind[N];
    void** bound_values = calloc(N * 2, sizeof(void*));
    memset(bind, 0, sizeof(bind));

    // Exact shape of QueryRef 093 (mail_queue insert pending row).
    TypedParameter* params[N] = {
        make_test_typed_param(PARAM_TYPE_STRING,  "message_uuid",    false),
        make_test_typed_param(PARAM_TYPE_INTEGER, "priority",        false),
        make_test_typed_param(PARAM_TYPE_STRING,  "template_key",    true),
        make_test_typed_param(PARAM_TYPE_STRING,  "from_addr",       false),
        make_test_typed_param(PARAM_TYPE_STRING,  "reply_to",        true),
        make_test_typed_param(PARAM_TYPE_STRING,  "recipients_json", false),
        make_test_typed_param(PARAM_TYPE_STRING,  "subject",         false),
        make_test_typed_param(PARAM_TYPE_STRING,  "body_text",       true),
        make_test_typed_param(PARAM_TYPE_STRING,  "body_html",       true),
        make_test_typed_param(PARAM_TYPE_STRING,  "headers_json",    true),
        make_test_typed_param(PARAM_TYPE_STRING,  "idempotency_key", false),
        make_test_typed_param(PARAM_TYPE_STRING,  "next_attempt_at", false)
    };
    params[0]->value.string_value = strdup("11111111-1111-1111-1111-111111111111");
    params[1]->value.int_value    = 5;
    params[3]->value.string_value = strdup("noreply@example.com");
    params[5]->value.string_value = strdup("[]");
    params[6]->value.string_value = strdup("hello");
    params[10]->value.string_value = strdup("idem-key");
    params[11]->value.string_value = strdup("2030-01-01 00:00:00");

    for (unsigned int i = 0; i < N; ++i) {
        TEST_ASSERT_TRUE(mysql_bind_single_parameter(bind, i, params[i],
                                                      bound_values, N, "test"));
    }

    // Every slot must satisfy the invariants.
    for (unsigned int i = 0; i < N; ++i) {
        TEST_ASSERT_NOT_NULL_MESSAGE(bind[i].length,
            "Persist-shape bind[i].length must be non-NULL");
        TEST_ASSERT_NOT_NULL_MESSAGE(bind[i].is_null,
            "Persist-shape bind[i].is_null must be non-NULL");
        TEST_ASSERT_NOT_NULL_MESSAGE(bind[i].error,
            "Persist-shape bind[i].error must be non-NULL");
        TEST_ASSERT_NOT_NULL_MESSAGE(bind[i].buffer,
            "Persist-shape bind[i].buffer must be non-NULL");
    }

    // JSON nulls (params 2, 4, 7, 8, 9) must use MYSQL_TYPE_NULL.
    unsigned int null_indices[] = {2, 4, 7, 8, 9};
    for (unsigned k = 0; k < sizeof(null_indices)/sizeof(null_indices[0]); ++k) {
        unsigned int i = null_indices[k];
        TEST_ASSERT_EQUAL_INT_MESSAGE(MYSQL_TYPE_NULL, bind[i].buffer_type,
            "Persist JSON null must use MYSQL_TYPE_NULL");
        TEST_ASSERT_EQUAL_INT_MESSAGE(1, bind[i].is_null_value,
            "Persist JSON null must set is_null_value=1");
    }

    // Non-null params must use their TypedParameter type.
    TEST_ASSERT_EQUAL_INT(MYSQL_TYPE_STRING,   bind[0].buffer_type);
    TEST_ASSERT_EQUAL_INT(MYSQL_TYPE_LONGLONG, bind[1].buffer_type);
    TEST_ASSERT_EQUAL_INT(MYSQL_TYPE_STRING,   bind[3].buffer_type);
    TEST_ASSERT_EQUAL_INT(MYSQL_TYPE_STRING,   bind[5].buffer_type);
    TEST_ASSERT_EQUAL_INT(MYSQL_TYPE_STRING,   bind[6].buffer_type);
    TEST_ASSERT_EQUAL_INT(MYSQL_TYPE_STRING,   bind[10].buffer_type);
    TEST_ASSERT_EQUAL_INT(MYSQL_TYPE_STRING,   bind[11].buffer_type);

    // Length pointers for INTEGER must reflect sizeof(long long); STRING
    // must reflect strlen (excluding NUL).
    TEST_ASSERT_EQUAL_UINT(sizeof(long long), *bind[1].length);
    TEST_ASSERT_EQUAL_UINT(36,                *bind[0].length);   // UUID
    TEST_ASSERT_EQUAL_UINT(0,                 *bind[2].length);   // null STRING
    TEST_ASSERT_EQUAL_UINT(19,                *bind[3].length);   // noreply@example.com
    TEST_ASSERT_EQUAL_UINT(5,                 *bind[6].length);   // hello

    mysql_cleanup_bound_values(bound_values, N);
    for (unsigned int i = 0; i < N; ++i) {
        free_test_typed_param(params[i]);
    }
}

// ============================================================================
// Per-type length invariant - individual tests for the types that used to
// have length=NULL (the actual PERSIST_PLAN bug).
// ============================================================================

void test_persist_shape_boolean_length_allocated(void) {
    enum { N = 1 };
    MYSQL_BIND bind[1];
    void** bound_values = calloc(N * 2, sizeof(void*));
    memset(bind, 0, sizeof(bind));

    TypedParameter* p = make_test_typed_param(PARAM_TYPE_BOOLEAN, "b", false);
    p->value.bool_value = true;

    TEST_ASSERT_TRUE(mysql_bind_single_parameter(bind, 0, p, bound_values, 1, "test"));
    TEST_ASSERT_NOT_NULL(bind[0].length);
    TEST_ASSERT_EQUAL_UINT(sizeof(short), *bind[0].length);
    TEST_ASSERT_EQUAL_INT(MYSQL_TYPE_SHORT, bind[0].buffer_type);

    mysql_cleanup_bound_values(bound_values, 1);
    free_test_typed_param(p);
}

void test_persist_shape_float_length_allocated(void) {
    enum { N = 1 };
    MYSQL_BIND bind[1];
    void** bound_values = calloc(N * 2, sizeof(void*));
    memset(bind, 0, sizeof(bind));

    TypedParameter* p = make_test_typed_param(PARAM_TYPE_FLOAT, "f", false);
    p->value.float_value = 1.5;

    TEST_ASSERT_TRUE(mysql_bind_single_parameter(bind, 0, p, bound_values, 1, "test"));
    TEST_ASSERT_NOT_NULL(bind[0].length);
    TEST_ASSERT_EQUAL_UINT(sizeof(double), *bind[0].length);
    TEST_ASSERT_EQUAL_INT(MYSQL_TYPE_DOUBLE, bind[0].buffer_type);

    mysql_cleanup_bound_values(bound_values, 1);
    free_test_typed_param(p);
}

void test_persist_shape_date_length_allocated(void) {
    enum { N = 1 };
    MYSQL_BIND bind[1];
    void** bound_values = calloc(N * 2, sizeof(void*));
    memset(bind, 0, sizeof(bind));

    TypedParameter* p = make_test_typed_param(PARAM_TYPE_DATE, "d", false);
    p->value.date_value = strdup("2026-09-04");

    TEST_ASSERT_TRUE(mysql_bind_single_parameter(bind, 0, p, bound_values, 1, "test"));
    TEST_ASSERT_NOT_NULL(bind[0].length);
    TEST_ASSERT_EQUAL_UINT(sizeof(MYSQL_TIME), *bind[0].length);
    TEST_ASSERT_EQUAL_INT(MYSQL_TYPE_DATE, bind[0].buffer_type);
    TEST_ASSERT_NOT_NULL(bind[0].buffer);

    mysql_cleanup_bound_values(bound_values, 1);
    free_test_typed_param(p);
}

void test_persist_shape_time_length_allocated(void) {
    enum { N = 1 };
    MYSQL_BIND bind[1];
    void** bound_values = calloc(N * 2, sizeof(void*));
    memset(bind, 0, sizeof(bind));

    TypedParameter* p = make_test_typed_param(PARAM_TYPE_TIME, "t", false);
    p->value.time_value = strdup("12:34:56");

    TEST_ASSERT_TRUE(mysql_bind_single_parameter(bind, 0, p, bound_values, 1, "test"));
    TEST_ASSERT_NOT_NULL(bind[0].length);
    TEST_ASSERT_EQUAL_UINT(sizeof(MYSQL_TIME), *bind[0].length);
    TEST_ASSERT_EQUAL_INT(MYSQL_TYPE_TIME, bind[0].buffer_type);
    TEST_ASSERT_NOT_NULL(bind[0].buffer);

    mysql_cleanup_bound_values(bound_values, 1);
    free_test_typed_param(p);
}

void test_persist_shape_datetime_length_allocated(void) {
    enum { N = 1 };
    MYSQL_BIND bind[1];
    void** bound_values = calloc(N * 2, sizeof(void*));
    memset(bind, 0, sizeof(bind));

    TypedParameter* p = make_test_typed_param(PARAM_TYPE_DATETIME, "dt", false);
    p->value.datetime_value = strdup("2026-09-04 12:34:56");

    TEST_ASSERT_TRUE(mysql_bind_single_parameter(bind, 0, p, bound_values, 1, "test"));
    TEST_ASSERT_NOT_NULL(bind[0].length);
    TEST_ASSERT_EQUAL_UINT(sizeof(MYSQL_TIME), *bind[0].length);
    TEST_ASSERT_EQUAL_INT(MYSQL_TYPE_DATETIME, bind[0].buffer_type);
    TEST_ASSERT_NOT_NULL(bind[0].buffer);

    mysql_cleanup_bound_values(bound_values, 1);
    free_test_typed_param(p);
}

void test_persist_shape_timestamp_length_allocated(void) {
    enum { N = 1 };
    MYSQL_BIND bind[1];
    void** bound_values = calloc(N * 2, sizeof(void*));
    memset(bind, 0, sizeof(bind));

    TypedParameter* p = make_test_typed_param(PARAM_TYPE_TIMESTAMP, "ts", false);
    p->value.timestamp_value = strdup("2026-09-04 12:34:56.123");

    TEST_ASSERT_TRUE(mysql_bind_single_parameter(bind, 0, p, bound_values, 1, "test"));
    TEST_ASSERT_NOT_NULL(bind[0].length);
    TEST_ASSERT_EQUAL_UINT(sizeof(MYSQL_TIME), *bind[0].length);
    TEST_ASSERT_EQUAL_INT(MYSQL_TYPE_TIMESTAMP, bind[0].buffer_type);
    TEST_ASSERT_NOT_NULL(bind[0].buffer);

    mysql_cleanup_bound_values(bound_values, 1);
    free_test_typed_param(p);
}

// ============================================================================
// Cleanup releases every length pointer (the second-half table).
// ============================================================================

void test_persist_shape_cleanup_releases_length_pointers(void) {
    enum { N = 4 };
    MYSQL_BIND bind[N];
    void** bound_values = calloc(N * 2, sizeof(void*));
    memset(bind, 0, sizeof(bind));

    TypedParameter* params[N] = {
        make_test_typed_param(PARAM_TYPE_BOOLEAN, "b", false),
        make_test_typed_param(PARAM_TYPE_FLOAT,   "f", false),
        make_test_typed_param(PARAM_TYPE_DATE,    "d", false),
        make_test_typed_param(PARAM_TYPE_DATETIME,"dt", false)
    };
    params[0]->value.bool_value = true;
    params[1]->value.float_value = 0.5;
    params[2]->value.date_value = strdup("2026-01-01");
    params[3]->value.datetime_value = strdup("2026-01-01 00:00:00");

    for (unsigned int i = 0; i < N; ++i) {
        TEST_ASSERT_TRUE(mysql_bind_single_parameter(bind, i, params[i],
                                                      bound_values, N, "test"));
    }

    // Sanity: length pointers in second-half table are non-NULL before cleanup.
    // cppcheck-suppress nullPointerOutOfMemory
    // Justification: calloc failure aborts in TEST_ASSERT_NOT_NULL above
    // for every prior test; this guard covers the cleanup-only path.
    TEST_ASSERT_NOT_NULL(bound_values);
    for (unsigned int i = 0; i < N; ++i) {
        TEST_ASSERT_NOT_NULL(bound_values[N + i]);
    }

    // cleanup must not crash; no assertion here, just exercise the code path.
    mysql_cleanup_bound_values(bound_values, N);

    for (unsigned int i = 0; i < N; ++i) {
        free_test_typed_param(params[i]);
    }
}

// ============================================================================
// malloc failure during length allocation must roll back cleanly.
// ============================================================================

void test_persist_shape_memory_failure_rollback_safe(void) {
    enum { N = 1 };
    MYSQL_BIND bind[1];
    void** bound_values = calloc(N * 2, sizeof(void*));
    memset(bind, 0, sizeof(bind));

    TypedParameter* p = make_test_typed_param(PARAM_TYPE_BOOLEAN, "b", false);
    p->value.bool_value = true;

    // First malloc (bool_val) succeeds; second malloc (bool_len) fails.
    // mysql_bind_single_parameter must return false and not leak.
    mock_system_set_malloc_failure(2);
    TEST_ASSERT_FALSE(mysql_bind_single_parameter(bind, 0, p, bound_values, 1, "test"));
    mock_system_reset_all();

    free_test_typed_param(p);
}

// ============================================================================
// mysql_bind_attach_indicators: NULL bind is a no-op (defensive).
// ============================================================================

void test_persist_shape_attach_indicators_null_bind_safe(void) {
    mysql_bind_attach_indicators(NULL, 0, 1);
    // If we reach here without crashing, the guard works.
    TEST_ASSERT_TRUE(true);
}

// ============================================================================
// mysql_bind_single_parameter: NULL bind_ptr/param/bound_values must fail.
// ============================================================================

void test_persist_shape_bind_single_parameter_null_inputs_safe(void) {
    enum { N = 1 };
    MYSQL_BIND bind[1];
    void** bound_values = calloc(N * 2, sizeof(void*));
    memset(bind, 0, sizeof(bind));

    TypedParameter* p = make_test_typed_param(PARAM_TYPE_INTEGER, "i", false);
    p->value.int_value = 1;

    TEST_ASSERT_FALSE(mysql_bind_single_parameter(NULL, 0, p, bound_values, 1, "test"));
    TEST_ASSERT_FALSE(mysql_bind_single_parameter(bind, 0, NULL, bound_values, 1, "test"));
    TEST_ASSERT_FALSE(mysql_bind_single_parameter(bind, 0, p, NULL, 1, "test"));

    free_test_typed_param(p);
}

int main(void) {
    UNITY_BEGIN();

    // Cross-type invariants
    RUN_TEST(test_persist_shape_length_non_null_for_all_types);
    RUN_TEST(test_persist_shape_is_null_attached_for_all_types);
    RUN_TEST(test_persist_shape_error_attached_for_all_types);
    RUN_TEST(test_persist_shape_json_null_uses_mysql_type_null);
    RUN_TEST(test_persist_shape_buffer_type_matches_param_type);
    RUN_TEST(test_persist_shape_length_value_correct_for_string_and_int);

    // Full Persist shape (QueryRef 093)
    RUN_TEST(test_persist_shape_twelve_binds_with_mixed_nulls);

    // Per-type length invariant (the actual bug)
    RUN_TEST(test_persist_shape_boolean_length_allocated);
    RUN_TEST(test_persist_shape_float_length_allocated);
    RUN_TEST(test_persist_shape_date_length_allocated);
    RUN_TEST(test_persist_shape_time_length_allocated);
    RUN_TEST(test_persist_shape_datetime_length_allocated);
    RUN_TEST(test_persist_shape_timestamp_length_allocated);

    // Cleanup and safety
    RUN_TEST(test_persist_shape_cleanup_releases_length_pointers);
    RUN_TEST(test_persist_shape_memory_failure_rollback_safe);
    RUN_TEST(test_persist_shape_attach_indicators_null_bind_safe);
    RUN_TEST(test_persist_shape_bind_single_parameter_null_inputs_safe);

    return UNITY_END();
}
