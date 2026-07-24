/*
 * Unity Test File: database.c core path coverage
 * Covers health_check / reload_config / readiness / queue counts /
 * process_api_query / validate / escape error and success branches.
 */

#include <src/hydrogen.h>
#include <unity.h>

#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

#include <src/database/database.h>
#include <src/database/dbqueue/dbqueue.h>
#include <src/config/config_databases.h>

extern DatabaseSubsystem* database_subsystem;
extern DatabaseQueueManager* global_queue_manager;
extern AppConfig* app_config;

void test_health_check_shutdown_flag(void);
void test_health_check_with_healthy_lead(void);
void test_health_check_with_shutdown_lead(void);
void test_health_check_skips_non_lead(void);
void test_subsystem_init_calloc_failure(void);
void test_subsystem_shutdown_destroys_queue_manager(void);
void test_reload_config_shutdown_flag(void);
void test_reload_config_add_missing_connection(void);
void test_reload_config_skips_disabled_and_present(void);
void test_reload_config_removes_orphaned_queue(void);
void test_process_api_query_invalid_template(void);
void test_process_api_query_empty_database_name(void);
void test_process_api_query_with_registered_db_no_pending(void);
void test_validate_query_too_large(void);
void test_escape_parameter_malloc_failure(void);
void test_get_total_queue_count_with_children(void);
void test_get_queue_counts_by_type_with_children(void);
void test_get_readiness_null(void);
void test_get_readiness_no_manager(void);
void test_get_readiness_with_leads(void);
void test_get_readiness_unknown_name(void);
void test_all_leads_ready_wrapper(void);

static AppConfig* saved_app_config;
static DatabaseQueueManager* saved_manager;

static void init_stub_sync(DatabaseQueue* q) {
    TEST_ASSERT_EQUAL(0, pthread_mutex_init(&q->queue_access_lock, NULL));
    TEST_ASSERT_EQUAL(0, pthread_mutex_init(&q->children_lock, NULL));
    TEST_ASSERT_EQUAL(0, pthread_mutex_init(&q->connection_lock, NULL));
    TEST_ASSERT_EQUAL(0, pthread_mutex_init(&q->bootstrap_lock, NULL));
    TEST_ASSERT_EQUAL(0, pthread_mutex_init(&q->initial_connection_lock, NULL));
    TEST_ASSERT_EQUAL(0, pthread_cond_init(&q->bootstrap_cond, NULL));
    TEST_ASSERT_EQUAL(0, pthread_cond_init(&q->initial_connection_cond, NULL));
    TEST_ASSERT_EQUAL(0, sem_init(&q->worker_semaphore, 0, 0));
}

static DatabaseQueue* make_stub_queue(const char* name, bool lead, bool shutdown, bool ready) {
    DatabaseQueue* q = calloc(1, sizeof(DatabaseQueue));
    TEST_ASSERT_NOT_NULL(q);
    if (name) {
        q->database_name = strdup(name);
        TEST_ASSERT_NOT_NULL(q->database_name);
    }
    q->queue_type = strdup(lead ? "Lead" : "fast");
    TEST_ASSERT_NOT_NULL(q->queue_type);
    q->is_lead_queue = lead;
    q->shutdown_requested = shutdown;
    q->conductor_sequence_completed = ready;
    q->child_queue_count = 0;
    q->child_queues = NULL;
    q->worker_thread_started = false;
    init_stub_sync(q);
    return q;
}

static DatabaseQueue* make_child_queue(const char* type) {
    DatabaseQueue* q = calloc(1, sizeof(DatabaseQueue));
    TEST_ASSERT_NOT_NULL(q);
    q->queue_type = strdup(type);
    TEST_ASSERT_NOT_NULL(q->queue_type);
    q->is_lead_queue = false;
    q->worker_thread_started = false;
    init_stub_sync(q);
    return q;
}

static void install_manager_with_queue(DatabaseQueue* q) {
    global_queue_manager = database_queue_manager_create(4);
    TEST_ASSERT_NOT_NULL(global_queue_manager);
    global_queue_manager->databases[0] = q;
    global_queue_manager->database_count = 1;
}

/* Destroy via manager so stub mutexes are cleaned by database_queue_destroy. */
static void teardown_manager_manual(void) {
    if (!global_queue_manager) {
        return;
    }
    database_queue_manager_destroy(global_queue_manager);
    global_queue_manager = NULL;
}

void setUp(void) {
    mock_system_reset_all();
    saved_app_config = app_config;
    saved_manager = global_queue_manager;
    app_config = NULL;
    global_queue_manager = NULL;
    database_subsystem_init();
}

void tearDown(void) {
    mock_system_reset_all();
    teardown_manager_manual();
    database_subsystem_shutdown();
    app_config = saved_app_config;
    global_queue_manager = saved_manager;
}

void test_health_check_shutdown_flag(void) {
    database_subsystem->shutdown_requested = true;
    TEST_ASSERT_FALSE(database_health_check());
    database_subsystem->shutdown_requested = false;
    database_subsystem->initialized = false;
    TEST_ASSERT_FALSE(database_health_check());
    database_subsystem->initialized = true;
}

void test_health_check_with_healthy_lead(void) {
    install_manager_with_queue(make_stub_queue("db1", true, false, true));
    TEST_ASSERT_TRUE(database_health_check());
}

void test_health_check_with_shutdown_lead(void) {
    install_manager_with_queue(make_stub_queue("db1", true, true, false));
    TEST_ASSERT_FALSE(database_health_check());
}

void test_health_check_skips_non_lead(void) {
    DatabaseQueue* child = make_stub_queue("db1", false, true, false);
    install_manager_with_queue(child);
    TEST_ASSERT_TRUE(database_health_check());
}

void test_subsystem_init_calloc_failure(void) {
    database_subsystem_shutdown();
    mock_system_set_calloc_failure(1);
    TEST_ASSERT_FALSE(database_subsystem_init());
    mock_system_set_calloc_failure(0);
    TEST_ASSERT_TRUE(database_subsystem_init());
}

void test_subsystem_shutdown_destroys_queue_manager(void) {
    global_queue_manager = database_queue_manager_create(2);
    TEST_ASSERT_NOT_NULL(global_queue_manager);
    database_subsystem_shutdown();
    TEST_ASSERT_NULL(global_queue_manager);
    TEST_ASSERT_NULL(database_subsystem);
    TEST_ASSERT_TRUE(database_subsystem_init());
}

void test_reload_config_shutdown_flag(void) {
    database_subsystem->shutdown_requested = true;
    TEST_ASSERT_FALSE(database_reload_config());
    database_subsystem->shutdown_requested = false;
}

void test_reload_config_add_missing_connection(void) {
    AppConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.databases.connection_count = 2;
    cfg.databases.connections[0].enabled = true;
    cfg.databases.connections[0].name = (char*)"newdb";
    cfg.databases.connections[0].type = (char*)"sqlite";
    cfg.databases.connections[1].enabled = false;
    cfg.databases.connections[1].name = (char*)"offdb";
    cfg.databases.connections[1].type = (char*)"sqlite";
    app_config = &cfg;

    bool ok = database_reload_config();
    TEST_ASSERT_FALSE(ok);
}

void test_reload_config_skips_disabled_and_present(void) {
    AppConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.databases.connection_count = 2;
    cfg.databases.connections[0].enabled = true;
    cfg.databases.connections[0].name = (char*)"keepdb";
    cfg.databases.connections[0].type = (char*)"sqlite";
    cfg.databases.connections[1].enabled = true;
    cfg.databases.connections[1].name = NULL;
    cfg.databases.connections[1].type = (char*)"sqlite";
    app_config = &cfg;

    install_manager_with_queue(make_stub_queue("keepdb", true, false, true));
    TEST_ASSERT_TRUE(database_reload_config());
}

void test_reload_config_removes_orphaned_queue(void) {
    AppConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.databases.connection_count = 1;
    cfg.databases.connections[0].enabled = true;
    cfg.databases.connections[0].name = (char*)"keepdb";
    cfg.databases.connections[0].type = (char*)"sqlite";
    app_config = &cfg;

    DatabaseQueue* orphan = make_stub_queue("orphan", true, false, false);
    DatabaseQueue* keep = make_stub_queue("keepdb", true, false, true);
    global_queue_manager = database_queue_manager_create(4);
    TEST_ASSERT_NOT_NULL(global_queue_manager);
    global_queue_manager->databases[0] = orphan;
    global_queue_manager->databases[1] = keep;
    global_queue_manager->database_count = 2;

    /* orphan is destroyed inside reload; keep remains for teardown destroy */
    bool ok = database_reload_config();
    (void)ok;
    TEST_ASSERT_NOT_NULL(database_queue_manager_get_database(global_queue_manager, "keepdb"));
    TEST_ASSERT_NULL(database_queue_manager_get_database(global_queue_manager, "orphan"));
}

void test_process_api_query_invalid_template(void) {
    char buf[64];
    TEST_ASSERT_FALSE(database_process_api_query("db", "   ", NULL, buf, sizeof(buf)));
    TEST_ASSERT_FALSE(database_process_api_query("", "SELECT 1", NULL, buf, sizeof(buf)));
}

void test_process_api_query_empty_database_name(void) {
    char buf[64];
    TEST_ASSERT_FALSE(database_process_api_query("", "SELECT 1", NULL, buf, sizeof(buf)));
}

void test_process_api_query_with_registered_db_no_pending(void) {
    install_manager_with_queue(make_stub_queue("testdb", true, false, true));
    char buf[64] = {0};
    /* pending manager may or may not exist; either false path is fine */
    TEST_ASSERT_FALSE(database_process_api_query("testdb", "SELECT 1", NULL, buf, sizeof(buf)));
    TEST_ASSERT_FALSE(database_process_api_query("missing", "SELECT 1", NULL, buf, sizeof(buf)));
}

void test_validate_query_too_large(void) {
    size_t huge = (1024U * 1024U) + 8U;
    char* big = malloc(huge + 1U);
    TEST_ASSERT_NOT_NULL(big);
    memset(big, 'A', huge);
    big[huge] = '\0';
    TEST_ASSERT_FALSE(database_validate_query(big));
    free(big);
}

void test_escape_parameter_malloc_failure(void) {
    mock_system_set_malloc_failure(1);
    char* escaped = database_escape_parameter("x");
    mock_system_set_malloc_failure(0);
    TEST_ASSERT_NULL(escaped);
}

void test_get_total_queue_count_with_children(void) {
    DatabaseQueue* lead = make_stub_queue("db1", true, false, true);
    lead->child_queue_count = 2;
    lead->max_child_queues = 2;
    lead->child_queues = calloc(2, sizeof(DatabaseQueue*));
    TEST_ASSERT_NOT_NULL(lead->child_queues);
    lead->child_queues[0] = make_child_queue("slow");
    lead->child_queues[1] = make_child_queue("fast");
    install_manager_with_queue(lead);

    TEST_ASSERT_EQUAL(3, database_get_total_queue_count());
}

void test_get_queue_counts_by_type_with_children(void) {
    DatabaseQueue* lead = make_stub_queue("db1", true, false, true);
    lead->child_queue_count = 4;
    lead->max_child_queues = 4;
    lead->child_queues = calloc(4, sizeof(DatabaseQueue*));
    TEST_ASSERT_NOT_NULL(lead->child_queues);
    const char* types[] = {"slow", "medium", "fast", "cache"};
    for (int i = 0; i < 4; i++) {
        lead->child_queues[i] = make_child_queue(types[i]);
    }
    install_manager_with_queue(lead);

    int lead_c = -1, slow = -1, medium = -1, fast = -1, cache = -1;
    database_get_queue_counts_by_type(&lead_c, &slow, &medium, &fast, &cache);
    TEST_ASSERT_EQUAL(1, lead_c);
    TEST_ASSERT_EQUAL(1, slow);
    TEST_ASSERT_EQUAL(1, medium);
    TEST_ASSERT_EQUAL(1, fast);
    TEST_ASSERT_EQUAL(1, cache);
}

void test_get_readiness_null(void) {
    TEST_ASSERT_FALSE(database_get_readiness(NULL));
}

void test_get_readiness_no_manager(void) {
    AppConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.databases.connection_count = 1;
    cfg.databases.connections[0].enabled = true;
    app_config = &cfg;

    DatabaseReadiness r;
    TEST_ASSERT_FALSE(database_get_readiness(&r));
    TEST_ASSERT_EQUAL(1, r.expected);
    TEST_ASSERT_FALSE(r.all_ready);
}

void test_get_readiness_with_leads(void) {
    AppConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.databases.connection_count = 1;
    cfg.databases.connections[0].enabled = true;
    app_config = &cfg;

    install_manager_with_queue(make_stub_queue("db1", true, false, true));

    DatabaseReadiness r;
    TEST_ASSERT_TRUE(database_get_readiness(&r));
    TEST_ASSERT_EQUAL(1, r.expected);
    TEST_ASSERT_EQUAL(1, r.started);
    TEST_ASSERT_EQUAL(1, r.count);
    TEST_ASSERT_EQUAL_STRING("db1", r.entries[0].name);
    TEST_ASSERT_TRUE(r.entries[0].ready);
    TEST_ASSERT_TRUE(r.all_ready);
}

void test_get_readiness_unknown_name(void) {
    AppConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.databases.connection_count = 1;
    cfg.databases.connections[0].enabled = true;
    app_config = &cfg;

    install_manager_with_queue(make_stub_queue(NULL, true, false, false));

    DatabaseReadiness r;
    TEST_ASSERT_FALSE(database_get_readiness(&r));
    TEST_ASSERT_EQUAL(1, r.count);
    TEST_ASSERT_EQUAL_STRING("Unknown", r.entries[0].name);
    TEST_ASSERT_FALSE(r.entries[0].ready);
}

void test_all_leads_ready_wrapper(void) {
    AppConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.databases.connection_count = 1;
    cfg.databases.connections[0].enabled = true;
    app_config = &cfg;

    install_manager_with_queue(make_stub_queue("db1", true, false, true));
    TEST_ASSERT_TRUE(database_all_leads_ready());
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_health_check_shutdown_flag);
    RUN_TEST(test_health_check_with_healthy_lead);
    RUN_TEST(test_health_check_with_shutdown_lead);
    RUN_TEST(test_health_check_skips_non_lead);
    RUN_TEST(test_subsystem_init_calloc_failure);
    RUN_TEST(test_subsystem_shutdown_destroys_queue_manager);
    RUN_TEST(test_reload_config_shutdown_flag);
    RUN_TEST(test_reload_config_add_missing_connection);
    RUN_TEST(test_reload_config_skips_disabled_and_present);
    RUN_TEST(test_reload_config_removes_orphaned_queue);
    RUN_TEST(test_process_api_query_invalid_template);
    RUN_TEST(test_process_api_query_empty_database_name);
    RUN_TEST(test_process_api_query_with_registered_db_no_pending);
    RUN_TEST(test_validate_query_too_large);
    RUN_TEST(test_escape_parameter_malloc_failure);
    RUN_TEST(test_get_total_queue_count_with_children);
    RUN_TEST(test_get_queue_counts_by_type_with_children);
    RUN_TEST(test_get_readiness_null);
    RUN_TEST(test_get_readiness_no_manager);
    RUN_TEST(test_get_readiness_with_leads);
    RUN_TEST(test_get_readiness_unknown_name);
    RUN_TEST(test_all_leads_ready_wrapper);

    return UNITY_END();
}
