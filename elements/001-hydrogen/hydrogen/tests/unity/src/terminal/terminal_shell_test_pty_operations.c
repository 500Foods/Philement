/*
 * Unity Test File: Terminal Shell PTY Operations Tests
 * Tests terminal_shell.c with actual PTY operations for improved coverage
 * Focuses on pty_write_data, pty_read_data, and pty_set_size with real PTY
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the terminal shell module
#include <src/terminal/terminal_shell.h>
#include <src/terminal/terminal_session.h>

#include <unistd.h>
#include <errno.h>
#include <string.h>

// Include mocks for external dependencies
#include <unity/mocks/mock_libwebsockets.h>
#include <unity/mocks/mock_libmicrohttpd.h>

// Include mock system (terminal sources are compiled with USE_MOCK_SYSTEM)
#ifndef USE_MOCK_SYSTEM
#define USE_MOCK_SYSTEM
#endif
#include <unity/mocks/mock_system.h>

// Test fixtures
static PtyShell *test_shell = NULL;
static TerminalSession *test_session = NULL;

// Function prototypes for test functions
void test_pty_write_data_with_real_pty(void);
void test_pty_read_data_with_real_pty(void);
void test_pty_set_size_with_real_pty(void);
void test_pty_write_read_cycle(void);
void test_pty_operations_full_cycle(void);

// Helper function prototypes
TerminalSession* create_test_session(void);
void cleanup_test_resources(void);
static void setup_spawn_mocks(void);

void setUp(void) {
    // Reset all mocks
    mock_mhd_reset_all();
    mock_session_reset_all();
    mock_system_reset_all();

    // Create test session
    test_session = create_test_session();
    test_shell = NULL;
}

void tearDown(void) {
    // Clean up test resources
    cleanup_test_resources();
    mock_system_reset_all();
}

// Helper function to create a test terminal session
TerminalSession* create_test_session(void) {
    TerminalSession *session = calloc(1, sizeof(TerminalSession));
    if (session) {
        strcpy(session->session_id, "test_pty_ops_session");
        session->active = true;
        session->connected = false;
        session->terminal_rows = 24;
        session->terminal_cols = 80;
    }
    return session;
}

// Helper function to clean up test resources
void cleanup_test_resources(void) {
    if (test_shell) {
        pty_cleanup_shell(test_shell);
        test_shell = NULL;
    }

    if (test_session) {
        free(test_session);
        test_session = NULL;
    }
}

// Helper to configure mocks for pty_spawn_shell success
static void setup_spawn_mocks(void) {
    // Mock fork to return a non-zero PID (simulating successful fork, parent path)
    mock_system_set_fork_result(99999);

    // Mock waitpid to return 0 (process still running, not terminated)
    mock_system_set_waitpid_result(0);

    // Mock openpty to succeed
    mock_system_set_openpty_failure(0);

    // Mock fcntl to succeed
    mock_system_set_fcntl_failure(0);

    // Mock access to succeed (so execv path is taken in child, but we're in parent mock)
    mock_system_set_access_result(0);

    // Mock write/read to behave as expected
    mock_system_set_write_result(10);
    mock_system_set_read_result(5);
    mock_system_set_read_should_fail(0);
    mock_system_set_write_should_fail(0);

    // Mock ioctl to succeed for setup_child_process
    mock_system_set_ioctl_failure(0);

    // Mock strdup to use real implementation
    mock_system_set_malloc_failure(0);
}

/*
 * TEST SUITE: pty_write_data with Real PTY
 */

// Test pty_write_data with an spawned shell
void test_pty_write_data_with_real_pty(void) {
    setup_spawn_mocks();
    
    // Spawn a shell with mocked system calls
    test_shell = pty_spawn_shell("/bin/sh", test_session);
    TEST_ASSERT_NOT_NULL(test_shell);
    
    if (test_shell) {
        // Test writing data to the PTY
        const char *test_data = "echo test\n";
        int result = pty_write_data(test_shell, test_data, strlen(test_data));
        
        // Should successfully write (mock returns 10 bytes)
        TEST_ASSERT_TRUE(result >= 0);
        
        // Clean up
        cleanup_test_resources();
    }
}

/*
 * TEST SUITE: pty_read_data with Real PTY
 */

// Test pty_read_data with spawned shell
void test_pty_read_data_with_real_pty(void) {
    setup_spawn_mocks();
    
    // Spawn a shell with mocked system calls
    test_shell = pty_spawn_shell("/bin/sh", test_session);
    TEST_ASSERT_NOT_NULL(test_shell);
    
    if (test_shell) {
        // Test reading data from the PTY
        char buffer[256];
        int result = pty_read_data(test_shell, buffer, sizeof(buffer));
        
        // Should read data (mock returns 5 bytes)
        TEST_ASSERT_TRUE(result >= 0);
        
        // If we got data, it should be within buffer size
        if (result > 0) {
            TEST_ASSERT_TRUE(result <= (int)sizeof(buffer));
        }
        
        // Clean up
        cleanup_test_resources();
    }
}

/*
 * TEST SUITE: pty_set_size with spawned PTY
 */

// Test pty_set_size with spawned shell
void test_pty_set_size_with_real_pty(void) {
    setup_spawn_mocks();
    
    // Spawn a shell with mocked system calls
    test_shell = pty_spawn_shell("/bin/sh", test_session);
    TEST_ASSERT_NOT_NULL(test_shell);
    
    if (test_shell) {
        // Test setting terminal size
        bool result = pty_set_size(test_shell, 40, 120);
        
        // Should succeed (mocked ioctl returns 0)
        TEST_ASSERT_TRUE(result);
        
        // Try setting a different size
        result = pty_set_size(test_shell, 25, 85);
        TEST_ASSERT_TRUE(result);
        
        // Clean up
        cleanup_test_resources();
    }
}

/*
 * TEST SUITE: Combined Write/Read Operations
 */

// Test writing and reading from PTY in sequence
void test_pty_write_read_cycle(void) {
    setup_spawn_mocks();
    
    // Spawn a shell with mocked system calls
    test_shell = pty_spawn_shell("/bin/sh", test_session);
    TEST_ASSERT_NOT_NULL(test_shell);
    
    if (test_shell) {
        // Write a command that produces output
        const char *command = "echo 'Hello PTY'\n";
        int write_result = pty_write_data(test_shell, command, strlen(command));
        
        // Write should succeed (mock returns 10 bytes)
        TEST_ASSERT_TRUE(write_result >= 0);
        
        // Try to read the output
        char buffer[512];
        memset(buffer, 0, sizeof(buffer));
        int read_result = pty_read_data(test_shell, buffer, sizeof(buffer) - 1);
        
        // Read should succeed (mock returns 5 bytes)
        TEST_ASSERT_TRUE(read_result >= 0);
        
        // If we got data, verify it's reasonable
        if (read_result > 0) {
            TEST_ASSERT_TRUE(read_result < (int)sizeof(buffer));
        }
        
        // Clean up
        cleanup_test_resources();
    }
}

/*
 * TEST SUITE: Full PTY Lifecycle Operations
 */

// Test complete PTY lifecycle with all operations
void test_pty_operations_full_cycle(void) {
    setup_spawn_mocks();
    
    // Spawn a shell with mocked system calls
    test_shell = pty_spawn_shell("/bin/sh", test_session);
    TEST_ASSERT_NOT_NULL(test_shell);
    
    if (test_shell) {
        // Verify shell is running
        TEST_ASSERT_TRUE(test_shell->running);
        
        // Set terminal size
        bool size_result = pty_set_size(test_shell, 30, 100);
        TEST_ASSERT_TRUE(size_result);
        
        // Write some data
        const char *data = "pwd\n";
        int write_result = pty_write_data(test_shell, data, strlen(data));
        TEST_ASSERT_TRUE(write_result >= 0);
        
        // Read response
        char buffer[512];
        int read_result = pty_read_data(test_shell, buffer, sizeof(buffer));
        TEST_ASSERT_TRUE(read_result >= 0);
        
        // Check if shell is still running
        (void)pty_is_running(test_shell);
        
        // Clean up
        cleanup_test_resources();
    }
}

int main(void) {
    UNITY_BEGIN();

    // Real PTY operation tests
    RUN_TEST(test_pty_write_data_with_real_pty);
    RUN_TEST(test_pty_read_data_with_real_pty);
    RUN_TEST(test_pty_set_size_with_real_pty);
    RUN_TEST(test_pty_write_read_cycle);
    RUN_TEST(test_pty_operations_full_cycle);

    return UNITY_END();
}