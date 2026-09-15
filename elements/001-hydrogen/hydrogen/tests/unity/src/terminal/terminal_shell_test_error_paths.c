/*
 * Unity Test File: Terminal Shell Error Path Tests
 * Tests terminal_shell.c error handling paths for improved coverage
 * Uses mock_system for deterministic process/kill waitpid behavior
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the terminal shell module
#include <src/terminal/terminal_shell.h>
#include <src/terminal/terminal_session.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>

// Include mocks for external dependencies
#include <unity/mocks/mock_libwebsockets.h>
#include <unity/mocks/mock_libmicrohttpd.h>

// Include mock system (terminal sources are compiled with USE_MOCK_SYSTEM)
#ifndef USE_MOCK_SYSTEM
#define USE_MOCK_SYSTEM
#endif
#include <unity/mocks/mock_system.h>

// Test fixtures
static TerminalSession *test_session = NULL;

// Function prototypes for test functions
void test_pty_write_data_write_error(void);
void test_pty_read_data_read_error(void);
void test_pty_set_size_ioctl_failure(void);
void test_pty_is_running_echild_error(void);
void test_pty_terminate_shell_kill_failure(void);
void test_pty_spawn_shell_premature_termination(void);
void test_pty_is_running_process_terminated_pid_returned(void);

// Helper function prototypes
TerminalSession* create_test_session(void);

void setUp(void) {
    // Reset all mocks
    mock_mhd_reset_all();
    mock_session_reset_all();
    mock_system_reset_all();

    // Create test session  
    test_session = create_test_session();
}

void tearDown(void) {
    // Clean up test session
    if (test_session) {
        free(test_session);
        test_session = NULL;
    }

    // Reset mocks
    mock_system_reset_all();
}

// Helper function to create a test terminal session
TerminalSession* create_test_session(void) {
    TerminalSession *session = calloc(1, sizeof(TerminalSession));
    if (session) {
        strcpy(session->session_id, "test_error_session");
        session->active = true;
        session->connected = false;
        session->terminal_rows = 24;
        session->terminal_cols = 80;
    }
    return session;
}

/*
 * TEST SUITE: pty_spawn_shell - Premature Termination (lines 190-195)
 */

// Test pty_spawn_shell when shell terminates immediately after spawning
void test_pty_spawn_shell_premature_termination(void) {
    // Mock pty_spawn_shell internals: openpty succeeds (returns 0),
    // fork succeeds returning PID, waitpid returns PID (process exited)
    // Since terminal_shell.c uses USE_MOCK_SYSTEM, we set up the mocks

    // Mock fork to return a non-zero PID (parent process)
    mock_system_set_fork_result(99999);

    // Mock waitpid to return the PID (simulating premature termination)
    mock_system_set_waitpid_result(99999);
    mock_system_set_waitpid_status(0);  // exited with code 0

    // Mock openpty to succeed
    mock_system_set_openpty_failure(0);

    // Mock fcntl to succeed
    mock_system_set_fcntl_failure(0);

    // Mock strdup to succeed (shell->slave_name allocation)
    mock_system_set_malloc_failure(0);
    mock_malloc_call_count = 0;  // reset counter

    PtyShell *result = pty_spawn_shell("/bin/false", test_session);

    // Should return NULL because shell terminated prematurely
    TEST_ASSERT_NULL(result);
}

/*
 * TEST SUITE: pty_is_running - Process Terminated with PID (lines 302-305)
 */

// Test pty_is_running when waitpid returns the shell PID (process terminated)
void test_pty_is_running_process_terminated_pid_returned(void) {
    // Create a shell with mock PID
    PtyShell shell;
    shell.master_fd = 42;
    shell.running = true;
    shell.session = test_session;
    shell.slave_fd = -1;
    shell.slave_name = NULL;
    shell.pid = 12345;

    // Mock waitpid to return the PID (process terminated)
    mock_system_set_waitpid_result(shell.pid);
    mock_system_set_waitpid_status(0);

    bool result = pty_is_running(&shell);

    // Should return false and set shell->running to false
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(shell.running);

    // This exercises the waitpid returns PID case
}

/*
 * TEST SUITE: pty_write_data - Write Error Path (line 220-223)
 */

// Test pty_write_data with write error that's not EAGAIN/EWOULDBLOCK
void test_pty_write_data_write_error(void) {
    // Create a shell with an invalid file descriptor to trigger write errors
    PtyShell shell;
    shell.master_fd = -1;  // Invalid FD
    shell.running = true;
    shell.session = test_session;
    shell.slave_fd = -1;
    shell.slave_name = NULL;
    shell.pid = 0;

    // Mock write to fail with EBADF (not EAGAIN)
    mock_system_set_write_should_fail(1);

    const char *data = "test data";
    int result = pty_write_data(&shell, data, strlen(data));

    // Should return -1 on write error
    TEST_ASSERT_EQUAL(-1, result);
}

/*
 * TEST SUITE: pty_read_data - Read Error Path (line 247-248)
 */

// Test pty_read_data with non-EAGAIN error
void test_pty_read_data_read_error(void) {
    // Create a shell with an invalid file descriptor
    PtyShell shell;
    shell.master_fd = -1;  // Invalid FD
    shell.running = true;
    shell.session = test_session;
    shell.slave_fd = -1;
    shell.slave_name = NULL;
    shell.pid = 0;

    // Mock read to fail with EBADF (not EAGAIN)
    mock_system_set_read_should_fail(1);

    char buffer[256];
    int result = pty_read_data(&shell, buffer, sizeof(buffer));

    // Should return -1 on read error
    TEST_ASSERT_EQUAL(-1, result);
}

/*
 * TEST SUITE: pty_set_size - ioctl Failure (line 276-277)
 */

// Test pty_set_size when ioctl fails
void test_pty_set_size_ioctl_failure(void) {
    // Create a shell with an invalid file descriptor
    PtyShell shell;
    shell.master_fd = -1;  // Invalid FD
    shell.running = true;
    shell.session = test_session;
    shell.slave_fd = -1;
    shell.slave_name = NULL;
    shell.pid = 0;

    // Mock ioctl to fail with EBADF
    mock_system_set_ioctl_failure(1);

    bool result = pty_set_size(&shell, 24, 80);

    // Should return false when ioctl fails
    TEST_ASSERT_FALSE(result);
}

/*
 * TEST SUITE: pty_is_running - ECHILD Error Path (line 308-310)
 */

// Test pty_is_running when waitpid returns ECHILD
void test_pty_is_running_echild_error(void) {
    // Create a shell with a non-existent PID
    PtyShell shell;
    shell.master_fd = 42;
    shell.running = true;
    shell.session = test_session;
    shell.slave_fd = -1;
    shell.slave_name = NULL;
    shell.pid = 999999;  // Non-existent PID

    // Mock waitpid to return -1 with ECHILD (process doesn't exist)
    mock_system_set_waitpid_result(-1);
    errno = ECHILD;

    bool result = pty_is_running(&shell);

    // Should return false and set running to false
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(shell.running);
}

/*
 * TEST SUITE: pty_terminate_shell - kill Failure (line 329-331)
 */

// Test pty_terminate_shell when kill fails
void test_pty_terminate_shell_kill_failure(void) {
    // Create a shell with a non-existent PID
    PtyShell shell;
    shell.master_fd = 42;
    shell.running = true;
    shell.session = test_session;
    shell.slave_fd = -1;
    shell.slave_name = NULL;
    shell.pid = 999998;  // Non-existent PID

    // Mock kill to fail (non-existent process)
    mock_system_set_kill_failure(1);

    bool result = pty_terminate_shell(&shell);

    // Should return false when kill fails
    TEST_ASSERT_FALSE(result);
}

int main(void) {
    UNITY_BEGIN();

    // Real system error path tests (use actual system errors)
    RUN_TEST(test_pty_write_data_write_error);
    RUN_TEST(test_pty_read_data_read_error);
    RUN_TEST(test_pty_set_size_ioctl_failure);
    RUN_TEST(test_pty_is_running_echild_error);
    RUN_TEST(test_pty_terminate_shell_kill_failure);
    
    // Premature termination and process status tests
    RUN_TEST(test_pty_spawn_shell_premature_termination);
    RUN_TEST(test_pty_is_running_process_terminated_pid_returned);

    return UNITY_END();

}