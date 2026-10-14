/*
 * Unity Test File: execute_load_test_execute_load_migrations_success
 * Unit tests for execute_load_migrations() in execute_load.c, covering the
 * successful LOAD path and the migration-name extraction failure path.
 *
 * These tests install a synthetic payload cache that holds a miniature Lua
 * design (database.lua, one database_<engine>.lua per engine and one numbered
 * migration file) so the real Lua loader in lua.c can run end to end.
 */

#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

#include <lua.h>
#include <lauxlib.h>

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/migration/migration.h>
#include <src/database/dbqueue/dbqueue.h>
#include <src/config/config.h>
#include <src/payload/payload.h>
#include <src/payload/payload_cache.h>

// The payload cache is a plain global so tests can install a synthetic list
extern PayloadCache global_payload_cache;

// Forward declarations for functions being tested
bool execute_load_migrations(DatabaseQueue* db_queue, DatabaseHandle* connection);

// Forward declarations for test functions
void test_execute_load_migrations_success(void);
void test_execute_load_migrations_invalid_migration_path_strdup_failure(void);

/* Miniature Lua sources for the synthetic design. */
static const char* kEngineModule = "return {}";
static const char* kMigrationModule = "return function(engine, design, schema, cfg) return {} end";
static const char* kDatabaseModule =
    "return { defaults = { sqlite = {} },"
    " run_migration = function(self, queries, engine, design, schema)"
    " return \"SELECT 1;\" end }";

static const char* kEngines[] = {"sqlite", "postgresql", "mysql", "mariadb", "mssql", "db2", "firebird"};

/* Install a synthetic cache holding one complete miniature design. */
static void cache_install_design(const char* design) {
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
    global_payload_cache.files[slot].data = (uint8_t*)strdup(kDatabaseModule);
    global_payload_cache.files[slot].size = strlen(kDatabaseModule);
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

static void cache_reset(void) {
    cleanup_payload_cache();
}

/* Minimal lead queue matching the app_config entry these tests install. */
static DatabaseQueue* make_lead_queue(const char* db_name) {
    DatabaseQueue* db_queue = calloc(1, sizeof(DatabaseQueue));
    TEST_ASSERT_NOT_NULL(db_queue);
    db_queue->database_name = strdup(db_name);
    db_queue->queue_type = strdup("Lead");
    db_queue->is_lead_queue = true;
    return db_queue;
}

static void free_lead_queue(DatabaseQueue* db_queue) {
    if (db_queue) {
        free(db_queue->database_name);
        free(db_queue->queue_type);
        free(db_queue);
    }
}

/* Point connections[0] at a fresh config, releasing whatever it held before. */
static void set_connection(const char* name, const char* type, const char* migrations,
                           const char* schema, bool auto_migration) {
    DatabaseConnection* c = &app_config->databases.connections[0];
    free(c->name);
    free(c->type);
    free(c->migrations);
    free(c->schema);
    c->name = strdup(name);
    c->type = strdup(type);
    c->migrations = migrations ? strdup(migrations) : NULL;
    c->schema = schema ? strdup(schema) : NULL;
    c->enabled = true;
    c->auto_migration = auto_migration;
    c->test_migration = true;
    app_config->databases.connection_count = 1;
}

void setUp(void) {
    mock_system_reset_all();
    database_queue_system_init();
    if (!app_config) {
        app_config = load_config(NULL);
    }
    TEST_ASSERT_NOT_NULL(app_config);
    cache_reset();
}

void tearDown(void) {
    cache_reset();
    if (app_config) {
        cleanup_application_config();
        app_config = NULL;
    }
    mock_system_reset_all();
}

// ===== execute_load_migrations TESTS =====

void test_execute_load_migrations_success(void) {
    DatabaseQueue* db_queue = make_lead_queue("testdb");
    DatabaseHandle connection;
    memset(&connection, 0, sizeof(connection));
    connection.engine_type = DB_ENGINE_SQLITE;

    set_connection("testdb", "sqlite", "PAYLOAD:test_design", "test_schema", true);
    cache_install_design("test_design");

    bool result = execute_load_migrations(db_queue, &connection);

    TEST_ASSERT_TRUE(result);

    free_lead_queue(db_queue);
}

void test_execute_load_migrations_invalid_migration_path_strdup_failure(void) {
    DatabaseQueue* db_queue = make_lead_queue("testdb");
    DatabaseHandle connection;
    memset(&connection, 0, sizeof(connection));
    connection.engine_type = DB_ENGINE_SQLITE;

    /* A path (not PAYLOAD:) makes extract_migration_name() strdup the config,
     * and that is the second allocation the call chain performs. */
    set_connection("testdb", "sqlite", "/var/lib/migrations", "test_schema", true);

    mock_system_set_malloc_failure(2);

    bool result = execute_load_migrations(db_queue, &connection);

    TEST_ASSERT_FALSE(result);

    free_lead_queue(db_queue);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_execute_load_migrations_success);
    RUN_TEST(test_execute_load_migrations_invalid_migration_path_strdup_failure);

    return UNITY_END();
}
