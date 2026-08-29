/*
 * Unity Test File: Terminal Shell setup_child_process Tests
 * Tests terminal_shell.c setup_child_process function using fork-based approach
 * Covers test_mode_force_execv_failure, test_mode_no_exit, and test_mode_force_tty_failure
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/terminal/terminal_shell.h>
#include <src/terminal/terminal_session.h>

#include <unistd.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <signal.h>

// Access to test mode variables - defined in terminal_shell.c under UNITY_TEST_MODE
extern bool test_mode_force_execv_failure;
extern bool test_mode_no_exit;
extern bool test_mode_force_tty_failure;

// Test fixtures
static TerminalSession *test_session = NULL;

// Function prototypes
void test_setup_child_process_force_execv_failure_no_exit(void);
void test_setup_child_process_tty_failure_no_exit(void);
void test_setup_child_process_tty_failure_exits(void);
void test_setup_child_process_both_execs_fail_no_exit(void);
void test_setup_child_process_real_ioctl_failure(void);

// Helper function prototypes
TerminalSession* create_test_session(void);
void reset_test_mode_vars(void);

TerminalSession* create_test_session(void) {
    TerminalSession *session = calloc(1, sizeof(TerminalSession));
    if (session) {
        strcpy(session->session_id, "test_scp_session");
        session->active = true;
        session->connected = false;
        session->terminal_rows = 24;
        session->terminal_cols = 80;
    }
    return session;
}

void reset_test_mode_vars(void) {
    test_mode_force_execv_failure = false;
    test_mode_no_exit = false;
    test_mode_force_tty_failure = false;
}

void setUp(void) {
    test_session = create_test_session();
    reset_test_mode_vars();
}

void tearDown(void) {
    if (test_session) {
        free(test_session);
        test_session = NULL;
    }
    reset_test_mode_vars();
}

/*
 * TEST SUITE: setup_child_process - force execv failure with no_exit
 * Both execv calls skipped, child should return from setup_child_process
 */
void test_setup_child_process_force_execv_failure_no_exit(void) {
    int master_fd, slave_fd;
    char slave_name[256];

    if (!create_pty_pair(&master_fd, &slave_fd, slave_name)) {
        TEST_IGNORE();
        return;
    }

    test_mode_force_execv_failure = true;
    test_mode_no_exit = true;

    pid_t pid = fork();
    TEST_ASSERT_TRUE(pid >= 0);

    if (pid == 0) {
        setup_child_process("/bin/sh", slave_fd, master_fd);
        _exit(0);
    } else {
        int status;
        waitpid(pid, &status, 0);
        TEST_ASSERT_TRUE(WIFEXITED(status));
        TEST_ASSERT_EQUAL(0, WEXITSTATUS(status));
        close(master_fd);
        close(slave_fd);
    }
}

/*
 * TEST SUITE: setup_child_process - ioctl failure (test_mode) with no_exit
 * Child should return from the test_mode_force_tty_failure path
 */
void test_setup_child_process_tty_failure_no_exit(void) {
    int master_fd, slave_fd;
    char slave_name[256];

    if (!create_pty_pair(&master_fd, &slave_fd, slave_name)) {
        TEST_IGNORE();
        return;
    }

    test_mode_force_tty_failure = true;
    test_mode_no_exit = true;

    pid_t pid = fork();
    TEST_ASSERT_TRUE(pid >= 0);

    if (pid == 0) {
        setup_child_process("/bin/sh", slave_fd, master_fd);
        _exit(0);
    } else {
        int status;
        waitpid(pid, &status, 0);
        TEST_ASSERT_TRUE(WIFEXITED(status));
        TEST_ASSERT_EQUAL(0, WEXITSTATUS(status));
        close(master_fd);
        close(slave_fd);
    }
}

/*
 * TEST SUITE: setup_child_process - ioctl failure (test_mode) causes exit(1)
 */
void test_setup_child_process_tty_failure_exits(void) {
    int master_fd, slave_fd;
    char slave_name[256];

    if (!create_pty_pair(&master_fd, &slave_fd, slave_name)) {
        TEST_IGNORE();
        return;
    }

    test_mode_force_tty_failure = true;
    test_mode_no_exit = false;

    pid_t pid = fork();
    TEST_ASSERT_TRUE(pid >= 0);

    if (pid == 0) {
        setup_child_process("/bin/sh", slave_fd, master_fd);
        _exit(2);
    } else {
        int status;
        waitpid(pid, &status, 0);
        TEST_ASSERT_TRUE(WIFEXITED(status));
        TEST_ASSERT_EQUAL(1, WEXITSTATUS(status));
        close(master_fd);
        close(slave_fd);
    }
}

/*
 * TEST SUITE: setup_child_process - both execv calls skipped, no_exit returns
 */
void test_setup_child_process_both_execs_fail_no_exit(void) {
    int master_fd, slave_fd;
    char slave_name[256];

    if (!create_pty_pair(&master_fd, &slave_fd, slave_name)) {
        TEST_IGNORE();
        return;
    }

    test_mode_force_execv_failure = true;
    test_mode_no_exit = true;
    test_mode_force_tty_failure = false;

    pid_t pid = fork();
    TEST_ASSERT_TRUE(pid >= 0);

    if (pid == 0) {
        setup_child_process("/bin/false", slave_fd, master_fd);
        _exit(0);
    } else {
        int status;
        waitpid(pid, &status, 0);
        TEST_ASSERT_TRUE(WIFEXITED(status));
        TEST_ASSERT_EQUAL(0, WEXITSTATUS(status));
        close(master_fd);
        close(slave_fd);
    }
}

/*
 * TEST SUITE: setup_child_process - real ioctl failure with invalid fd
 * Passes a closed fd to trigger real EBADF on ioctl
 */
void test_setup_child_process_real_ioctl_failure(void) {
    int master_fd, slave_fd;
    char slave_name[256];

    if (!create_pty_pair(&master_fd, &slave_fd, slave_name)) {
        TEST_IGNORE();
        return;
    }

    // Close slave_fd before fork so child inherits it as closed
    close(slave_fd);

    pid_t pid = fork();
    TEST_ASSERT_TRUE(pid >= 0);

    if (pid == 0) {
        setup_child_process("/bin/sh", slave_fd, master_fd);
        _exit(2);
    } else {
        int status;
        waitpid(pid, &status, 0);
        TEST_ASSERT_TRUE(WIFEXITED(status));
        TEST_ASSERT_EQUAL(1, WEXITSTATUS(status));
        close(master_fd);
    }
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_setup_child_process_force_execv_failure_no_exit);
    RUN_TEST(test_setup_child_process_tty_failure_no_exit);
    RUN_TEST(test_setup_child_process_tty_failure_exits);
    RUN_TEST(test_setup_child_process_both_execs_fail_no_exit);
    RUN_TEST(test_setup_child_process_real_ioctl_failure);

    return UNITY_END();
}
