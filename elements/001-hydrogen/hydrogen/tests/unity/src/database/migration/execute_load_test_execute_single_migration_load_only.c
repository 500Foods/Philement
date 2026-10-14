/*
 * Unity Test File: execute_load_test_execute_single_migration_load_only
 * Unit tests for execute_single_migration_load_only() in execute_load.c, the
 * legacy wrapper that creates its own Lua state.
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
bool execute_single_migration_load_only(DatabaseHandle* connection, const char* migration_file,
                                       const char* engine_name, const char* migration_name,
                                       const char* schema_name, const char* dqm_label);

// Forward declarations for test functions
void test_execute_single_migration_load_only_success(void);
void test_execute_single_migration_load_only_module_load_failure(void);
void test_execute_single_migration_load_only_payload_failure(void);

/* Miniature Lua sources for the synthetic design. */
static const char* kEngineModule = "return {}";
static const char* kMigrationModule = "return function(engine, design, schema, cfg) return {} end";
static const char* kDatabaseModule =
    "return { defaults = { sqlite = {} },"
    " run_migration = function(self, queries, engine, design, schema)"
    " return \"SELECT 1;\" end }";

static const char* kEngines[] = {"sqlite", "postgresql", "mysql", "mariadb", "mssql", "db2", "firebird"};

/* Install a synthetic cache holding one complete miniature design. */
static void cache_install_design(const char* design, const char* database_lua) {
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
    global_payload_cache.files[slot].data = (uint8_t*)strdup(kMigrationModule);
    global_payload_cache.files[slot].size = strlen(kMigrationModule);
    global_payload_cache.files[slot].is_compressed = false;

    for (size_t i = 0; i < 9; i++) {
        TEST_ASSERT_NOT_NULL(global_payload_cache.files[i].name);
        TEST_ASSERT_NOT_NULL(global_payload_cache.files[i].data);
    }
}

/* Install a cache with one matching file but no database.lua module. */
static void cache_install_without_module(const char* design) {
    char name[128];

    cleanup_payload_cache();
    initialize_payload_cache();
    global_payload_cache.is_initialized = true;
    global_payload_cache.is_available = true;
    global_payload_cache.capacity = 1;
    global_payload_cache.num_files = 1;
    global_payload_cache.files = calloc(1, sizeof(PayloadFile));
    TEST_ASSERT_NOT_NULL(global_payload_cache.files);

    snprintf(name, sizeof(name), "%s/readme.txt", design);
    global_payload_cache.files[0].name = strdup(name);
    global_payload_cache.files[0].data = (uint8_t*)strdup("not lua");
    global_payload_cache.files[0].size = strlen("not lua");
    global_payload_cache.files[0].is_compressed = false;
}

static void cache_reset(void) {
    cleanup_payload_cache();
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

// ===== execute_single_migration_load_only TESTS =====

void test_execute_single_migration_load_only_success(void) {
    DatabaseHandle connection = make_connection();

    cache_install_design("test_design", kDatabaseModule);

    bool result = execute_single_migration_load_only(&connection, "test_design/test_design_1001.lua",
                                                    "sqlite", "test_design", "test_schema",
                                                    "test-label");

    TEST_ASSERT_TRUE(result);
}

void test_execute_single_migration_load_only_module_load_failure(void) {
    DatabaseHandle connection = make_connection();

    /* The design has files, but none of them is database.lua. */
    cache_install_without_module("test_design");

    bool result = execute_single_migration_load_only(&connection, "test_design/readme.txt",
                                                    "sqlite", "test_design", "test_schema",
                                                    "test-label");

    TEST_ASSERT_FALSE(result);
}

void test_execute_single_migration_load_only_payload_failure(void) {
    DatabaseHandle connection = make_connection();

    /* Nothing installed, so the payload lookup fails right after lua_setup. */
    bool result = execute_single_migration_load_only(&connection, "test_design/test_design_1001.lua",
                                                    "sqlite", "test_design", "test_schema",
                                                    "test-label");

    TEST_ASSERT_FALSE(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_execute_single_migration_load_only_success);
    RUN_TEST(test_execute_single_migration_load_only_module_load_failure);
    RUN_TEST(test_execute_single_migration_load_only_payload_failure);

    return UNITY_END();
}
