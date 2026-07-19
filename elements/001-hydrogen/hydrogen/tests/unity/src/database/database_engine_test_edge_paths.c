/*
 * Unity Test File: database_engine_test_edge_paths
 *
 * Targets the error/edge branches in src/database/database_engine.c
 * that real engines (and therefore the blackbox suite) never reach:
 *
 *   - health_check with an engine that has no health_check fn (69-70)
 *   - execute with an engine that has no execute_query fn   (155-156)
 *   - prepared-statement create with no parameters_json
 *     (empty ParameterList malloc, 193-196)
 *   - prepared-statement create with parameters_json
 *     (free ordered_params, 220)
 *   - prepared-statement cache full -> use-once-then-free   (238)
 *   - created-but-uncached prepared statement cleanup       (269-272)
 *   - retry layer bumping DB_ERR_NONE to DB_ERR_OTHER       (383)
 *   - engine-level build/validate connection string         (447-451, 459-464)
 *   - cleanup of connection with empty prepared-statement
 *     array (count == 0)                                     (485-489)
 *   - cleanup skipping a NULL/invalid prepared statement     (498)
 *   - DB2 connection handle prepared-statement cache free    (556)
 *
 * NOTE: lines 287-289 (the second `!engine->execute_query` check) and
 * 399 (backoff cap at 30s) are intentionally NOT covered. 287-289 is
 * unreachable because the identical check at line 154 already returns
 * earlier whenever execute_query is NULL, and 399 would require ~63s of
 * real sleep() in the retry loop - not acceptable for a unit test.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>

// ---------------------------------------------------------------------------
// Controllable mock engine (registered under DB_ENGINE_AI)
// ---------------------------------------------------------------------------

static ConnectionConfig mock_config;

static bool g_execute_success = true;
static DatabaseErrorClass g_execute_error_class = DB_ERR_NONE;
static bool g_prepare_success = true;
static bool g_has_health_check = true;
static bool g_has_execute_query = true;

// Captured prepared statement from the last prepare call (for cleanup tests)
static PreparedStatement* g_captured_stmt = NULL;

bool mock_edge_connect(ConnectionConfig* config, DatabaseHandle** connection, const char* designator);
bool mock_edge_disconnect(DatabaseHandle* connection);
bool mock_edge_health_check(DatabaseHandle* connection);
bool mock_edge_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result);
bool mock_edge_execute_prepared(DatabaseHandle* connection, const PreparedStatement* stmt, QueryRequest* request, QueryResult** result);
bool mock_edge_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt, bool add_to_cache);
bool mock_edge_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt);
char* mock_edge_get_connection_string(const ConnectionConfig* config);
bool mock_edge_validate_connection_string(const char* connection_string);

bool mock_edge_connect(ConnectionConfig* config, DatabaseHandle** connection, const char* designator) {
    (void)config;
    (void)designator;
    if (connection) {
        *connection = calloc(1, sizeof(DatabaseHandle));
        if (*connection) {
            (*connection)->engine_type = DB_ENGINE_AI;
            (*connection)->designator = designator ? strdup(designator) : NULL;
            /*
             * The connection owns its config (free_connection_config is called
             * on cleanup), so give it a private heap copy rather than pointing
             * at the shared static mock_config fixture.
             */
            ConnectionConfig* cfg = calloc(1, sizeof(ConnectionConfig));
            if (cfg) {
                cfg->host = mock_config.host ? strdup(mock_config.host) : NULL;
                cfg->database = mock_config.database ? strdup(mock_config.database) : NULL;
                cfg->username = mock_config.username ? strdup(mock_config.username) : NULL;
                cfg->password = mock_config.password ? strdup(mock_config.password) : NULL;
                cfg->timeout_seconds = mock_config.timeout_seconds;
                cfg->prepared_statement_cache_size = mock_config.prepared_statement_cache_size;
            }
            (*connection)->config = cfg;
            pthread_mutex_init(&(*connection)->connection_lock, NULL);
            return true;
        }
    }
    return false;
}

bool mock_edge_disconnect(DatabaseHandle* connection) {
    (void)connection;
    return true;
}

bool mock_edge_health_check(DatabaseHandle* connection) {
    (void)connection;
    return true;
}

bool mock_edge_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result) {
    (void)connection;
    (void)request;
    if (!g_has_execute_query) {
        return false;
    }
    if (result) {
        *result = calloc(1, sizeof(QueryResult));
        if (*result) {
            (*result)->success = g_execute_success;
            if (!g_execute_success) {
                (*result)->error_class = g_execute_error_class;
            }
        }
    }
    return g_execute_success;
}

bool mock_edge_execute_prepared(DatabaseHandle* connection, const PreparedStatement* stmt, QueryRequest* request, QueryResult** result) {
    (void)connection;
    (void)stmt;
    (void)request;
    if (result) {
        *result = calloc(1, sizeof(QueryResult));
        if (*result) {
            (*result)->success = true;
            (*result)->row_count = 1;
        }
    }
    return true;
}

bool mock_edge_prepare_statement(DatabaseHandle* connection, const char* name, const char* sql, PreparedStatement** stmt, bool add_to_cache) {
    (void)connection;
    (void)sql;
    (void)add_to_cache;
    if (!g_prepare_success) {
        return false;
    }
    PreparedStatement* s = calloc(1, sizeof(PreparedStatement));
    if (!s) {
        return false;
    }
    s->name = strdup(name);
    s->sql_template = strdup(sql ? sql : "SELECT 1");
    *stmt = s;
    g_captured_stmt = s;
    return true;
}

bool mock_edge_unprepare_statement(DatabaseHandle* connection, PreparedStatement* stmt) {
    (void)connection;
    if (!stmt) {
        return false;
    }
    if (stmt->name) {
        free(stmt->name);
    }
    if (stmt->sql_template) {
        free(stmt->sql_template);
    }
    free(stmt);
    if (g_captured_stmt == stmt) {
        g_captured_stmt = NULL;
    }
    return true;
}

char* mock_edge_get_connection_string(const ConnectionConfig* config) {
    (void)config;
    return strdup("mock://connection/string");
}

bool mock_edge_validate_connection_string(const char* connection_string) {
    (void)connection_string;
    return true;
}

static DatabaseEngineInterface mock_edge_engine = {
    .engine_type = DB_ENGINE_AI,
    .name = (char*)"ai",
    .connect = mock_edge_connect,
    .disconnect = mock_edge_disconnect,
    .health_check = mock_edge_health_check,
    .reset_connection = NULL,
    .execute_query = mock_edge_execute_query,
    .execute_prepared = mock_edge_execute_prepared,
    .begin_transaction = NULL,
    .commit_transaction = NULL,
    .rollback_transaction = NULL,
    .prepare_statement = mock_edge_prepare_statement,
    .unprepare_statement = mock_edge_unprepare_statement,
    .get_connection_string = mock_edge_get_connection_string,
    .validate_connection_string = mock_edge_validate_connection_string,
    .escape_string = NULL
};

// ---------------------------------------------------------------------------
// Test fixtures
// ---------------------------------------------------------------------------

// Forward declarations
void test_health_check_engine_without_health_check_fn(void);
void test_execute_engine_without_execute_query_fn(void);
void test_execute_prepared_no_parameters_json(void);
void test_execute_prepared_with_parameters_json(void);
void test_execute_prepared_cache_full(void);
void test_execute_retry_bumps_none_to_other(void);
void test_build_connection_string_engine_path(void);
void test_validate_connection_string_engine_path(void);
void test_cleanup_connection_empty_prepared_array(void);
void test_cleanup_connection_skips_invalid_statement(void);
void test_cleanup_connection_db2_cache(void);

static void reset_mock_state(void) {
    g_execute_success = true;
    g_execute_error_class = DB_ERR_NONE;
    g_prepare_success = true;
    g_has_health_check = true;
    g_has_execute_query = true;
    g_captured_stmt = NULL;
}

void setUp(void) {
    reset_mock_state();
    database_engine_init();

    // Register our controllable engine under DB_ENGINE_AI if not already present.
    if (!database_engine_get(DB_ENGINE_AI)) {
        (void)database_engine_register(&mock_edge_engine);
    }

    memset(&mock_config, 0, sizeof(ConnectionConfig));
    mock_config.host = strdup("localhost");
    mock_config.database = strdup("testdb");
    mock_config.username = strdup("testuser");
    mock_config.password = strdup("testpass");
    mock_config.timeout_seconds = 30;
    mock_config.prepared_statement_cache_size = 1000;
}

void tearDown(void) {
    // Restore the engine's function pointers so other tests in this file
    // (or a re-run) start from a known-good state.
    mock_edge_engine.health_check = mock_edge_health_check;
    mock_edge_engine.execute_query = mock_edge_execute_query;
    reset_mock_state();

    if (mock_config.host) {
        free(mock_config.host);
        mock_config.host = NULL;
    }
    if (mock_config.database) {
        free(mock_config.database);
        mock_config.database = NULL;
    }
    if (mock_config.username) {
        free(mock_config.username);
        mock_config.username = NULL;
    }
    if (mock_config.password) {
        free(mock_config.password);
        mock_config.password = NULL;
    }
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

/* Lines 69-70: health_check with an engine that has no health_check fn. */
void test_health_check_engine_without_health_check_fn(void) {
    // Build a connection of the registered AI engine type.
    DatabaseHandle* connection = NULL;
    bool ok = database_engine_connect_with_designator(DB_ENGINE_AI, &mock_config, &connection, "edge-hc");
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_NOT_NULL(connection);

    // Mutate the registered engine so it has no health_check function.
    mock_edge_engine.health_check = NULL;

    bool hc = database_engine_health_check(connection);
    TEST_ASSERT_FALSE(hc);

    // Restore and clean up.
    mock_edge_engine.health_check = mock_edge_health_check;
    if (connection) {
        database_engine_cleanup_connection(connection);
    }
}

/* Lines 155-156: execute with an engine that has no execute_query fn. */
void test_execute_engine_without_execute_query_fn(void) {
    DatabaseHandle* connection = NULL;
    bool ok = database_engine_connect_with_designator(DB_ENGINE_AI, &mock_config, &connection, "edge-eq");
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_NOT_NULL(connection);

    // Mutate the registered engine so it has no execute_query function.
    mock_edge_engine.execute_query = NULL;

    QueryRequest request = {0};
    request.sql_template = strdup("SELECT 1");
    QueryResult* result = NULL;
    bool exec = database_engine_execute(connection, &request, &result);
    TEST_ASSERT_FALSE(exec);

    free(request.sql_template);
    mock_edge_engine.execute_query = mock_edge_execute_query;
    if (connection) {
        database_engine_cleanup_connection(connection);
    }
}

/*
 * Lines 193-196: prepared-statement create path with no parameters_json.
 * parse_typed_parameters is skipped, so an empty ParameterList is malloc'd.
 */
void test_execute_prepared_no_parameters_json(void) {
    DatabaseHandle* connection = NULL;
    bool ok = database_engine_connect_with_designator(DB_ENGINE_AI, &mock_config, &connection, "edge-prep1");
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_NOT_NULL(connection);

    QueryRequest request = {0};
    request.sql_template = strdup("SELECT * FROM t WHERE id = :id");
    request.use_prepared_statement = true;
    request.prepared_statement_name = strdup("stmt_no_params");
    // No parameters_json -> empty ParameterList malloc branch (193-196).
    request.parameters_json = NULL;

    QueryResult* result = NULL;
    bool exec = database_engine_execute(connection, &request, &result);
    TEST_ASSERT_TRUE(exec);

    free(request.sql_template);
    free(request.prepared_statement_name);
    if (result) {
        database_engine_cleanup_result(result);
    }
    if (connection) {
        database_engine_cleanup_connection(connection);
    }
}

/*
 * Line 220: prepared-statement create path WITH parameters_json so
 * convert_named_to_positional allocates (and the test frees) ordered_params.
 */
void test_execute_prepared_with_parameters_json(void) {
    DatabaseHandle* connection = NULL;
    bool ok = database_engine_connect_with_designator(DB_ENGINE_AI, &mock_config, &connection, "edge-prep2");
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_NOT_NULL(connection);

    QueryRequest request = {0};
    request.sql_template = strdup("SELECT * FROM t WHERE id = :id");
    request.use_prepared_statement = true;
    request.prepared_statement_name = strdup("stmt_with_params");
    request.parameters_json = strdup("{\"INTEGER\":{\"id\":42}}");

    QueryResult* result = NULL;
    bool exec = database_engine_execute(connection, &request, &result);
    TEST_ASSERT_TRUE(exec);

    free(request.sql_template);
    free(request.prepared_statement_name);
    free(request.parameters_json);
    if (result) {
        database_engine_cleanup_result(result);
    }
    if (connection) {
        database_engine_cleanup_connection(connection);
    }
}

/*
 * Lines 238 and 269-272: prepared-statement cache is full, so the freshly
 * prepared statement cannot be cached and must be unprepared after use.
 */
void test_execute_prepared_cache_full(void) {
    DatabaseHandle* connection = NULL;
    bool ok = database_engine_connect_with_designator(DB_ENGINE_AI, &mock_config, &connection, "edge-cachefull");
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_NOT_NULL(connection);

    // Shrink the cache to 1 slot and pre-store one statement so the cache is full.
    connection->config->prepared_statement_cache_size = 1; // cppcheck-suppress nullPointerRedundantCheck
    PreparedStatement* existing = calloc(1, sizeof(PreparedStatement));
    TEST_ASSERT_NOT_NULL(existing);
    existing->name = strdup("already_cached");
    existing->sql_template = strdup("SELECT 1");
    bool stored = store_prepared_statement(connection, existing);
    TEST_ASSERT_TRUE(stored);
    TEST_ASSERT_EQUAL_size_t(1, connection->prepared_statement_count); // cppcheck-suppress nullPointerRedundantCheck

    QueryRequest request = {0};
    request.sql_template = strdup("SELECT * FROM t WHERE id = :id");
    request.use_prepared_statement = true;
    request.prepared_statement_name = strdup("stmt_cache_full");
    request.parameters_json = NULL;

    QueryResult* result = NULL;
    bool exec = database_engine_execute(connection, &request, &result);
    TEST_ASSERT_TRUE(exec);

    // The uncached statement should have been unprepared (freed) by cleanup.
    TEST_ASSERT_NULL(g_captured_stmt);

    free(request.sql_template);
    free(request.prepared_statement_name);
    if (result) {
        database_engine_cleanup_result(result);
    }

    /*
     * store_prepared_statement evicted the pre-stored statement via
     * engine->unprepare_statement, leaving a NULL (not dangling) slot, so
     * normal cleanup frees the array correctly.
     */
    if (connection) {
        database_engine_cleanup_connection(connection);
    }
}

/*
 * Line 383: a failed query that returns a result with error_class == DB_ERR_NONE
 * (the calloc default) must be bumped to DB_ERR_OTHER so it is not retried.
 */
void test_execute_retry_bumps_none_to_other(void) {
    DatabaseHandle* connection = NULL;
    bool ok = database_engine_connect_with_designator(DB_ENGINE_AI, &mock_config, &connection, "edge-none");
    TEST_ASSERT_TRUE(ok);
    TEST_ASSERT_NOT_NULL(connection);

    // Engine returns failure but leaves error_class at its calloc default (NONE).
    g_execute_success = false;
    g_execute_error_class = DB_ERR_NONE;

    QueryRequest request = {0};
    request.sql_template = strdup("SELECT 1");
    request.max_retries = 0; // Single attempt -> no real retry/sleep.

    QueryResult* result = NULL;
    bool exec = database_engine_execute(connection, &request, &result);
    // The engine failed; with error_class left at its calloc default (NONE) the
    // abstraction bumps the *local* error class to OTHER so it is not retried
    // (line 383). It does not rewrite result->error_class, so just verify the
    // call was not retried (single attempt, max_retries == 0).
    TEST_ASSERT_FALSE(exec);
    if (result) {
        database_engine_cleanup_result(result);
    }

    if (connection) {
        database_engine_cleanup_connection(connection);
    }
}

/* Lines 447-451: engine-level build connection string delegates to the engine. */
void test_build_connection_string_engine_path(void) {
    char* result = database_engine_build_connection_string(DB_ENGINE_AI, &mock_config);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("mock://connection/string", result);
    free(result);
}

/* Lines 459-464: engine-level validate connection string delegates to the engine. */
void test_validate_connection_string_engine_path(void) {
    bool ok = database_engine_validate_connection_string(DB_ENGINE_AI, "mock://connection/string");
    TEST_ASSERT_TRUE(ok);
}

/*
 * Lines 485-489: cleanup of a connection that has a prepared-statement array
 * but prepared_statement_count == 0 (already cleared by a transaction boundary).
 */
void test_cleanup_connection_empty_prepared_array(void) {
    DatabaseHandle* connection = calloc(1, sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(connection);
    connection->engine_type = DB_ENGINE_AI;
    connection->designator = NULL;
    connection->config = NULL;
    connection->prepared_statements = calloc(4, sizeof(PreparedStatement*));
    connection->prepared_statement_count = 0; // Cleared, but array still allocated.
    connection->prepared_statement_lru_counter = calloc(4, sizeof(uint64_t));
    pthread_mutex_init(&connection->connection_lock, NULL);

    database_engine_cleanup_connection(connection);
}

/*
 * Line 498: cleanup skips a NULL/invalid prepared-statement pointer in the array.
 */
void test_cleanup_connection_skips_invalid_statement(void) {
    DatabaseHandle* connection = calloc(1, sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(connection);
    connection->engine_type = DB_ENGINE_AI;
    connection->designator = strdup("edge-invalid-ps");
    connection->config = NULL;
    connection->prepared_statements = calloc(1, sizeof(PreparedStatement*));
    connection->prepared_statements[0] = NULL; // Invalid entry -> skip (498).
    connection->prepared_statement_count = 1;
    connection->prepared_statement_lru_counter = calloc(1, sizeof(uint64_t));
    pthread_mutex_init(&connection->connection_lock, NULL);

    database_engine_cleanup_connection(connection);
}

/*
 * Line 556: DB2 connection handle prepared-statement cache free loop with
 * at least one entry in the cache's name array.
 */
void test_cleanup_connection_db2_cache(void) {
    // Minimal structural mirror of the DB2Connection / PreparedStatementCache
    // types used in database_engine_cleanup_connection (we cannot include the
    // DB2 header here, so we recreate the layout the cleanup code casts to).
    typedef struct {
        char** names;
        size_t count;
        size_t capacity;
        pthread_mutex_t lock;
    } FakePreparedStatementCache;

    typedef struct {
        void* environment;
        void* connection;
        void* prepared_statements;
    } FakeDB2Connection;

    FakePreparedStatementCache* cache = calloc(1, sizeof(FakePreparedStatementCache));
    TEST_ASSERT_NOT_NULL(cache);
    cache->count = 2;
    cache->capacity = 4;
    cache->names = calloc(cache->capacity, sizeof(char*));
    cache->names[0] = strdup("stmt_a");
    cache->names[1] = strdup("stmt_b");
    pthread_mutex_init(&cache->lock, NULL);

    FakeDB2Connection* db2 = calloc(1, sizeof(FakeDB2Connection));
    TEST_ASSERT_NOT_NULL(db2);
    db2->prepared_statements = cache;

    DatabaseHandle* connection = calloc(1, sizeof(DatabaseHandle));
    TEST_ASSERT_NOT_NULL(connection);
    connection->engine_type = DB_ENGINE_DB2; // No DB2 engine registered -> engine NULL.
    connection->designator = NULL;
    connection->config = NULL;
    connection->connection_handle = db2;
    connection->prepared_statements = NULL;
    pthread_mutex_init(&connection->connection_lock, NULL);

    database_engine_cleanup_connection(connection);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_health_check_engine_without_health_check_fn);
    RUN_TEST(test_execute_engine_without_execute_query_fn);
    RUN_TEST(test_execute_prepared_no_parameters_json);
    RUN_TEST(test_execute_prepared_with_parameters_json);
    RUN_TEST(test_execute_prepared_cache_full);
    RUN_TEST(test_execute_retry_bumps_none_to_other);
    RUN_TEST(test_build_connection_string_engine_path);
    RUN_TEST(test_validate_connection_string_engine_path);
    RUN_TEST(test_cleanup_connection_empty_prepared_array);
    RUN_TEST(test_cleanup_connection_skips_invalid_statement);
    RUN_TEST(test_cleanup_connection_db2_cache);

    return UNITY_END();
}
