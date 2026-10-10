/*
 * Unity Test File: validate_test
 * This file contains unit tests for the migration validation functions in
 * src/database/migration/validate.c
 *
 * Tests cover: validate(), validate_payload_migrations(), validate_path_migrations(),
 * scan_migration_directory(), find_conn_config_for_queue(),
 * and find_latest_available_migration()
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/database.h>
#include <src/database/migration/migration.h>
#include <src/database/dbqueue/dbqueue.h>
#include <src/config/config.h>
#include <src/config/config_databases.h>

#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>

/* Forward declarations for test functions */
void test_validate_null_queue(void);
void test_validate_non_lead_queue(void);
void test_validate_no_app_config(void);
void test_validate_no_matching_database(void);
void test_validate_migrations_disabled(void);
void test_validate_auto_migration_no_migrations_config_field_null(void);
void test_validate_payload_invalid_format(void);
void test_validate_payload_success_calls_validate_payload_migrations(void);
void test_validate_path_no_directory(void);
void test_validate_path_success_with_real_files(void);
void test_validate_find_conn_config_null(void);
void test_validate_no_migrations_config_field_null(void);

void test_validate_payload_migrations_null_config(void);
void test_validate_payload_migrations_null_migrations(void);
void test_validate_payload_migrations_invalid_format(void);
void test_validate_payload_migrations_empty_name(void);
void test_validate_payload_migrations_empty_name_no_colon(void);
void test_validate_path_migrations_null_config(void);
void test_validate_path_migrations_null_migrations(void);
void test_validate_path_migrations_strdup_failure(void);
void test_validate_path_migrations_strdup_failure_second(void);
void test_validate_path_migrations_invalid_path_root(void);
void test_validate_path_migrations_nonexistent_directory(void);
void test_validate_path_migrations_no_matching_files(void);
void test_validate_path_migrations_success_single_file(void);
void test_validate_path_migrations_success_multiple_files(void);
void test_validate_path_migrations_success_unrelated_files_present(void);

void test_scan_migration_directory_null_conn_config(void);
void test_scan_migration_directory_null_migrations(void);
void test_scan_migration_directory_strdup_failure_first(void);
void test_scan_migration_directory_strdup_failure_second(void);
void test_scan_migration_directory_invalid_path_root(void);
void test_scan_migration_directory_nonexistent_directory(void);
void test_scan_migration_directory_no_matching_files(void);
void test_scan_migration_directory_success_single_file_only_first(void);
void test_scan_migration_directory_only_unrelated_files(void);
void test_scan_migration_directory_success_single_file(void);
void test_scan_migration_directory_success_multiple_files(void);
void test_scan_migration_directory_success_latest_differs(void);
void test_scan_migration_directory_file_not_statable(void);

void test_find_conn_config_for_queue_null_queue(void);
void test_find_conn_config_for_queue_no_app_config(void);
void test_find_conn_config_for_queue_no_match(void);
void test_find_conn_config_for_queue_match(void);

void test_find_latest_available_migration_null_queue(void);
void test_find_latest_available_migration_zero_ranges(void);
void test_find_latest_available_migration_with_ranges(void);
void test_find_latest_available_migration_single_range(void);
void test_find_latest_available_migration_no_app_config(void);
void test_find_latest_available_migration_no_conn_config(void);
void test_find_latest_available_migration_null_migrations(void);
void test_find_latest_available_migration_invalid_payload_format(void);

/* Helper to create a temp directory with migration files */
static char* g_test_dir = NULL;

static void create_files_in_dir(const char* dirname, const char** filenames, int count) {
    char fullpath[PATH_MAX];
    for (int i = 0; i < count; i++) {
        snprintf(fullpath, sizeof(fullpath), "%s/%s", dirname, filenames[i]);
        FILE* f = fopen(fullpath, "w");
        if (f) {
            fputs("-- test migration\n", f);
            fclose(f);
        }
    }
}

static void cleanup_test_dir(void) {
    if (g_test_dir) {
        char cmd[PATH_MAX + 32];
        snprintf(cmd, sizeof(cmd), "rm -rf '%s'", g_test_dir);
        system(cmd);
        free(g_test_dir);
        g_test_dir = NULL;
    }
}

static void setup_test_dir(void) {
    cleanup_test_dir();
    const char template[] = "/tmp/test_mig_validate_XXXXXX";
    g_test_dir = strdup(template);
    if (g_test_dir) {
        if (mkdtemp(g_test_dir)) {
            struct stat st;
            if (stat(g_test_dir, &st) != 0) {
                free(g_test_dir);
                g_test_dir = NULL;
            }
        } else {
            free(g_test_dir);
            g_test_dir = NULL;
        }
    }
}

/* Helper to set up an AppConfig with a database connection */
static void setup_test_config(AppConfig* config, const char* db_name,
                              const char* migrations, bool auto_migration) {
    config->databases.connection_count = 1;
    config->databases.connections[0].name = strdup(db_name);
    config->databases.connections[0].enabled = true;
    config->databases.connections[0].auto_migration = auto_migration;
    if (migrations) {
        config->databases.connections[0].migrations = strdup(migrations);
    } else {
        config->databases.connections[0].migrations = NULL;
    }
}

static void cleanup_test_config(AppConfig* config) {
    if (config->databases.connection_count > 0) {
        if (config->databases.connections[0].name) {
            free(config->databases.connections[0].name);
            config->databases.connections[0].name = NULL;
        }
        if (config->databases.connections[0].migrations) {
            free(config->databases.connections[0].migrations);
            config->databases.connections[0].migrations = NULL;
        }
    }
    config->databases.connection_count = 0;
}

/* Helper to create a minimal DatabaseQueue for testing */
static DatabaseQueue* create_test_queue(const char* db_name, bool is_lead) {
    DatabaseQueue* db_queue = calloc(1, sizeof(DatabaseQueue));
    if (!db_queue) return NULL;
    db_queue->database_name = strdup(db_name);
    db_queue->queue_number = 0;
    db_queue->is_lead_queue = is_lead;
    db_queue->tags = strdup("");
    return db_queue;
}

static void free_test_queue(DatabaseQueue* db_queue) {
    if (db_queue) {
        if (db_queue->database_name) free(db_queue->database_name);
        if (db_queue->tags) free(db_queue->tags);
        free(db_queue);
    }
}

/* Helper to create a DatabaseConnection with zeroed memory + migrations string */
static DatabaseConnection make_conn_config(const char* migrations) {
    DatabaseConnection conn_config;
    memset(&conn_config, 0, sizeof(conn_config));
    if (migrations) {
        conn_config.migrations = strdup(migrations);
    }
    return conn_config;
}

/* Helper to build a migrations path inside the temp dir */
static void build_migrations_path(char* buf, size_t bufsize, const char* suffix) {
    snprintf(buf, bufsize, "%s/%s", g_test_dir, suffix);
}

void setUp(void) {
    mock_system_reset_all();
    if (app_config) {
        cleanup_application_config();
        app_config = NULL;
    }
    if (!app_config) {
        app_config = load_config(NULL);
    }
}

void tearDown(void) {
    cleanup_test_dir();
    if (app_config) {
        cleanup_application_config();
        app_config = NULL;
    }
}

/* ===== validate() tests ===== */

void test_validate_null_queue(void) {
    bool result = validate(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_validate_non_lead_queue(void) {
    DatabaseQueue* db_queue = create_test_queue("testdb", false);
    TEST_ASSERT_NOT_NULL(db_queue);

    bool result = validate(db_queue);
    TEST_ASSERT_FALSE(result);

    free_test_queue(db_queue);
}

void test_validate_no_app_config(void) {
    app_config = NULL;

    DatabaseQueue* db_queue = create_test_queue("testdb", true);
    TEST_ASSERT_NOT_NULL(db_queue);

    bool result = validate(db_queue);
    TEST_ASSERT_FALSE(result);

    free_test_queue(db_queue);
    app_config = load_config(NULL);
}

void test_validate_no_matching_database(void) {
    app_config->databases.connection_count = 0;

    DatabaseQueue* db_queue = create_test_queue("nonexistent", true);
    TEST_ASSERT_NOT_NULL(db_queue);

    bool result = validate(db_queue);
    TEST_ASSERT_FALSE(result);

    free_test_queue(db_queue);
}

void test_validate_migrations_disabled(void) {
    setup_test_config(app_config, "testdb", "PAYLOAD:acuranzo", false);

    DatabaseQueue* db_queue = create_test_queue("testdb", true);
    TEST_ASSERT_NOT_NULL(db_queue);

    bool result = validate(db_queue);
    TEST_ASSERT_TRUE(result);

    free_test_queue(db_queue);
    cleanup_test_config(app_config);
}

void test_validate_auto_migration_no_migrations_config_field_null(void) {
    setup_test_config(app_config, "testdb", NULL, true);

    DatabaseQueue* db_queue = create_test_queue("testdb", true);
    TEST_ASSERT_NOT_NULL(db_queue);

    bool result = validate(db_queue);
    TEST_ASSERT_TRUE(result);

    free_test_queue(db_queue);
    cleanup_test_config(app_config);
}

void test_validate_payload_invalid_format(void) {
    setup_test_config(app_config, "testdb", "PAYLOAD:", true);

    DatabaseQueue* db_queue = create_test_queue("testdb", true);
    TEST_ASSERT_NOT_NULL(db_queue);

    bool result = validate(db_queue);
    TEST_ASSERT_FALSE(result);

    free_test_queue(db_queue);
    cleanup_test_config(app_config);
}

void test_validate_payload_success_calls_validate_payload_migrations(void) {
    setup_test_config(app_config, "testdb", "PAYLOAD:nonexistent", true);

    DatabaseQueue* db_queue = create_test_queue("testdb", true);
    TEST_ASSERT_NOT_NULL(db_queue);

    bool result = validate(db_queue);
    TEST_ASSERT_FALSE(result);

    free_test_queue(db_queue);
    cleanup_test_config(app_config);
}

void test_validate_path_no_directory(void) {
    setup_test_config(app_config, "testdb", "/nonexistent/path/migration", true);

    DatabaseQueue* db_queue = create_test_queue("testdb", true);
    TEST_ASSERT_NOT_NULL(db_queue);

    bool result = validate(db_queue);
    TEST_ASSERT_FALSE(result);

    free_test_queue(db_queue);
    cleanup_test_config(app_config);
}

void test_validate_path_success_with_real_files(void) {
    setup_test_dir();
    TEST_ASSERT_NOT_NULL(g_test_dir);

    char migrations_path[PATH_MAX];
    build_migrations_path(migrations_path, sizeof(migrations_path), "migration");

    const char* files[] = {
        "migration_000001.lua",
        "migration_000002.lua",
        "migration_000003.lua"
    };
    create_files_in_dir(g_test_dir, files, 3);

    setup_test_config(app_config, "testdb", migrations_path, true);

    DatabaseQueue* db_queue = create_test_queue("testdb", true);
    TEST_ASSERT_NOT_NULL(db_queue);

    bool result = validate(db_queue);
    TEST_ASSERT_TRUE(result);

    free_test_queue(db_queue);
    cleanup_test_config(app_config);
}

void test_validate_find_conn_config_null(void) {
    setup_test_config(app_config, "testdb", "PAYLOAD:", true);

    DatabaseQueue* db_queue = create_test_queue("wrongdb", true);
    TEST_ASSERT_NOT_NULL(db_queue);

    bool result = validate(db_queue);
    TEST_ASSERT_FALSE(result);

    free_test_queue(db_queue);
    cleanup_test_config(app_config);
}

void test_validate_no_migrations_config_field_null(void) {
    setup_test_config(app_config, "testdb", "PAYLOAD:acuranzo", true);
    app_config->databases.connections[0].migrations = NULL;

    DatabaseQueue* db_queue = create_test_queue("testdb", true);
    TEST_ASSERT_NOT_NULL(db_queue);

    bool result = validate(db_queue);
    TEST_ASSERT_TRUE(result);

    free_test_queue(db_queue);
    cleanup_test_config(app_config);
    free(app_config->databases.connections[0].name);
    app_config->databases.connections[0].name = NULL;
}

/* ===== validate_payload_migrations() tests ===== */

void test_validate_payload_migrations_null_config(void) {
    bool result = validate_payload_migrations(NULL, "test_label");
    TEST_ASSERT_FALSE(result);
}

void test_validate_payload_migrations_null_migrations(void) {
    DatabaseConnection conn_config = make_conn_config(NULL);

    bool result = validate_payload_migrations(&conn_config, "test_label");
    TEST_ASSERT_FALSE(result);
}

void test_validate_payload_migrations_invalid_format(void) {
    DatabaseConnection conn_config = make_conn_config("path/to/migrations");

    bool result = validate_payload_migrations(&conn_config, "test_label");
    TEST_ASSERT_FALSE(result);

    free(conn_config.migrations);
}

void test_validate_payload_migrations_empty_name(void) {
    DatabaseConnection conn_config = make_conn_config("PAYLOAD:");

    bool result = validate_payload_migrations(&conn_config, "test_label");
    TEST_ASSERT_FALSE(result);

    free(conn_config.migrations);
}

void test_validate_payload_migrations_empty_name_no_colon(void) {
    DatabaseConnection conn_config = make_conn_config("PAYLOAD");

    bool result = validate_payload_migrations(&conn_config, "test_label");
    TEST_ASSERT_FALSE(result);

    free(conn_config.migrations);
}

/* ===== validate_path_migrations() tests ===== */

void test_validate_path_migrations_null_config(void) {
    bool result = validate_path_migrations(NULL, "test_label");
    TEST_ASSERT_FALSE(result);
}

void test_validate_path_migrations_null_migrations(void) {
    DatabaseConnection conn_config = make_conn_config(NULL);

    bool result = validate_path_migrations(&conn_config, "test_label");
    TEST_ASSERT_FALSE(result);
}

void test_validate_path_migrations_strdup_failure(void) {
    DatabaseConnection conn_config = make_conn_config("/tmp/test");
    TEST_ASSERT_NOT_NULL(conn_config.migrations);

    mock_system_set_malloc_failure(1);

    bool result = validate_path_migrations(&conn_config, "test_label");
    TEST_ASSERT_FALSE(result);

    mock_system_reset_all();
    free(conn_config.migrations);
}

void test_validate_path_migrations_strdup_failure_second(void) {
    DatabaseConnection conn_config = make_conn_config("/tmp/test");
    TEST_ASSERT_NOT_NULL(conn_config.migrations);

    mock_system_set_malloc_failure(2);

    bool result = validate_path_migrations(&conn_config, "test_label");
    TEST_ASSERT_FALSE(result);

    mock_system_reset_all();
    free(conn_config.migrations);
}

void test_validate_path_migrations_invalid_path_root(void) {
    DatabaseConnection conn_config = make_conn_config("/");

    bool result = validate_path_migrations(&conn_config, "test_label");
    TEST_ASSERT_FALSE(result);

    free(conn_config.migrations);
}

void test_validate_path_migrations_nonexistent_directory(void) {
    DatabaseConnection conn_config = make_conn_config("/nonexistent/dir/migration_000001");

    bool result = validate_path_migrations(&conn_config, "test_label");
    TEST_ASSERT_FALSE(result);

    free(conn_config.migrations);
}

void test_validate_path_migrations_no_matching_files(void) {
    setup_test_dir();
    TEST_ASSERT_NOT_NULL(g_test_dir);

    char migrations_path[PATH_MAX];
    build_migrations_path(migrations_path, sizeof(migrations_path), "notexist_000001");

    DatabaseConnection conn_config = make_conn_config(migrations_path);

    bool result = validate_path_migrations(&conn_config, "test_label");
    TEST_ASSERT_FALSE(result);

    free(conn_config.migrations);
}

void test_validate_path_migrations_success_single_file(void) {
    setup_test_dir();
    TEST_ASSERT_NOT_NULL(g_test_dir);

    char migrations_path[PATH_MAX];
    build_migrations_path(migrations_path, sizeof(migrations_path), "migration");

    const char* files[] = {"migration_000001.lua"};
    create_files_in_dir(g_test_dir, files, 1);

    DatabaseConnection conn_config = make_conn_config(migrations_path);

    bool result = validate_path_migrations(&conn_config, "test_label");
    TEST_ASSERT_TRUE(result);

    free(conn_config.migrations);
}

void test_validate_path_migrations_success_multiple_files(void) {
    setup_test_dir();
    TEST_ASSERT_NOT_NULL(g_test_dir);

    char migrations_path[PATH_MAX];
    build_migrations_path(migrations_path, sizeof(migrations_path), "migration");

    const char* files[] = {
        "migration_000001.lua",
        "migration_000002.lua",
        "migration_000003.lua"
    };
    create_files_in_dir(g_test_dir, files, 3);

    DatabaseConnection conn_config = make_conn_config(migrations_path);

    bool result = validate_path_migrations(&conn_config, "test_label");
    TEST_ASSERT_TRUE(result);

    free(conn_config.migrations);
}

void test_validate_path_migrations_success_unrelated_files_present(void) {
    setup_test_dir();
    TEST_ASSERT_NOT_NULL(g_test_dir);

    char migrations_path[PATH_MAX];
    build_migrations_path(migrations_path, sizeof(migrations_path), "migration");

    const char* files[] = {
        "migration_000001.lua",
        "migration_000002.lua",
        "other_file.txt",
        "not_migration_001.lua",
        "readme.md"
    };
    create_files_in_dir(g_test_dir, files, 5);

    DatabaseConnection conn_config = make_conn_config(migrations_path);

    bool result = validate_path_migrations(&conn_config, "test_label");
    TEST_ASSERT_TRUE(result);

    free(conn_config.migrations);
}

/* ===== scan_migration_directory() tests ===== */

/* Helper for scan_migration_directory tests: set up temp dir with files,
   run scan, and free outputs */
static void run_scan_test(const char* path_suffix, const char** files,
                          int file_count, bool expect_result,
                          char** out_first, char** out_latest) {
    setup_test_dir();
    TEST_ASSERT_NOT_NULL(g_test_dir);

    char migrations_path[PATH_MAX];
    build_migrations_path(migrations_path, sizeof(migrations_path), path_suffix);

    if (files && file_count > 0) {
        create_files_in_dir(g_test_dir, files, file_count);
    }

    DatabaseConnection conn_config = make_conn_config(migrations_path);

    bool result = scan_migration_directory(&conn_config, out_first, out_latest, "test");
    TEST_ASSERT_EQUAL(expect_result, result);

    free(conn_config.migrations);
}

void test_scan_migration_directory_null_conn_config(void) {
    char* first_file = NULL;
    char* latest_file = NULL;
    bool result = scan_migration_directory(NULL, &first_file, &latest_file, "test");
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(first_file);
    TEST_ASSERT_NULL(latest_file);
}

void test_scan_migration_directory_null_migrations(void) {
    DatabaseConnection conn_config = make_conn_config(NULL);

    char* first_file = NULL;
    char* latest_file = NULL;
    bool result = scan_migration_directory(&conn_config, &first_file, &latest_file, "test");
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(first_file);
    TEST_ASSERT_NULL(latest_file);
}

void test_scan_migration_directory_strdup_failure_first(void) {
    DatabaseConnection conn_config = make_conn_config("/tmp/test");
    TEST_ASSERT_NOT_NULL(conn_config.migrations);

    mock_system_set_malloc_failure(1);

    char* first_file = NULL;
    char* latest_file = NULL;
    bool result = scan_migration_directory(&conn_config, &first_file, &latest_file, "test");
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(first_file);
    TEST_ASSERT_NULL(latest_file);

    mock_system_reset_all();
    free(conn_config.migrations);
}

void test_scan_migration_directory_strdup_failure_second(void) {
    DatabaseConnection conn_config = make_conn_config("/tmp/test");
    TEST_ASSERT_NOT_NULL(conn_config.migrations);

    mock_system_set_malloc_failure(2);

    char* first_file = NULL;
    char* latest_file = NULL;
    bool result = scan_migration_directory(&conn_config, &first_file, &latest_file, "test");
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(first_file);
    TEST_ASSERT_NULL(latest_file);

    mock_system_reset_all();
    free(conn_config.migrations);
}

void test_scan_migration_directory_invalid_path_root(void) {
    DatabaseConnection conn_config = make_conn_config("/");

    char* first_file = NULL;
    char* latest_file = NULL;
    bool result = scan_migration_directory(&conn_config, &first_file, &latest_file, "test");
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(first_file);
    TEST_ASSERT_NULL(latest_file);

    free(conn_config.migrations);
}

void test_scan_migration_directory_nonexistent_directory(void) {
    DatabaseConnection conn_config = make_conn_config("/nonexistent/path/migration_000001");

    char* first_file = NULL;
    char* latest_file = NULL;
    bool result = scan_migration_directory(&conn_config, &first_file, &latest_file, "test");
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(first_file);
    TEST_ASSERT_NULL(latest_file);

    free(conn_config.migrations);
}

void test_scan_migration_directory_no_matching_files(void) {
    const char* files[] = {"other_file.txt", "readme.md"};
    char* first_file = NULL;
    char* latest_file = NULL;
    run_scan_test("migration", files, 2, false, &first_file, &latest_file);
    TEST_ASSERT_NULL(first_file);
    TEST_ASSERT_NULL(latest_file);
}

void test_scan_migration_directory_success_single_file_only_first(void) {
    const char* files[] = {"migration_000001.lua"};
    char* first_file = NULL;
    bool result = true;
    /* Use a local setup since we need NULL for latest_file */
    setup_test_dir();
    TEST_ASSERT_NOT_NULL(g_test_dir);

    char migrations_path[PATH_MAX];
    build_migrations_path(migrations_path, sizeof(migrations_path), "migration");

    create_files_in_dir(g_test_dir, files, 1);

    DatabaseConnection conn_config = make_conn_config(migrations_path);

    result = scan_migration_directory(&conn_config, &first_file, NULL, "test");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(first_file);

    free(first_file);
    free(conn_config.migrations);
}

void test_scan_migration_directory_only_unrelated_files(void) {
    const char* files[] = {
        "other_file.txt",
        "not_migration_001.lua",
        "readme.md"
    };
    char* first_file = NULL;
    char* latest_file = NULL;
    run_scan_test("migration", files, 3, false, &first_file, &latest_file);
    TEST_ASSERT_NULL(first_file);
    TEST_ASSERT_NULL(latest_file);
}

void test_scan_migration_directory_success_single_file(void) {
    const char* files[] = {"migration_000001.lua"};
    char* first_file = NULL;
    char* latest_file = NULL;
    run_scan_test("migration", files, 1, true, &first_file, &latest_file);
    TEST_ASSERT_NOT_NULL(first_file);
    TEST_ASSERT_NOT_NULL(strstr(first_file, "migration_000001.lua"));

    free(first_file);
    free(latest_file);
}

void test_scan_migration_directory_success_multiple_files(void) {
    const char* files[] = {
        "migration_000003.lua",
        "migration_000001.lua",
        "migration_000002.lua"
    };
    char* first_file = NULL;
    char* latest_file = NULL;
    run_scan_test("migration", files, 3, true, &first_file, &latest_file);
    TEST_ASSERT_NOT_NULL(first_file);
    TEST_ASSERT_NOT_NULL(latest_file);
    TEST_ASSERT_NOT_NULL(strstr(first_file, "migration_000001.lua"));

    free(first_file);
    free(latest_file);
}

void test_scan_migration_directory_success_latest_differs(void) {
    const char* files[] = {
        "migration_000001.lua",
        "migration_000005.lua",
        "migration_000003.lua"
    };
    char* first_file = NULL;
    char* latest_file = NULL;
    run_scan_test("migration", files, 3, true, &first_file, &latest_file);
    TEST_ASSERT_NOT_NULL(first_file);
    TEST_ASSERT_NOT_NULL(latest_file);
    TEST_ASSERT_NOT_NULL(strstr(first_file, "migration_000001.lua"));
    TEST_ASSERT_NOT_NULL(strstr(latest_file, "migration_000005.lua"));

    free(first_file);
    free(latest_file);
}

void test_scan_migration_directory_file_not_statable(void) {
    const char* files[] = {"migration_000001.lua"};
    char* first_file = NULL;
    char* latest_file = NULL;
    run_scan_test("migration", files, 1, true, &first_file, &latest_file);
    TEST_ASSERT_NOT_NULL(first_file);

    free(first_file);
    free(latest_file);
}

/* ===== find_conn_config_for_queue() tests ===== */

void test_find_conn_config_for_queue_null_queue(void) {
    const DatabaseConnection* result = find_conn_config_for_queue(NULL);
    TEST_ASSERT_NULL(result);
}

void test_find_conn_config_for_queue_no_app_config(void) {
    app_config = NULL;

    DatabaseQueue* db_queue = create_test_queue("testdb", true);
    const DatabaseConnection* result = find_conn_config_for_queue(db_queue);
    TEST_ASSERT_NULL(result);

    free_test_queue(db_queue);
    app_config = load_config(NULL);
}

void test_find_conn_config_for_queue_no_match(void) {
    setup_test_config(app_config, "testdb", "PAYLOAD:acuranzo", true);

    DatabaseQueue* db_queue = create_test_queue("otherdb", true);
    const DatabaseConnection* result = find_conn_config_for_queue(db_queue);
    TEST_ASSERT_NULL(result);

    free_test_queue(db_queue);
    cleanup_test_config(app_config);
}

void test_find_conn_config_for_queue_match(void) {
    setup_test_config(app_config, "testdb", "PAYLOAD:acuranzo", true);

    DatabaseQueue* db_queue = create_test_queue("testdb", true);
    const DatabaseConnection* result = find_conn_config_for_queue(db_queue);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("testdb", result->name);

    free_test_queue(db_queue);
    cleanup_test_config(app_config);
}

/* ===== find_latest_available_migration() tests ===== */

void test_find_latest_available_migration_null_queue(void) {
    long long result = find_latest_available_migration(NULL);
    TEST_ASSERT_EQUAL(-1, result);
}

void test_find_latest_available_migration_zero_ranges(void) {
    DatabaseQueue* db_queue = create_test_queue("testdb", true);
    db_queue->migration_range_count = 0;

    long long result = find_latest_available_migration(db_queue);
    TEST_ASSERT_EQUAL(-1, result);

    free_test_queue(db_queue);
}

void test_find_latest_available_migration_with_ranges(void) {
    DatabaseQueue* db_queue = create_test_queue("testdb", true);
    db_queue->migration_range_count = 2;
    db_queue->migration_ranges[0].available = 1001;
    db_queue->migration_ranges[1].available = 2003;

    long long result = find_latest_available_migration(db_queue);
    TEST_ASSERT_EQUAL(2003, result);

    free_test_queue(db_queue);
}

void test_find_latest_available_migration_single_range(void) {
    DatabaseQueue* db_queue = create_test_queue("testdb", true);
    db_queue->migration_range_count = 1;
    db_queue->migration_ranges[0].available = 1500;

    long long result = find_latest_available_migration(db_queue);
    TEST_ASSERT_EQUAL(1500, result);

    free_test_queue(db_queue);
}

void test_find_latest_available_migration_no_app_config(void) {
    app_config = NULL;

    DatabaseQueue* db_queue = create_test_queue("testdb", true);
    db_queue->migration_range_count = 0;

    long long result = find_latest_available_migration(db_queue);
    TEST_ASSERT_EQUAL(-1, result);

    free_test_queue(db_queue);
    app_config = load_config(NULL);
}

void test_find_latest_available_migration_no_conn_config(void) {
    app_config->databases.connection_count = 0;

    DatabaseQueue* db_queue = create_test_queue("testdb", true);
    db_queue->migration_range_count = 0;

    long long result = find_latest_available_migration(db_queue);
    TEST_ASSERT_EQUAL(-1, result);

    free_test_queue(db_queue);
}

void test_find_latest_available_migration_null_migrations(void) {
    setup_test_config(app_config, "testdb", NULL, true);

    DatabaseQueue* db_queue = create_test_queue("testdb", true);
    db_queue->migration_range_count = 0;

    long long result = find_latest_available_migration(db_queue);
    TEST_ASSERT_EQUAL(-1, result);

    free_test_queue(db_queue);
    cleanup_test_config(app_config);
}

void test_find_latest_available_migration_invalid_payload_format(void) {
    setup_test_config(app_config, "testdb", "/some/path", true);

    DatabaseQueue* db_queue = create_test_queue("testdb", true);
    db_queue->migration_range_count = 0;

    long long result = find_latest_available_migration(db_queue);
    TEST_ASSERT_EQUAL(-1, result);

    free_test_queue(db_queue);
    cleanup_test_config(app_config);
}

int main(void) {
    UNITY_BEGIN();

    /* validate() tests */
    RUN_TEST(test_validate_null_queue);
    RUN_TEST(test_validate_non_lead_queue);
    RUN_TEST(test_validate_no_app_config);
    RUN_TEST(test_validate_no_matching_database);
    RUN_TEST(test_validate_migrations_disabled);
    RUN_TEST(test_validate_auto_migration_no_migrations_config_field_null);
    RUN_TEST(test_validate_payload_invalid_format);
    RUN_TEST(test_validate_payload_success_calls_validate_payload_migrations);
    RUN_TEST(test_validate_path_no_directory);
    RUN_TEST(test_validate_path_success_with_real_files);
    RUN_TEST(test_validate_find_conn_config_null);
    RUN_TEST(test_validate_no_migrations_config_field_null);

    /* validate_payload_migrations() tests */
    RUN_TEST(test_validate_payload_migrations_null_config);
    RUN_TEST(test_validate_payload_migrations_null_migrations);
    RUN_TEST(test_validate_payload_migrations_invalid_format);
    RUN_TEST(test_validate_payload_migrations_empty_name);
    RUN_TEST(test_validate_payload_migrations_empty_name_no_colon);

    /* validate_path_migrations() tests */
    RUN_TEST(test_validate_path_migrations_null_config);
    RUN_TEST(test_validate_path_migrations_null_migrations);
    RUN_TEST(test_validate_path_migrations_strdup_failure);
    RUN_TEST(test_validate_path_migrations_strdup_failure_second);
    RUN_TEST(test_validate_path_migrations_invalid_path_root);
    RUN_TEST(test_validate_path_migrations_nonexistent_directory);
    RUN_TEST(test_validate_path_migrations_no_matching_files);
    RUN_TEST(test_validate_path_migrations_success_single_file);
    RUN_TEST(test_validate_path_migrations_success_multiple_files);
    RUN_TEST(test_validate_path_migrations_success_unrelated_files_present);

    /* scan_migration_directory() tests */
    RUN_TEST(test_scan_migration_directory_null_conn_config);
    RUN_TEST(test_scan_migration_directory_null_migrations);
    RUN_TEST(test_scan_migration_directory_strdup_failure_first);
    RUN_TEST(test_scan_migration_directory_strdup_failure_second);
    RUN_TEST(test_scan_migration_directory_invalid_path_root);
    RUN_TEST(test_scan_migration_directory_nonexistent_directory);
    RUN_TEST(test_scan_migration_directory_no_matching_files);
    RUN_TEST(test_scan_migration_directory_success_single_file_only_first);
    RUN_TEST(test_scan_migration_directory_only_unrelated_files);
    RUN_TEST(test_scan_migration_directory_success_single_file);
    RUN_TEST(test_scan_migration_directory_success_multiple_files);
    RUN_TEST(test_scan_migration_directory_success_latest_differs);
    RUN_TEST(test_scan_migration_directory_file_not_statable);

    /* find_conn_config_for_queue() tests */
    RUN_TEST(test_find_conn_config_for_queue_null_queue);
    RUN_TEST(test_find_conn_config_for_queue_no_app_config);
    RUN_TEST(test_find_conn_config_for_queue_no_match);
    RUN_TEST(test_find_conn_config_for_queue_match);

    /* find_latest_available_migration() tests */
    RUN_TEST(test_find_latest_available_migration_null_queue);
    RUN_TEST(test_find_latest_available_migration_zero_ranges);
    RUN_TEST(test_find_latest_available_migration_with_ranges);
    RUN_TEST(test_find_latest_available_migration_single_range);
    RUN_TEST(test_find_latest_available_migration_no_app_config);
    RUN_TEST(test_find_latest_available_migration_no_conn_config);
    RUN_TEST(test_find_latest_available_migration_null_migrations);
    RUN_TEST(test_find_latest_available_migration_invalid_payload_format);

    return UNITY_END();
}
