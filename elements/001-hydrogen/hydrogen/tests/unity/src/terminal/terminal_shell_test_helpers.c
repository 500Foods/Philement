/*
 * Unity Test File: Terminal Shell Helper Functions Tests
 * Tests terminal_shell.c helper functions for improved coverage
 * Focuses on create_pty_pair and configure_master_fd functions
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the terminal shell module
#include <src/terminal/terminal_shell.h>
#include <src/terminal/terminal_session.h>

#include <unistd.h>
#include <fcntl.h>

// Test fixtures
static int test_master_fd = -1;
static int test_slave_fd = -1;

// Function prototypes for test functions
void test_create_pty_pair_null_master_fd(void);
void test_create_pty_pair_null_slave_fd(void);
void test_create_pty_pair_null_slave_name(void);
void test_create_pty_pair_success(void);
void test_configure_master_fd_invalid_fd(void);
void test_configure_master_fd_success(void);
void test_pty_cleanup_shell_null_shell(void);
void test_pty_cleanup_shell_with_running_shell(void);
void test_pty_cleanup_shell_not_running(void);
void test_cleanup_pty_resources_all_null(void);
void test_cleanup_pty_resources_partial(void);
void test_cleanup_pty_resources_with_shell(void);

// Helper function prototypes
TerminalSession* create_test_session(void);

// Helper function to create a test session
TerminalSession* create_test_session(void) {
    TerminalSession *session = calloc(1, sizeof(TerminalSession));
    if (session) {
        strcpy(session->session_id, "test_spawn_session");
        session->active = true;
        session->connected = false;
        session->terminal_rows = 24;
        session->terminal_cols = 80;
    }
    return session;
}

void setUp(void) {
    test_master_fd = -1;
    test_slave_fd = -1;
}

void tearDown(void) {
    // Clean up file descriptors
    if (test_master_fd >= 0) {
        close(test_master_fd);
        test_master_fd = -1;
    }
    if (test_slave_fd >= 0) {
        close(test_slave_fd);
        test_slave_fd = -1;
    }
}

/*
 * TEST SUITE: create_pty_pair Parameter Validation
 */

// Test create_pty_pair with NULL master_fd parameter
void test_create_pty_pair_null_master_fd(void) {
    char slave_name[256];
    
    bool result = create_pty_pair(NULL, &test_slave_fd, slave_name);
    
    TEST_ASSERT_FALSE(result);
}

// Test create_pty_pair with NULL slave_fd parameter
void test_create_pty_pair_null_slave_fd(void) {
    char slave_name[256];
    
    bool result = create_pty_pair(&test_master_fd, NULL, slave_name);
    
    TEST_ASSERT_FALSE(result);
}

// Test create_pty_pair with NULL slave_name parameter
void test_create_pty_pair_null_slave_name(void) {
    bool result = create_pty_pair(&test_master_fd, &test_slave_fd, NULL);
    
    TEST_ASSERT_FALSE(result);
}

// Test create_pty_pair with valid parameters
void test_create_pty_pair_success(void) {
    char slave_name[256];
    
    bool result = create_pty_pair(&test_master_fd, &test_slave_fd, slave_name);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(test_master_fd >= 0);
    TEST_ASSERT_TRUE(test_slave_fd >= 0);
    TEST_ASSERT_TRUE(strlen(slave_name) > 0);
    
    // Verify we can get file status (file descriptors are valid)
    int flags = fcntl(test_master_fd, F_GETFL);
    TEST_ASSERT_TRUE(flags >= 0);
}

/*
 * TEST SUITE: configure_master_fd
 */

// Test configure_master_fd with invalid file descriptor
void test_configure_master_fd_invalid_fd(void) {
    // Use an invalid file descriptor (-1)
    bool result = configure_master_fd(-1);
    
    TEST_ASSERT_FALSE(result);
}

// Test configure_master_fd with valid file descriptor
void test_configure_master_fd_success(void) {
    char slave_name[256];
    
    // First create a valid PTY pair
    bool pty_result = create_pty_pair(&test_master_fd, &test_slave_fd, slave_name);
    TEST_ASSERT_TRUE(pty_result);
    
    // Now configure the master FD
    bool result = configure_master_fd(test_master_fd);
    
    TEST_ASSERT_TRUE(result);
    
    // Verify the FD is now non-blocking
    int flags = fcntl(test_master_fd, F_GETFL);
    TEST_ASSERT_TRUE(flags >= 0);
    TEST_ASSERT_TRUE((flags & O_NONBLOCK) != 0);
}

/*
 * TEST SUITE: pty_cleanup_shell
 */

// Test pty_cleanup_shell with NULL shell
void test_pty_cleanup_shell_null_shell(void) {
    // Should not crash
    pty_cleanup_shell(NULL);
    TEST_PASS();
}

// Test pty_cleanup_shell with running shell
void test_pty_cleanup_shell_with_running_shell(void) {
    // Create a real shell to test cleanup
    TerminalSession *session = create_test_session();
    PtyShell *shell = pty_spawn_shell("/bin/sh", session);
    
    if (shell) {
        // Give shell time to start
        usleep(50000); // 50ms
        
        // Cleanup should terminate and free everything
        pty_cleanup_shell(shell);
        
        // Shell is now freed, don't cleanup again
    }
    
    if (session) {
        free(session);
    }
    
    TEST_PASS();
}

// Test pty_cleanup_shell with already stopped shell
void test_pty_cleanup_shell_not_running(void) {
    // Create a real shell first
    TerminalSession *session = create_test_session();
    PtyShell *shell = pty_spawn_shell("/bin/sh", session);
    
    if (shell) {
        usleep(50000); // 50ms
        
        // Terminate first
        pty_terminate_shell(shell);
        
        // Now cleanup the already-terminated shell
        pty_cleanup_shell(shell);
    }
    
    if (session) {
        free(session);
    }
    
    TEST_PASS();
}

/*
 * TEST SUITE: cleanup_pty_resources
 */

// Test cleanup_pty_resources with all null/invalid parameters
void test_cleanup_pty_resources_all_null(void) {
    // Should not crash with all invalid/null values
    cleanup_pty_resources(-1, -1, NULL, NULL);
    TEST_PASS();
}

// Test cleanup_pty_resources with partial cleanup
void test_cleanup_pty_resources_partial(void) {
    // Create a real PTY pair
    int master_fd, slave_fd;
    char slave_name[256];
    bool result = create_pty_pair(&master_fd, &slave_fd, slave_name);
    TEST_ASSERT_TRUE(result);
    
    // Duplicate the slave name
    char *name_copy = strdup(slave_name);
    TEST_ASSERT_NOT_NULL(name_copy);
    
    // Clean up just the file descriptors and name, no shell structure
    cleanup_pty_resources(master_fd, slave_fd, name_copy, NULL);
    
    // If we got here without crashing, the cleanup worked
    TEST_PASS();
}

// Test cleanup_pty_resources with a shell structure
void test_cleanup_pty_resources_with_shell(void) {
    // Create a mock shell structure
    PtyShell *shell = calloc(1, sizeof(PtyShell));
    TEST_ASSERT_NOT_NULL(shell);
    
    // Create a real PTY pair
    int master_fd, slave_fd;
    char slave_name[256];
    bool result = create_pty_pair(&master_fd, &slave_fd, slave_name);
    TEST_ASSERT_TRUE(result);
    
    // Duplicate the slave name
    char *name_copy = strdup(slave_name);
    TEST_ASSERT_NOT_NULL(name_copy);
    
    // Clean up everything including shell
    cleanup_pty_resources(master_fd, slave_fd, name_copy, shell);
    
    // If we got here without crashing, the cleanup worked
    TEST_PASS();
}

int main(void) {
    UNITY_BEGIN();

    // create_pty_pair parameter validation tests
    RUN_TEST(test_create_pty_pair_null_master_fd);
    RUN_TEST(test_create_pty_pair_null_slave_fd);
    RUN_TEST(test_create_pty_pair_null_slave_name);
    RUN_TEST(test_create_pty_pair_success);

    // configure_master_fd tests
    RUN_TEST(test_configure_master_fd_invalid_fd);
    RUN_TEST(test_configure_master_fd_success);

    // cleanup_pty_resources tests
    RUN_TEST(test_cleanup_pty_resources_all_null);
    RUN_TEST(test_cleanup_pty_resources_partial);
    RUN_TEST(test_cleanup_pty_resources_with_shell);

    // pty_cleanup_shell tests
    RUN_TEST(test_pty_cleanup_shell_null_shell);
    RUN_TEST(test_pty_cleanup_shell_with_running_shell);
    RUN_TEST(test_pty_cleanup_shell_not_running);

    return UNITY_END();
}