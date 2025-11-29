/*
 * Unity Test File: Web Server Core Run Web Server Tests
 * This file contains unit tests for run_web_server functionality
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks BEFORE including source headers
#define USE_MOCK_LIBMICROHTTPD
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_logging.h>
#include <unity/mocks/mock_system.h>

// Include source headers (functions will be mocked)
#include <src/webserver/web_server_core.h>

// Note: Global state variables are defined in the source file

// Forward declarations for mock functions
struct ifaddrs;

// Mock external functions
static int mock_getifaddrs_should_fail = 0;

__attribute__((weak))
int getifaddrs(struct ifaddrs **ifap);
__attribute__((weak))
void freeifaddrs(struct ifaddrs *ifa);

__attribute__((weak))
int getifaddrs(struct ifaddrs **ifap) {
    // Mock implementation - can be controlled to fail
    if (mock_getifaddrs_should_fail) {
        return -1;
    }
    *ifap = NULL;
    return 0;
}

static void mock_getifaddrs_set_failure(int should_fail) {
    mock_getifaddrs_should_fail = should_fail;
}

__attribute__((weak))
void freeifaddrs(struct ifaddrs *ifa) {
    // Mock implementation - do nothing
    (void)ifa;
}

__attribute__((weak))
int getnameinfo(const struct sockaddr *addr, socklen_t addrlen,
                char *host, socklen_t hostlen,
                char *serv, socklen_t servlen, int flags) {
    // Mock implementation - return success
    (void)addr;
    (void)addrlen;
    (void)serv;
    (void)servlen;
    (void)flags;

    if (host && hostlen > 0) {
        strncpy(host, "127.0.0.1", hostlen - 1);
        host[hostlen - 1] = '\0';
    }
    return 0;
}

// Note: MHD_start_daemon is mocked by the mock_libmicrohttpd library

__attribute__((weak))
const union MHD_DaemonInfo* MHD_get_daemon_info(struct MHD_Daemon *daemon, enum MHD_DaemonInfoType info_type, ...) {
    // Mock implementation - return NULL to simulate failure
    (void)daemon;
    (void)info_type;
    return NULL;
}

__attribute__((weak))
void MHD_stop_daemon(struct MHD_Daemon *daemon) {
    // Mock implementation - do nothing
    (void)daemon;
}

void setUp(void) {
    // Reset all mocks to default state
    mock_mhd_reset_all();
    mock_logging_reset_all();
    mock_system_reset_all();
    mock_getifaddrs_set_failure(0);

    // Reset global state
    webserver_daemon = NULL;
    server_web_config = NULL;
    web_server_shutdown = 0;
    server_stopping = 0;
    server_starting = 0;
}

void tearDown(void) {
    // Clean up after each test
    mock_mhd_reset_all();
    mock_logging_reset_all();
    mock_system_reset_all();
    mock_getifaddrs_set_failure(0);

    // Reset global state
    webserver_daemon = NULL;
    server_web_config = NULL;
    web_server_shutdown = 0;
    server_stopping = 0;
    server_starting = 0;
}

// Function prototypes for test functions
void test_run_web_server_shutdown_flag_set(void);
void test_run_web_server_not_starting_phase(void);
void test_run_web_server_daemon_already_exists(void);
void test_run_web_server_shutdown_during_startup(void);
void test_run_web_server_getifaddrs_failure(void);
void test_run_web_server_mhd_start_daemon_failure(void);
void test_run_web_server_mhd_get_daemon_info_failure(void);
void test_run_web_server_successful_startup(void);

// Test cases for run_web_server function

void test_run_web_server_shutdown_flag_set(void) {
    // Test: shutdown flag set should return NULL immediately

    // Setup
    server_stopping = 1;
    web_server_shutdown = 0;

    // Execute
    void* result = run_web_server(NULL);

    // Verify
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_EQUAL(1, mock_logging_get_call_count());
    TEST_ASSERT_EQUAL_STRING("WebServer", mock_logging_get_last_subsystem());
}

void test_run_web_server_not_starting_phase(void) {
    // Test: not in starting phase should return NULL

    // Setup
    server_stopping = 0;
    web_server_shutdown = 0;
    server_starting = 0; // Not in starting phase

    // Execute
    void* result = run_web_server(NULL);

    // Verify
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_EQUAL(1, mock_logging_get_call_count());
    TEST_ASSERT_EQUAL_STRING("WebServer", mock_logging_get_last_subsystem());
}

void test_run_web_server_daemon_already_exists(void) {
    // Test: daemon already exists should return NULL

    // Setup
    server_stopping = 0;
    web_server_shutdown = 0;
    server_starting = 1;
    webserver_daemon = (struct MHD_Daemon *)0x12345678; // Mock daemon exists

    // Execute
    void* result = run_web_server(NULL);

    // Verify
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_EQUAL(1, mock_logging_get_call_count());
    TEST_ASSERT_EQUAL_STRING("WebServer", mock_logging_get_last_subsystem());
}

void test_run_web_server_shutdown_during_startup(void) {
    // Test: shutdown initiated during startup checks

    // Setup
    server_stopping = 0;
    web_server_shutdown = 0;
    server_starting = 1;
    webserver_daemon = NULL;

    // Set shutdown flag after initial checks
    server_stopping = 1;

    // Execute
    void* result = run_web_server(NULL);

    // Verify
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_TRUE(mock_logging_get_call_count() >= 1);
    TEST_ASSERT_EQUAL_STRING("WebServer", mock_logging_get_last_subsystem());
}

void test_run_web_server_getifaddrs_failure(void) {
    // Test: getifaddrs failure

    // Setup
    server_stopping = 0;
    web_server_shutdown = 0;
    server_starting = 1;
    webserver_daemon = NULL;

    // Mock getifaddrs to fail
    mock_getifaddrs_set_failure(1);

    // Execute
    void* result = run_web_server(NULL);

    // Verify
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_TRUE(mock_logging_get_call_count() >= 1);
    TEST_ASSERT_EQUAL_STRING("WebServer", mock_logging_get_last_subsystem());
}

void test_run_web_server_mhd_start_daemon_failure(void) {
    // Test: MHD_start_daemon failure

    // Setup
    server_stopping = 0;
    web_server_shutdown = 0;
    server_starting = 1;
    webserver_daemon = NULL;

    // Create mock config (required for run_web_server)
    WebServerConfig *config = calloc(1, sizeof(WebServerConfig));
    TEST_ASSERT_NOT_NULL(config);
    config->port = 8080;
    config->enable_ipv6 = false;
    config->thread_pool_size = 4;
    config->max_connections = 100;
    config->max_connections_per_ip = 10;
    config->connection_timeout = 30;
    config->web_root = strdup("/tmp");
    config->upload_path = strdup("/tmp/upload");
    config->upload_dir = strdup("/tmp/upload");
    server_web_config = config;

    // Mock MHD_start_daemon to return NULL (failure)
    mock_mhd_set_start_daemon_should_fail(true);

    // Execute
    void* result = run_web_server(NULL);

    // Verify
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_TRUE(mock_logging_get_call_count() >= 1);
    TEST_ASSERT_EQUAL_STRING("WebServer", mock_logging_get_last_subsystem());

    // Cleanup
    free(config->web_root);
    free(config->upload_path);
    free(config->upload_dir);
    free(config);
    server_web_config = NULL;
}

void test_run_web_server_mhd_get_daemon_info_failure(void) {
    // Test: MHD_get_daemon_info failure

    // Setup
    server_stopping = 0;
    web_server_shutdown = 0;
    server_starting = 1;
    webserver_daemon = NULL;

    // Create mock config (required for run_web_server)
    WebServerConfig *config = calloc(1, sizeof(WebServerConfig));
    TEST_ASSERT_NOT_NULL(config);
    config->port = 8080;
    config->enable_ipv6 = false;
    config->thread_pool_size = 4;
    config->max_connections = 100;
    config->max_connections_per_ip = 10;
    config->connection_timeout = 30;
    config->web_root = strdup("/tmp");
    config->upload_path = strdup("/tmp/upload");
    config->upload_dir = strdup("/tmp/upload");
    server_web_config = config;

    // Mock MHD_start_daemon to succeed, but MHD_get_daemon_info to fail
    mock_mhd_set_start_daemon_should_fail(false);  // Let start_daemon succeed
    // MHD_get_daemon_info returns NULL by default (failure)

    // Execute
    void* result = run_web_server(NULL);

    // Verify
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_TRUE(mock_logging_get_call_count() >= 1);
    TEST_ASSERT_EQUAL_STRING("WebServer", mock_logging_get_last_subsystem());

    // Cleanup
    free(config->web_root);
    free(config->upload_path);
    free(config->upload_dir);
    free(config);
    server_web_config = NULL;
}

void test_run_web_server_successful_startup(void) {
    // Test: Successful startup scenario

    // Setup
    server_stopping = 0;
    web_server_shutdown = 0;
    server_starting = 1;
    webserver_daemon = NULL;

    // Create mock config
    WebServerConfig *config = calloc(1, sizeof(WebServerConfig));
    TEST_ASSERT_NOT_NULL(config);
    config->port = 8080;
    config->enable_ipv6 = false;
    config->thread_pool_size = 4;
    config->max_connections = 100;
    config->max_connections_per_ip = 10;
    config->connection_timeout = 30;
    config->web_root = strdup("/tmp");
    config->upload_path = strdup("/tmp/upload");
    config->upload_dir = strdup("/tmp/upload");
    server_web_config = config;

    // Mock successful startup
    mock_mhd_set_start_daemon_should_fail(false);

    // Create mock daemon info for successful MHD_get_daemon_info
    static union MHD_DaemonInfo daemon_info;
    daemon_info.port = 8080;  // Mock bound port
    mock_mhd_set_daemon_info_result(&daemon_info);

    // Execute
    void* result = run_web_server(NULL);

    // Verify
    TEST_ASSERT_NULL(result); // Thread function returns NULL
    TEST_ASSERT_TRUE(mock_logging_get_call_count() >= 1);
    TEST_ASSERT_EQUAL_STRING("WebServer", mock_logging_get_last_subsystem());

    // Cleanup
    free(config->web_root);
    free(config->upload_path);
    free(config->upload_dir);
    free(config);
    server_web_config = NULL;
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_run_web_server_shutdown_flag_set);
    RUN_TEST(test_run_web_server_not_starting_phase);
    RUN_TEST(test_run_web_server_daemon_already_exists);
    RUN_TEST(test_run_web_server_shutdown_during_startup);
    RUN_TEST(test_run_web_server_getifaddrs_failure);
    RUN_TEST(test_run_web_server_mhd_start_daemon_failure);
    RUN_TEST(test_run_web_server_mhd_get_daemon_info_failure);
    RUN_TEST(test_run_web_server_successful_startup);

    return UNITY_END();
}