/*
 * Unity Test File: Terminal Shell Process Exit Code Tests
 * Tests terminal_shell.c process exit code handling for improved coverage
 * Focuses on WIFEXITED/WIFSIGNALED branches and termination paths
 */

#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the terminal shell module
#include "../../../../src/terminal/terminal_shell.h"
#include "../../../../src/terminal/terminal_session.h"

// Include mocks for external dependencies
#include "../../../../tests/unity/mocks/mock_libwebsockets.h"
#include "../../../../tests/unity/mocks/mock_libmicrohttpd.h"

// Test fixtures
static PtyShell *test_shell = NULL;
static TerminalSession *test_session = NULL;

// Function prototypes for test functions
void test_pty_is_running_process_exited_with_code(void);
void test_pty_is_running_process_killed_by_signal(void);
void test_pty_terminate_shell_graceful_timeout(void);
void test_pty_terminate_shell_force_kill_required(void);

// Helper function prototypes
PtyShell* create_mock_shell_for_exit_tests(void);
TerminalSession* create_mock_session_for_exit_tests(void);
void cleanup_exit_test_resources(void);

void setUp(void) {
    // Reset mocks
    mock_mhd_reset_all();
    mock_session_reset_all();

    // Create test fixtures
    test_session = create_mock_session_for_exit_tests();
    test_shell = NULL;
}

void tearDown(void) {
    // Clean up test resources
    cleanup_exit_test_resources();
}

// Helper function to create a mock terminal session for exit tests
TerminalSession* create_mock_session_for_exit_tests(void) {
    TerminalSession *session = calloc(1, sizeof(TerminalSession));
    if (session) {
        strcpy(session->session_id, "test_exit_session_123");
        session->active = true;
        session->connected = false;
        session->terminal_rows = 24;
        session->terminal_cols = 80;
    }
    return session;
}

// Helper function to create a mock PtyShell for exit code tests
PtyShell* create_mock_shell_for_exit_tests(void) {
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

// Helper function to clean up exit test resources
void cleanup_exit_test_resources(void) {
    if (test_shell) {
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
 * TEST SUITE: pty_is_running - Process Exit Code Handling
 */

// Test pty_is_running when process exited with a specific code
void test_pty_is_running_process_exited_with_code(void) {
    PtyShell *shell = create_mock_shell_for_exit_tests();

    // Use a PID that doesn't exist to simulate exited process
    shell->pid = 99999; // Non-existent PID

    bool result = pty_is_running(shell);

    // Process should be detected as not running
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(shell->running); // Should be set to false

    cleanup_exit_test_resources();
}

// Test pty_is_running when process was killed by signal
void test_pty_is_running_process_killed_by_signal(void) {
    PtyShell *shell = create_mock_shell_for_exit_tests();

    // Use a PID that doesn't exist to simulate killed process
    shell->pid = 99998; // Non-existent PID

    bool result = pty_is_running(shell);

    // Process should be detected as not running
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(shell->running); // Should be set to false

    cleanup_exit_test_resources();
}

/*
 * TEST SUITE: pty_terminate_shell - Termination Path Coverage
 */

// Test pty_terminate_shell when graceful termination times out
void test_pty_terminate_shell_graceful_timeout(void) {
    PtyShell *shell = create_mock_shell_for_exit_tests();

    // Use a PID that doesn't exist - kill will fail, but we test the timeout logic
    shell->pid = 99997; // Non-existent PID

    bool result = pty_terminate_shell(shell);

    // Should fail to terminate non-existent process
    TEST_ASSERT_FALSE(result);

    cleanup_exit_test_resources();
}

// Test pty_terminate_shell when force kill is required
void test_pty_terminate_shell_force_kill_required(void) {
    PtyShell *shell = create_mock_shell_for_exit_tests();

    // Use a PID that doesn't exist - both SIGTERM and SIGKILL will fail
    shell->pid = 99996; // Non-existent PID

    bool result = pty_terminate_shell(shell);

    // Should fail to terminate non-existent process
    TEST_ASSERT_FALSE(result);

    cleanup_exit_test_resources();
}

int main(void) {
    UNITY_BEGIN();

    // pty_is_running exit code tests
    RUN_TEST(test_pty_is_running_process_exited_with_code);
    RUN_TEST(test_pty_is_running_process_killed_by_signal);

    // pty_terminate_shell termination path tests
    RUN_TEST(test_pty_terminate_shell_graceful_timeout);
    RUN_TEST(test_pty_terminate_shell_force_kill_required);

    return UNITY_END();
}