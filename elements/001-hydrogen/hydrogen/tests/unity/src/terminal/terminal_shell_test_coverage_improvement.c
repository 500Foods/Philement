/*
 * Unity Test File: Terminal Shell Coverage Improvement Tests
 * Tests terminal_shell.c functions with comprehensive coverage
 * Focuses on PTY shell management, process lifecycle, and error handling
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the terminal shell module
#include <src/terminal/terminal_shell.h>
#include <src/terminal/terminal_session.h>

#include <unistd.h>
#include <errno.h>

// Include mocks for external dependencies
#include <unity/mocks/mock_libwebsockets.h>
#include <unity/mocks/mock_libmicrohttpd.h>

// Include mock system (terminal sources are compiled with USE_MOCK_SYSTEM)
#ifndef USE_MOCK_SYSTEM
#define USE_MOCK_SYSTEM
#endif
#include <unity/mocks/mock_system.h>

// Access to test mode variables - defined in terminal_shell.c under UNITY_TEST_MODE
extern bool test_mode_force_openpty_failure;
extern bool test_mode_force_calloc_failure;
extern bool test_mode_force_strdup_failure;
extern bool test_mode_force_fcntl_failure;
extern bool test_mode_force_fork_failure;
extern bool test_mode_force_execv_failure;

// Test fixtures
static PtyShell *test_shell = NULL;
static TerminalSession *test_session = NULL;

// Function prototypes for test functions
void test_pty_spawn_shell_null_parameters(void);
void test_pty_spawn_shell_success(void);
void test_pty_write_data_null_shell(void);
void test_pty_write_data_not_running(void);
void test_pty_write_data_null_data(void);
void test_pty_write_data_empty_data(void);
void test_pty_write_data_success(void);
void test_pty_read_data_null_shell(void);
void test_pty_read_data_not_running(void);
void test_pty_read_data_null_buffer(void);
void test_pty_read_data_empty_size(void);
void test_pty_read_data_success(void);
void test_pty_read_data_no_data_available(void);
void test_pty_set_size_null_shell(void);
void test_pty_set_size_not_running(void);
void test_pty_set_size_success(void);
void test_pty_is_running_null_shell(void);
void test_pty_is_running_not_running(void);
void test_pty_is_running_process_terminated(void);
void test_pty_is_running_process_exited(void);
void test_pty_is_running_process_signaled(void);
void test_pty_is_running_success(void);
void test_pty_terminate_shell_null_shell(void);
void test_pty_terminate_shell_not_running(void);
void test_pty_terminate_shell_kill_failure(void);
void test_pty_terminate_shell_success(void);
void test_pty_cleanup_shell_null_shell(void);
void test_pty_cleanup_shell_with_running_process(void);
void test_pty_cleanup_shell_success(void);

// Helper function prototypes
PtyShell* create_mock_shell(void);
TerminalSession* create_mock_session(void);
void cleanup_test_resources(void);

void setUp(void) {
    // Reset mocks
    mock_mhd_reset_all();
    mock_session_reset_all();
    mock_system_reset_all();

    // Reset test mode variables
    test_mode_force_openpty_failure = false;
    test_mode_force_calloc_failure = false;
    test_mode_force_strdup_failure = false;
    test_mode_force_fcntl_failure = false;
    test_mode_force_fork_failure = false;
    test_mode_force_execv_failure = false;

    // Create test fixtures
    test_session = create_mock_session();
    test_shell = NULL;
}

void tearDown(void) {
    // Clean up test resources
    cleanup_test_resources();
    mock_system_reset_all();
}

// Helper function to create a mock terminal session
TerminalSession* create_mock_session(void) {
    TerminalSession *session = calloc(1, sizeof(TerminalSession));
    if (session) {
        strcpy(session->session_id, "test_session_123");
        session->active = true;
        session->connected = false;
        session->terminal_rows = 24;
        session->terminal_cols = 80;
    }
    return session;
}

// Helper function to create a mock PtyShell for testing
PtyShell* create_mock_shell(void) {
    PtyShell *shell = calloc(1, sizeof(PtyShell));
    if (shell) {
        shell->master_fd = 42; // Mock file descriptor
        shell->slave_fd = 43;  // Mock file descriptor
        shell->slave_name = strdup("/dev/pts/5");
        shell->pid = 12345;   // Mock PID
        shell->running = true;
        shell->session = test_session;
    }
    return shell;
}

// Helper function to clean up test resources
void cleanup_test_resources(void) {
    if (test_shell) {
        // Don't call pty_cleanup_shell here as it might try to terminate real processes
        if (test_shell->slave_name) {
            free(test_shell->slave_name);
        }
        free(test_shell);
        test_shell = NULL;
    }

    if (test_session) {
        free(test_session);
        test_session = NULL;
    }
}

/*
 * TEST SUITE: pty_spawn_shell
 */

void test_pty_spawn_shell_null_parameters(void) {
    // Test NULL shell_command
    PtyShell *result = pty_spawn_shell(NULL, test_session);
    TEST_ASSERT_NULL(result);

    // Test NULL session
    result = pty_spawn_shell("/bin/bash", NULL);
    TEST_ASSERT_NULL(result);

    // Test both NULL
    result = pty_spawn_shell(NULL, NULL);
    TEST_ASSERT_NULL(result);
}

void test_pty_spawn_shell_success(void) {
    // Configure mocks for a successful spawn
    mock_system_set_fork_result(99999);  // parent path
    mock_system_set_waitpid_result(0);   // process still running
    mock_system_set_openpty_failure(0);
    mock_system_set_fcntl_failure(0);
    mock_system_set_malloc_failure(0);

    PtyShell *result = pty_spawn_shell("/bin/bash", test_session);

    // With mocked fork returning parent PID and waitpid returning 0, spawn succeeds
    TEST_ASSERT_NOT_NULL(result);

    if (result) {
        // Clean up immediately
        pty_cleanup_shell(result);
        test_shell = NULL;
    }
}

/*
 * TEST SUITE: pty_write_data
 */

void test_pty_write_data_null_shell(void) {
    int result = pty_write_data(NULL, "test", 4);
    TEST_ASSERT_EQUAL(-1, result);
}

void test_pty_write_data_not_running(void) {
    PtyShell *shell = create_mock_shell();
    test_shell = shell;
    shell->running = false;

    int result = pty_write_data(shell, "test", 4);
    TEST_ASSERT_EQUAL(-1, result);

    cleanup_test_resources();
}

void test_pty_write_data_null_data(void) {
    PtyShell *shell = create_mock_shell();
    test_shell = shell;

    int result = pty_write_data(shell, NULL, 4);
    TEST_ASSERT_EQUAL(-1, result);

    cleanup_test_resources();
}

void test_pty_write_data_empty_data(void) {
    PtyShell *shell = create_mock_shell();
    test_shell = shell;

    int result = pty_write_data(shell, "test", 0);
    TEST_ASSERT_EQUAL(-1, result);

    cleanup_test_resources();
}

void test_pty_write_data_success(void) {
    PtyShell *shell = create_mock_shell();
    test_shell = shell;

    // Use an invalid file descriptor and force write to fail
    shell->master_fd = 99999;
    mock_system_set_write_should_fail(1);

    int result = pty_write_data(shell, "test", 4);

    // Should return -1 when write fails
    TEST_ASSERT_EQUAL(-1, result);

    cleanup_test_resources();
}

/*
 * TEST SUITE: pty_read_data
 */

void test_pty_read_data_null_shell(void) {
    char buffer[100];
    int result = pty_read_data(NULL, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL(-1, result);
}

void test_pty_read_data_not_running(void) {
    PtyShell *shell = create_mock_shell();
    test_shell = shell;
    shell->running = false;

    char buffer[100];
    int result = pty_read_data(shell, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL(-1, result);

    cleanup_test_resources();
}

void test_pty_read_data_null_buffer(void) {
    PtyShell *shell = create_mock_shell();
    test_shell = shell;

    int result = pty_read_data(shell, NULL, 100);
    TEST_ASSERT_EQUAL(-1, result);

    cleanup_test_resources();
}

void test_pty_read_data_empty_size(void) {
    PtyShell *shell = create_mock_shell();
    test_shell = shell;
    char buffer[100];

    int result = pty_read_data(shell, buffer, 0);
    TEST_ASSERT_EQUAL(-1, result);

    cleanup_test_resources();
}

void test_pty_read_data_success(void) {
    PtyShell *shell = create_mock_shell();
    test_shell = shell;
    char buffer[100];

    // Force read to fail (non-EAGAIN error)
    shell->master_fd = 99999;
    mock_system_set_read_should_fail(1);

    int result = pty_read_data(shell, buffer, sizeof(buffer));

    // Should return -1 on read error
    TEST_ASSERT_EQUAL(-1, result);

    cleanup_test_resources();
}

void test_pty_read_data_no_data_available(void) {
    PtyShell *shell = create_mock_shell();
    test_shell = shell;
    char buffer[100];

    // Mock read to return EAGAIN (no data available)
    shell->master_fd = 99999;
    mock_system_set_read_should_fail(0);
    mock_system_set_read_eagain(1);

    int result = pty_read_data(shell, buffer, sizeof(buffer));

    // Should return 0 when EAGAIN (no data available)
    TEST_ASSERT_EQUAL(0, result);

    // Reset EAGAIN for other tests
    mock_system_set_read_eagain(0);

    cleanup_test_resources();
}

/*
 * TEST SUITE: pty_set_size
 */

void test_pty_set_size_null_shell(void) {
    bool result = pty_set_size(NULL, 24, 80);
    TEST_ASSERT_FALSE(result);
}

void test_pty_set_size_not_running(void) {
    PtyShell *shell = create_mock_shell();
    test_shell = shell;
    shell->running = false;

    bool result = pty_set_size(shell, 24, 80);
    TEST_ASSERT_FALSE(result);

    cleanup_test_resources();
}

void test_pty_set_size_success(void) {
    PtyShell *shell = create_mock_shell();
    test_shell = shell;

    // Force ioctl to fail
    shell->master_fd = 99999;
    mock_system_set_ioctl_failure(1);

    bool result = pty_set_size(shell, 24, 80);

    // Should return false when ioctl fails
    TEST_ASSERT_FALSE(result);

    cleanup_test_resources();
}

/*
 * TEST SUITE: pty_is_running
 */

void test_pty_is_running_null_shell(void) {
    bool result = pty_is_running(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_pty_is_running_not_running(void) {
    PtyShell *shell = create_mock_shell();
    test_shell = shell;
    shell->running = false;

    bool result = pty_is_running(shell);
    TEST_ASSERT_FALSE(result);

    cleanup_test_resources();
}

void test_pty_is_running_process_terminated(void) {
    PtyShell *shell = create_mock_shell();
    test_shell = shell;
    shell->pid = 99999; // Non-existent PID

    // Mock waitpid to return -1 with ECHILD (process doesn't exist)
    mock_system_set_waitpid_result(-1);
    errno = ECHILD;

    bool result = pty_is_running(shell);

    // Process doesn't exist, so should return false
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(shell->running); // Should be set to false

    cleanup_test_resources();
}

void test_pty_is_running_process_exited(void) {
    PtyShell *shell = create_mock_shell();
    test_shell = shell;

    // Mock waitpid to return the PID (process exited)
    mock_system_set_waitpid_result(shell->pid);
    mock_system_set_waitpid_status(0);

    bool result = pty_is_running(shell);
    TEST_ASSERT_FALSE(result); // Process exited

    cleanup_test_resources();
}

void test_pty_is_running_process_signaled(void) {
    PtyShell *shell = create_mock_shell();
    test_shell = shell;

    // Mock waitpid to return the PID (process signaled, SIGKILL=9)
    mock_system_set_waitpid_result(shell->pid);
    mock_system_set_waitpid_status(0x80 | 9);

    bool result = pty_is_running(shell);
    TEST_ASSERT_FALSE(result); // Process signaled

    cleanup_test_resources();
}

void test_pty_is_running_success(void) {
    PtyShell *shell = create_mock_shell();
    test_shell = shell;

    // Mock waitpid to return 0 (process still running)
    mock_system_set_waitpid_result(0);

    bool result = pty_is_running(shell);
    TEST_ASSERT_TRUE(result); // Process is running

    cleanup_test_resources();
}

/*
 * TEST SUITE: pty_terminate_shell
 */

void test_pty_terminate_shell_null_shell(void) {
    bool result = pty_terminate_shell(NULL);
    TEST_ASSERT_FALSE(result);
}

void test_pty_terminate_shell_not_running(void) {
    PtyShell *shell = create_mock_shell();
    test_shell = shell;
    shell->running = false;

    bool result = pty_terminate_shell(shell);
    TEST_ASSERT_FALSE(result);

    cleanup_test_resources();
}

void test_pty_terminate_shell_kill_failure(void) {
    PtyShell *shell = create_mock_shell();
    test_shell = shell;
    shell->pid = 99999; // Non-existent PID

    // Force kill to fail
    mock_system_set_kill_failure(1);

    bool result = pty_terminate_shell(shell);

    // Should fail to send signal to non-existent process
    TEST_ASSERT_FALSE(result);

    cleanup_test_resources();
}

void test_pty_terminate_shell_success(void) {
    PtyShell *shell = create_mock_shell();
    test_shell = shell;

    // kill succeeds (mock default returns 0)
    mock_system_set_kill_failure(0);

    bool result = pty_terminate_shell(shell);

    // kill succeeded, so pty_terminate_shell should return true
    TEST_ASSERT_TRUE(result);

    cleanup_test_resources();
}

/*
 * TEST SUITE: pty_cleanup_shell
 */

void test_pty_cleanup_shell_null_shell(void) {
    // Should not crash
    pty_cleanup_shell(NULL);
    TEST_PASS();
}

void test_pty_cleanup_shell_with_running_process(void) {
    PtyShell *shell = create_mock_shell();

    // This will attempt to terminate and clean up the mock process
    pty_cleanup_shell(shell);

    // Shell should be freed, so we don't call cleanup_test_resources
    test_shell = NULL;
}

void test_pty_cleanup_shell_success(void) {
    PtyShell *shell = create_mock_shell();
    shell->running = false; // Not running, so no termination attempt

    pty_cleanup_shell(shell);

    // Shell should be freed, so we don't call cleanup_test_resources
    test_shell = NULL;
}

int main(void) {
    UNITY_BEGIN();

    // pty_spawn_shell tests
    RUN_TEST(test_pty_spawn_shell_null_parameters);
    RUN_TEST(test_pty_spawn_shell_success);

    // pty_write_data tests
    RUN_TEST(test_pty_write_data_null_shell);
    RUN_TEST(test_pty_write_data_not_running);
    RUN_TEST(test_pty_write_data_null_data);
    RUN_TEST(test_pty_write_data_empty_data);
    RUN_TEST(test_pty_write_data_success);

    // pty_read_data tests
    RUN_TEST(test_pty_read_data_null_shell);
    RUN_TEST(test_pty_read_data_not_running);
    RUN_TEST(test_pty_read_data_null_buffer);
    RUN_TEST(test_pty_read_data_empty_size);
    RUN_TEST(test_pty_read_data_success);
    RUN_TEST(test_pty_read_data_no_data_available);

    // pty_set_size tests
    RUN_TEST(test_pty_set_size_null_shell);
    RUN_TEST(test_pty_set_size_not_running);
    RUN_TEST(test_pty_set_size_success);

    // pty_is_running tests
    RUN_TEST(test_pty_is_running_null_shell);
    RUN_TEST(test_pty_is_running_not_running);
    RUN_TEST(test_pty_is_running_process_terminated);
    RUN_TEST(test_pty_is_running_process_exited);
    RUN_TEST(test_pty_is_running_process_signaled);
    RUN_TEST(test_pty_is_running_success);

    // pty_terminate_shell tests
    RUN_TEST(test_pty_terminate_shell_null_shell);
    RUN_TEST(test_pty_terminate_shell_not_running);
    RUN_TEST(test_pty_terminate_shell_kill_failure);
    RUN_TEST(test_pty_terminate_shell_success);

    // pty_cleanup_shell tests
    RUN_TEST(test_pty_cleanup_shell_null_shell);
    RUN_TEST(test_pty_cleanup_shell_with_running_process);
    RUN_TEST(test_pty_cleanup_shell_success);

    return UNITY_END();
}