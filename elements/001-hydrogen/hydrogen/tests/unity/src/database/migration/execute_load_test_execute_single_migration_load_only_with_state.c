/*
 * Unity Test File: execute_load_test_execute_single_migration_load_only_with_state
 * Unit tests for execute_single_migration_load_only_with_state() in execute_load.c,
 * covering both the own-state and shared-state paths.
 *
 * These tests install a synthetic payload cache that holds a miniature Lua
 * design so the real Lua loader in lua.c can run end to end.
 */

#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/migration/migration.h>
#include <src/payload/payload.h>
#include <src/payload/payload_cache.h>

// The payload cache is a plain global so tests can install a synthetic list
extern PayloadCache global_payload_cache;

// Forward declarations for functions being tested
bool execute_single_migration_load_only_with_state(DatabaseHandle* connection, const char* migration_file,
                                                   const char* engine_name, const char* migration_name,
                                                   const char* schema_name, const char* dqm_label,
                                                   lua_State* L, PayloadFile* payload_files, size_t payload_count);

// Forward declarations for test functions
void test_execute_single_migration_load_only_with_state_own_state_success(void);
void test_execute_single_migration_load_only_with_state_migration_file_not_found(void);
void test_execute_single_migration_load_only_with_state_migration_file_load_failure(void);
void test_execute_single_migration_load_only_with_state_migration_function_failure(void);
void test_execute_single_migration_load_only_with_state_load_metadata_failure(void);
void test_execute_single_migration_load_only_with_state_sql_copy_allocation_failure(void);
void test_execute_single_migration_load_only_with_state_no_sql_generated(void);
void test_execute_single_migration_load_only_with_state_shared_state_success(void);

/* Miniature Lua sources for the synthetic design. */
static const char* kEngineModule = "return {}";
static const char* kMigrationModule = "return function(engine, design, schema, cfg) return {} end";
static const char* kDatabaseModule =
    "return { defaults = { sqlite = {} },"
    " run_migration = function(self, queries, engine, design, schema)"
    " return \"SELECT 1;\" end }";
static const char* kDatabaseModuleWithoutRunMigration = "return { defaults = { sqlite = {} } }";
static const char* kDatabaseModuleWithoutEngine = "return { defaults = {} }";
static const char* kMigrationModuleNotAFunction = "return 5";

static const char* kEngines[] = {"sqlite", "postgresql", "mysql", "mariadb", "mssql", "db2", "firebird"};

/* Install a synthetic cache holding one complete miniature design. */
static void cache_install_design(const char* design, const char* database_lua,
                                 const char* migration_lua) {
    char name[128];

    cleanup_payload_cache();
    initialize_payload_cache();
    global_payload_cache.is_initialized = true;
    global_payload_cache.is_available = true;
    global_payload_cache.capacity = 9;
    global_payload_cache.num_files = 9;
    global_payload_cache.files = calloc(9, sizeof(PayloadFile));
    TEST_ASSERT_NOT_NULL(global_payload_cache.files);

    size_t slot = 0;
    for (size_t i = 0; i < sizeof(kEngines) / sizeof(kEngines[0]); i++) {
        snprintf(name, sizeof(name), "%s/database_%s.lua", design, kEngines[i]);
        global_payload_cache.files[slot].name = strdup(name);
        global_payload_cache.files[slot].data = (uint8_t*)strdup(kEngineModule);
        global_payload_cache.files[slot].size = strlen(kEngineModule);
        global_payload_cache.files[slot].is_compressed = false;
        slot++;
    }

    snprintf(name, sizeof(name), "%s/database.lua", design);
    global_payload_cache.files[slot].name = strdup(name);
    global_payload_cache.files[slot].data = (uint8_t*)strdup(database_lua);
    global_payload_cache.files[slot].size = strlen(database_lua);
    global_payload_cache.files[slot].is_compressed = false;
    slot++;

    snprintf(name, sizeof(name), "%s/%s_1001.lua", design, design);
    global_payload_cache.files[slot].name = strdup(name);
    global_payload_cache.files[slot].data = (uint8_t*)strdup(migration_lua);
    global_payload_cache.files[slot].size = strlen(migration_lua);
    global_payload_cache.files[slot].is_compressed = false;

    for (size_t i = 0; i < 9; i++) {
        TEST_ASSERT_NOT_NULL(global_payload_cache.files[i].name);
        TEST_ASSERT_NOT_NULL(global_payload_cache.files[i].data);
    }
}

static void cache_reset(void) {
    cleanup_payload_cache();
}

/* Fetch the design's payload files the way the caller normally would. */
static void fetch_payload(const char* design, PayloadFile** files_out, size_t* count_out) {
    size_t capacity = 0;
    TEST_ASSERT_TRUE(get_payload_files_by_prefix(design, files_out, count_out, &capacity));
    TEST_ASSERT_NOT_NULL(*files_out);
    TEST_ASSERT_TRUE(*count_out > 0);
}

static DatabaseHandle make_connection(void) {
    DatabaseHandle connection;
    memset(&connection, 0, sizeof(connection));
    connection.engine_type = DB_ENGINE_SQLITE;
    return connection;
}

void setUp(void) {
    mock_system_reset_all();
    cache_reset();
}

void tearDown(void) {
    cache_reset();
    mock_system_reset_all();
}

// ===== execute_single_migration_load_only_with_state TESTS =====

void test_execute_single_migration_load_only_with_state_own_state_success(void) {
    DatabaseHandle connection = make_connection();

    cache_install_design("test_design", kDatabaseModule, kMigrationModule);

    PayloadFile* payload_files = NULL;
    size_t payload_count = 0;
    fetch_payload("test_design", &payload_files, &payload_count);

    bool result = execute_single_migration_load_only_with_state(&connection, "test_design/test_design_1001.lua",
                                                                "sqlite", "test_design", "test_schema",
                                                                "test-label", NULL, payload_files, payload_count);

    TEST_ASSERT_TRUE(result);

    free_payload_files(payload_files, payload_count);
}

void test_execute_single_migration_load_only_with_state_migration_file_not_found(void) {
    DatabaseHandle connection = make_connection();

    cache_install_design("test_design", kDatabaseModule, kMigrationModule);

    PayloadFile* payload_files = NULL;
    size_t payload_count = 0;
    fetch_payload("test_design", &payload_files, &payload_count);

    /* The Lua state is created and the module loads, but the requested file is
     * not in the payload. */
    bool result = execute_single_migration_load_only_with_state(&connection, "test_design/test_design_9999.lua",
                                                                "sqlite", "test_design", "test_schema",
                                                                "test-label", NULL, payload_files, payload_count);

    TEST_ASSERT_FALSE(result);

    free_payload_files(payload_files, payload_count);
}

void test_execute_single_migration_load_only_with_state_migration_file_load_failure(void) {
    DatabaseHandle connection = make_connection();

    /* The file loads but returns a number instead of a function. */
    cache_install_design("test_design", kDatabaseModule, kMigrationModuleNotAFunction);

    PayloadFile* payload_files = NULL;
    size_t payload_count = 0;
    fetch_payload("test_design", &payload_files, &payload_count);

    bool result = execute_single_migration_load_only_with_state(&connection, "test_design/test_design_1001.lua",
                                                                "sqlite", "test_design", "test_schema",
                                                                "test-label", NULL, payload_files, payload_count);

    TEST_ASSERT_FALSE(result);

    free_payload_files(payload_files, payload_count);
}

void test_execute_single_migration_load_only_with_state_migration_function_failure(void) {
    DatabaseHandle connection = make_connection();

    /* defaults has no entry for the requested engine. */
    cache_install_design("test_design", kDatabaseModuleWithoutEngine, kMigrationModule);

    PayloadFile* payload_files = NULL;
    size_t payload_count = 0;
    fetch_payload("test_design", &payload_files, &payload_count);

    bool result = execute_single_migration_load_only_with_state(&connection, "test_design/test_design_1001.lua",
                                                                "sqlite", "test_design", "test_schema",
                                                                "test-label", NULL, payload_files, payload_count);

    TEST_ASSERT_FALSE(result);

    free_payload_files(payload_files, payload_count);
}

void test_execute_single_migration_load_only_with_state_load_metadata_failure(void) {
    DatabaseHandle connection = make_connection();

    /* The engine config exists, but database.lua has no run_migration(). */
    cache_install_design("test_design", kDatabaseModuleWithoutRunMigration, kMigrationModule);

    PayloadFile* payload_files = NULL;
    size_t payload_count = 0;
    fetch_payload("test_design", &payload_files, &payload_count);

    bool result = execute_single_migration_load_only_with_state(&connection, "test_design/test_design_1001.lua",
                                                                "sqlite", "test_design", "test_schema",
                                                                "test-label", NULL, payload_files, payload_count);

    TEST_ASSERT_FALSE(result);

    free_payload_files(payload_files, payload_count);
}

void test_execute_single_migration_load_only_with_state_sql_copy_allocation_failure(void) {
    DatabaseHandle connection = make_connection();

    cache_install_design("test_design", kDatabaseModule, kMigrationModule);

    PayloadFile* payload_files = NULL;
    size_t payload_count = 0;
    fetch_payload("test_design", &payload_files, &payload_count);

    /* Lua allocates through mmap, so the first heap allocation in the chain is
     * the SQL copy. */
    mock_system_set_malloc_failure(1);

    bool result = execute_single_migration_load_only_with_state(&connection, "test_design/test_design_1001.lua",
                                                                "sqlite", "test_design", "test_schema",
                                                                "test-label", NULL, payload_files, payload_count);

    TEST_ASSERT_FALSE(result);

    free_payload_files(payload_files, payload_count);
}

void test_execute_single_migration_load_only_with_state_no_sql_generated(void) {
    DatabaseHandle connection = make_connection();

    /* run_migration returns an empty string, so no SQL copy is made. */
    cache_install_design("test_design",
                         "return { defaults = { sqlite = {} },"
                         " run_migration = function(self, queries, engine, design, schema)"
                         " return \"\" end }",
                         kMigrationModule);

    PayloadFile* payload_files = NULL;
    size_t payload_count = 0;
    fetch_payload("test_design", &payload_files, &payload_count);

    bool result = execute_single_migration_load_only_with_state(&connection, "test_design/test_design_1001.lua",
                                                                "sqlite", "test_design", "test_schema",
                                                                "test-label", NULL, payload_files, payload_count);

    TEST_ASSERT_FALSE(result);

    free_payload_files(payload_files, payload_count);
}

void test_execute_single_migration_load_only_with_state_shared_state_success(void) {
    DatabaseHandle connection = make_connection();
    const char* migration_file = "test_design/test_design_1001.lua";

    cache_install_design("test_design", kDatabaseModule, kMigrationModule);

    PayloadFile* payload_files = NULL;
    size_t payload_count = 0;
    fetch_payload("test_design", &payload_files, &payload_count);

    /* A shared state must already have the database module loaded. */
    lua_State* L = lua_setup("test-label");
    TEST_ASSERT_NOT_NULL(L);
    TEST_ASSERT_TRUE(lua_load_database_module(L, "test_design", payload_files, payload_count, "test-label"));

    bool result = execute_single_migration_load_only_with_state(&connection, migration_file,
                                                                "sqlite", "test_design", "test_schema",
                                                                "test-label", L, payload_files, payload_count);

    TEST_ASSERT_TRUE(result);

    /* execute_single_migration_load_only_with_state() does not close a state it
     * was handed, so the test owns the cleanup. */
    lua_cleanup(L);
    free_payload_files(payload_files, payload_count);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_execute_single_migration_load_only_with_state_own_state_success);
    RUN_TEST(test_execute_single_migration_load_only_with_state_migration_file_not_found);
    RUN_TEST(test_execute_single_migration_load_only_with_state_migration_file_load_failure);
    RUN_TEST(test_execute_single_migration_load_only_with_state_migration_function_failure);
    RUN_TEST(test_execute_single_migration_load_only_with_state_load_metadata_failure);
    RUN_TEST(test_execute_single_migration_load_only_with_state_sql_copy_allocation_failure);
    RUN_TEST(test_execute_single_migration_load_only_with_state_no_sql_generated);
    RUN_TEST(test_execute_single_migration_load_only_with_state_shared_state_success);

    return UNITY_END();
}
