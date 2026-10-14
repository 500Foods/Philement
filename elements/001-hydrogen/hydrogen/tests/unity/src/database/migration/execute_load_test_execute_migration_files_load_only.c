/*
 * Unity Test File: execute_load_test_execute_migration_files_load_only
 * Unit tests for execute_migration_files_load_only() in execute_load.c,
 * covering the PAYLOAD+ list handling, the skip logic and the ref extraction.
 *
 * These tests install a synthetic payload cache that holds a miniature Lua
 * design (database.lua, one database_<engine>.lua per engine and one numbered
 * migration file) so the real Lua loader in lua.c can run end to end.
 */

#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/migration/migration.h>
#include <src/database/dbqueue/dbqueue.h>
#include <src/payload/payload.h>
#include <src/payload/payload_cache.h>

// The payload cache is a plain global so tests can install a synthetic list
extern PayloadCache global_payload_cache;

// Forward declarations for functions being tested
bool execute_migration_files_load_only(DatabaseQueue* db_queue, DatabaseHandle* connection, char** migration_files,
                                      size_t migration_count, const char* engine_name,
                                      const char* migration_name, const char* schema_name,
                                      const char* dqm_label);

// Forward declarations for test functions
void test_execute_migration_files_load_only_plus_list_spec_too_long(void);
void test_execute_migration_files_load_only_plus_list_invalid_design(void);
void test_execute_migration_files_load_only_plus_list_no_payload_files(void);
void test_execute_migration_files_load_only_plus_list_uses_per_file_design(void);
void test_execute_migration_files_load_only_ref_extraction_failure(void);
void test_execute_migration_files_load_only_skips_already_loaded(void);
void test_execute_migration_files_load_only_applied_becomes_floor(void);
void test_execute_migration_files_load_only_band_not_in_payload(void);
void test_execute_migration_files_load_only_null_files_with_count(void);

/* Miniature Lua sources for the synthetic design. */
static const char* kEngineModule = "return {}";
static const char* kMigrationModule = "return function(engine, design, schema, cfg) return {} end";
static const char* kDatabaseModule =
    "return { defaults = { sqlite = {} },"
    " run_migration = function(self, queries, engine, design, schema)"
    " return \"SELECT 1;\" end }";

static const char* kEngines[] = {"sqlite", "postgresql", "mysql", "mariadb", "mssql", "db2", "firebird"};

/* Install a synthetic cache holding one complete miniature design. The array
 * has spare slots so a test can append extra design files. */
static void cache_install_design(const char* design) {
    char name[128];

    cleanup_payload_cache();
    initialize_payload_cache();
    global_payload_cache.is_initialized = true;
    global_payload_cache.is_available = true;
    global_payload_cache.capacity = 16;
    global_payload_cache.num_files = 9;
    global_payload_cache.files = calloc(16, sizeof(PayloadFile));
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

/* Install an available but empty cache so lookups succeed with zero files. */
static void cache_install_empty(void) {
    cleanup_payload_cache();
    initialize_payload_cache();
    global_payload_cache.is_initialized = true;
    global_payload_cache.is_available = true;
    global_payload_cache.capacity = 1;
    global_payload_cache.num_files = 0;
    global_payload_cache.files = calloc(1, sizeof(PayloadFile));
    TEST_ASSERT_NOT_NULL(global_payload_cache.files);
}

/* Add a single file to an already installed cache. */
static void cache_append_file(const char* name, const char* content) {
    global_payload_cache.files[global_payload_cache.num_files].name = strdup(name);
    global_payload_cache.files[global_payload_cache.num_files].data = (uint8_t*)strdup(content);
    global_payload_cache.files[global_payload_cache.num_files].size = strlen(content);
    global_payload_cache.files[global_payload_cache.num_files].is_compressed = false;
    global_payload_cache.num_files++;
}

static void cache_reset(void) {
    cleanup_payload_cache();
}

static DatabaseQueue make_queue(void) {
    DatabaseQueue db_queue;
    memset(&db_queue, 0, sizeof(db_queue));
    return db_queue;
}

static DatabaseHandle make_connection(void) {
    DatabaseHandle connection;
    memset(&connection, 0, sizeof(connection));
    connection.engine_type = DB_ENGINE_SQLITE;
    return connection;
}

/* Build a strdup'd file list the caller owns. */
static char** make_file_list(const char** names, size_t count) {
    char** files = calloc(count, sizeof(char*));
    TEST_ASSERT_NOT_NULL(files);
    for (size_t i = 0; i < count; i++) {
        files[i] = strdup(names[i]);
        TEST_ASSERT_NOT_NULL(files[i]);
    }
    return files;
}

static void free_file_list(char** files, size_t count) {
    for (size_t i = 0; i < count; i++) {
        free(files[i]);
    }
    free(files);
}

void setUp(void) {
    mock_system_reset_all();
    cache_reset();
}

void tearDown(void) {
    cache_reset();
    mock_system_reset_all();
}

// ===== execute_migration_files_load_only TESTS =====

void test_execute_migration_files_load_only_plus_list_spec_too_long(void) {
    char long_name[300];
    memset(long_name, 'a', sizeof(long_name) - 1);
    long_name[sizeof(long_name) - 1] = '\0';
    long_name[10] = '+';

    DatabaseQueue db_queue = make_queue();
    DatabaseHandle connection = make_connection();

    bool result = execute_migration_files_load_only(&db_queue, &connection, NULL, 0, "sqlite",
                                                    long_name, "test_schema", "test-label");

    TEST_ASSERT_FALSE(result);
}

void test_execute_migration_files_load_only_plus_list_invalid_design(void) {
    DatabaseQueue db_queue = make_queue();
    DatabaseHandle connection = make_connection();

    /* "PAYLOAD:+bad" is rejected: the first name is empty. */
    bool result = execute_migration_files_load_only(&db_queue, &connection, NULL, 0, "sqlite",
                                                    "+bad", "test_schema", "test-label");

    TEST_ASSERT_FALSE(result);
}

void test_execute_migration_files_load_only_plus_list_no_payload_files(void) {
    DatabaseQueue db_queue = make_queue();
    DatabaseHandle connection = make_connection();
    const char* names[] = {"good/good_1001.lua"};
    char** files = make_file_list(names, 1);

    cache_install_empty();

    /* The design list parses, but no payload files match "good". */
    bool result = execute_migration_files_load_only(&db_queue, &connection, files, 1, "sqlite",
                                                    "good+design", "test_schema", "test-label");

    TEST_ASSERT_FALSE(result);

    free_file_list(files, 1);
}

void test_execute_migration_files_load_only_plus_list_uses_per_file_design(void) {
    DatabaseQueue db_queue = make_queue();
    db_queue.latest_loaded_migration = 0;
    DatabaseHandle connection = make_connection();
    const char* names[] = {"test_design/test_design_1001.lua", "other/other_2001.lua"};
    char** files = make_file_list(names, 2);

    cache_install_design("test_design");
    cache_append_file("other/other_2001.lua", "return 5");

    /* The plus list loads both designs, and each file is executed with the
     * design parsed out of its own path. The "other" design has no Lua module,
     * so the second migration fails and the batch stops. */
    bool result = execute_migration_files_load_only(&db_queue, &connection, files, 2, "sqlite",
                                                    "test_design+other", "test_schema", "test-label");

    TEST_ASSERT_FALSE(result);

    free_file_list(files, 2);
}

void test_execute_migration_files_load_only_ref_extraction_failure(void) {
    DatabaseQueue db_queue = make_queue();
    DatabaseHandle connection = make_connection();
    const char* names[] = {"test_design/test_design_bad.lua"};
    char** files = make_file_list(names, 1);

    cache_install_design("test_design");

    bool result = execute_migration_files_load_only(&db_queue, &connection, files, 1, "sqlite",
                                                    "test_design", "test_schema", "test-label");

    TEST_ASSERT_FALSE(result);

    free_file_list(files, 1);
}

void test_execute_migration_files_load_only_skips_already_loaded(void) {
    DatabaseQueue db_queue = make_queue();
    db_queue.latest_loaded_migration = 2000;
    DatabaseHandle connection = make_connection();
    const char* names[] = {"test_design/test_design_1001.lua"};
    char** files = make_file_list(names, 1);

    cache_install_design("test_design");

    bool result = execute_migration_files_load_only(&db_queue, &connection, files, 1, "sqlite",
                                                    "test_design", "test_schema", "test-label");

    TEST_ASSERT_TRUE(result);

    free_file_list(files, 1);
}

void test_execute_migration_files_load_only_applied_becomes_floor(void) {
    DatabaseQueue db_queue = make_queue();
    db_queue.latest_loaded_migration = 500;
    db_queue.latest_applied_migration = 1500;
    DatabaseHandle connection = make_connection();
    const char* names[] = {"test_design/test_design_1001.lua"};
    char** files = make_file_list(names, 1);

    cache_install_design("test_design");

    /* The applied watermark is higher, so it becomes the floor. */
    bool result = execute_migration_files_load_only(&db_queue, &connection, files, 1, "sqlite",
                                                    "test_design", "test_schema", "test-label");

    TEST_ASSERT_TRUE(result);

    free_file_list(files, 1);
}

void test_execute_migration_files_load_only_band_not_in_payload(void) {
    DatabaseQueue db_queue = make_queue();
    db_queue.migration_range_count = 1;
    db_queue.migration_ranges[0].band = 1;
    db_queue.migration_ranges[0].loaded = 1004;
    DatabaseHandle connection = make_connection();
    const char* names[] = {"test_design/test_design_2001.lua"};
    char** files = make_file_list(names, 1);

    cache_install_design("test_design");

    /* Band 2 has no range in this queue, so its floor is -1 and the file is
     * skipped rather than executed. */
    bool result = execute_migration_files_load_only(&db_queue, &connection, files, 1, "sqlite",
                                                    "test_design", "test_schema", "test-label");

    TEST_ASSERT_TRUE(result);

    free_file_list(files, 1);
}

void test_execute_migration_files_load_only_null_files_with_count(void) {
    DatabaseQueue db_queue = make_queue();
    DatabaseHandle connection = make_connection();

    bool result = execute_migration_files_load_only(&db_queue, &connection, NULL, 3, "sqlite",
                                                    "test_design", "test_schema", "test-label");

    TEST_ASSERT_FALSE(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_execute_migration_files_load_only_plus_list_spec_too_long);
    RUN_TEST(test_execute_migration_files_load_only_plus_list_invalid_design);
    RUN_TEST(test_execute_migration_files_load_only_plus_list_no_payload_files);
    RUN_TEST(test_execute_migration_files_load_only_plus_list_uses_per_file_design);
    RUN_TEST(test_execute_migration_files_load_only_ref_extraction_failure);
    RUN_TEST(test_execute_migration_files_load_only_skips_already_loaded);
    RUN_TEST(test_execute_migration_files_load_only_applied_becomes_floor);
    RUN_TEST(test_execute_migration_files_load_only_band_not_in_payload);
    RUN_TEST(test_execute_migration_files_load_only_null_files_with_count);

    return UNITY_END();
}
