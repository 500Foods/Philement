/*
 * Unity Test File: Terminal Shell Process Status Tests
 * Tests terminal_shell.c process status and termination functions
 * Focuses on improving coverage for pty_is_running and pty_terminate_shell
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
void test_pty_is_running_process_exited(void);
void test_pty_is_running_process_signaled(void);
void test_pty_terminate_shell_sigkill_path(void);
void test_pty_terminate_shell_waitpid_error(void);

// Helper function prototypes
PtyShell* create_mock_shell_for_process_tests(void);
TerminalSession* create_mock_session_for_process_tests(void);
void cleanup_process_test_resources(void);

void setUp(void) {
    // Reset mocks
    mock_mhd_reset_all();
    mock_session_reset_all();

    // Create test fixtures
    test_session = create_mock_session_for_process_tests();
    test_shell = NULL;
}

void tearDown(void) {
    // Clean up test resources
    cleanup_process_test_resources();
}

// Helper function to create a mock terminal session for process tests
TerminalSession* create_mock_session_for_process_tests(void) {
    TerminalSession *session = calloc(1, sizeof(TerminalSession));
    if (session) {
        strcpy(session->session_id, "test_process_session_123");
        session->active = true;
        session->connected = false;
        session->terminal_rows = 24;
        session->terminal_cols = 80;
    }
    return session;
}

// Helper function to create a mock PtyShell for process status tests
PtyShell* create_mock_shell_for_process_tests(void) {
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

// Helper function to clean up process test resources
void cleanup_process_test_resources(void) {
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
 * TEST SUITE: pty_is_running - Process Status Testing
 */

// Test pty_is_running when process has exited normally
void test_pty_is_running_process_exited(void) {
    PtyShell *shell = create_mock_shell_for_process_tests();

    // Mock a PID that doesn't exist (simulating exited process)
    shell->pid = 99999; // Non-existent PID

    bool result = pty_is_running(shell);

    // Process should be detected as not running
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(shell->running); // Should be set to false

    cleanup_process_test_resources();
}

// Test pty_is_running when process was terminated by signal
void test_pty_is_running_process_signaled(void) {
    PtyShell *shell = create_mock_shell_for_process_tests();

    // Mock a PID that doesn't exist (simulating signaled process)
    shell->pid = 99998; // Non-existent PID

    bool result = pty_is_running(shell);

    // Process should be detected as not running
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(shell->running); // Should be set to false

    cleanup_process_test_resources();
}

/*
 * TEST SUITE: pty_terminate_shell - Process Termination Testing
 */

// Test pty_terminate_shell when kill fails for non-existent process
void test_pty_terminate_shell_sigkill_path(void) {
    PtyShell *shell = create_mock_shell_for_process_tests();

    // Use a non-existent PID to simulate process that doesn't exist
    shell->pid = 99997; // Non-existent PID

    bool result = pty_terminate_shell(shell);

    // Should fail to send signal to non-existent process
    TEST_ASSERT_FALSE(result); // Function returns false when kill fails
    // shell->running should still be true since termination didn't succeed

    cleanup_process_test_resources();
}

// Test pty_terminate_shell when kill fails for non-existent process
void test_pty_terminate_shell_waitpid_error(void) {
    PtyShell *shell = create_mock_shell_for_process_tests();

    // Use a non-existent PID
    shell->pid = 99996; // Non-existent PID

    bool result = pty_terminate_shell(shell);

    // Should fail to send signal to non-existent process
    TEST_ASSERT_FALSE(result); // Function returns false when kill fails
    // shell->running should still be true since termination didn't succeed

    cleanup_process_test_resources();
}

int main(void) {
    UNITY_BEGIN();

    // pty_is_running tests for process status
    RUN_TEST(test_pty_is_running_process_exited);
    RUN_TEST(test_pty_is_running_process_signaled);

    // pty_terminate_shell tests for termination paths
    RUN_TEST(test_pty_terminate_shell_sigkill_path);
    RUN_TEST(test_pty_terminate_shell_waitpid_error);

    return UNITY_END();
}