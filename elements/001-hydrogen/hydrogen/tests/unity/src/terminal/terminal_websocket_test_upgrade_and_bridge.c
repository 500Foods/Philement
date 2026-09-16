/*
 * Unity Test File: Terminal WebSocket Upgrade and I/O Bridge Coverage
 *
 * Targets lines in src/terminal/terminal_websocket.c that were previously
 * uncovered by both the Unity and blackbox suites:
 *   - start_terminal_websocket_bridge pthread_create failure (572-574)
 *   - process_terminal_websocket_message raw-input activity update (266)
 *   - read_pty_with_select real select() error path (446, 452-453)
 *   - process_pty_read_result send-failure path (479-480)
 *   - terminal_io_bridge_thread invalid connection guard (508-509)
 *   - handle_terminal_websocket_close bridge-thread join path (615-620)
 *
 * The terminal source objects are compiled with USE_MOCK_LIBMICROHTTPD and
 * USE_MOCK_PTHREAD (see cmake/CMakeLists-unity.cmake). This means:
 *   - pthread_create inside the source is mocked, so
 *     start_terminal_websocket_bridge never spawns a real thread; the bridge
 *     thread handle is a sentinel and must NOT be joined by the test.
 *
 * The libmicrohttpd mock's session helpers are weak symbols, so the REAL
 * terminal_session implementation (linked from the hydrogen_unity archive with
 * strong symbols) wins. We therefore drive the real session subsystem directly:
 * init_session_manager()/cleanup_session_manager() with the cleanup thread
 * disabled. create_terminal_session() genuinely spawns a PTY shell, which is
 * the same behaviour exercised by the terminal_session unit tests.
 *
 * pthread_create in THIS test file is the real implementation (the test file
 * is compiled with the websocket mock set, not the pthread mock), so we can
 * create a genuine joinable thread to exercise the close/join path.
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the terminal websocket module
#include <src/config/config_terminal.h>
#include <src/terminal/terminal_websocket.h>
#include <src/terminal/terminal_session.h>
#include <src/terminal/terminal_shell.h>

// USE_MOCK_* defines are provided by CMake - don't redefine them.
// Include the libmicrohttpd mock header to access the mock_session_* and
// mock_mhd_* control helpers. Do NOT include mock_terminal_websocket.h so the
// real terminal_websocket.c functions are exercised.
#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_libwebsockets.h>

// The terminal_websocket.c object is compiled with USE_MOCK_SYSTEM (it matches
// the "websocket" source rule), so select()/malloc()/read() etc. inside the
// source are the mock_system versions. These control helpers/globals live in
// mock_system.c and are referenced here directly because this test file is NOT
// compiled with USE_MOCK_SYSTEM (so its own select/malloc stay real).
extern int mock_select_result;          // Value returned by the mocked select()
void mock_system_set_malloc_failure(int should_fail);
void mock_system_set_calloc_failure(int should_fail);
void mock_system_reset_all(void);


bool send_terminal_websocket_output(TerminalWSConnection *connection,
                                   const char *data,
                                   size_t data_size);
bool should_continue_io_bridge(TerminalWSConnection *connection);
int read_pty_with_select(TerminalWSConnection *connection, char *buffer, size_t buffer_size);
bool process_pty_read_result(TerminalWSConnection *connection, const char *buffer, int bytes_read);
bool start_terminal_websocket_bridge(TerminalWSConnection *connection);
void handle_terminal_websocket_close(TerminalWSConnection *connection);
void *terminal_io_bridge_thread(void *arg);

// Test control from the pthread mock (source uses the mocked pthread_create)
void mock_pthread_set_create_failure(int should_fail);
void mock_pthread_reset_all(void);

// Real session manager controls (strong symbols win over the weak MHD mocks)
extern SessionManager *global_session_manager;

// Fork/waitpid control helpers (defined in mock_system.c, always available)
void mock_system_set_fork_result(pid_t result);
void mock_system_set_waitpid_result(pid_t result);

// Test function prototypes
void test_start_bridge_pthread_create_failure(void);
void test_process_message_raw_input_activity_update(void);
void test_read_pty_with_select_error(void);
void test_read_pty_with_select_interrupted(void);
void test_process_pty_read_result_send_failure(void);
void test_io_bridge_thread_null_session(void);
void test_close_joins_real_bridge_thread(void);
void test_send_output_lws_write_failure_drops_frame(void);
void test_send_output_lws_write_partial(void);
void test_send_output_buffer_malloc_failure(void);
void test_io_bridge_thread_buffer_malloc_failure(void);

// Test fixtures
static TerminalConfig test_config;
static TerminalSession test_session;
static PtyShell test_pty_shell;

void setUp(void) {
    memset(&test_config, 0, sizeof(TerminalConfig));
    test_config.enabled = true;
    test_config.web_path = strdup("/terminal");
    test_config.shell_command = strdup("/bin/bash");
    test_config.max_sessions = 10;
    test_config.idle_timeout_seconds = 300;
    test_config.buffer_size = 1024;

    memset(&test_pty_shell, 0, sizeof(PtyShell));
    test_pty_shell.master_fd = -1; // Invalid FD by default (safe for select error tests)
    test_pty_shell.running = true;

    memset(&test_session, 0, sizeof(TerminalSession));
    strncpy(test_session.session_id, "test_session_upg", sizeof(test_session.session_id) - 1);
    test_session.created_time = time(NULL);
    test_session.last_activity = time(NULL);
    test_session.pty_shell = &test_pty_shell;
    test_session.terminal_rows = 24;
    test_session.terminal_cols = 80;
    pthread_mutex_init(&test_session.session_mutex, NULL);
    test_session.active = true;
    test_session.connected = true;

    mock_mhd_reset_all();
    mock_session_reset_all();
    mock_pthread_reset_all();
    mock_system_reset_all();
    mock_lws_reset_all();

    // Prevent real fork/exec from spawning a live /bin/bash that contaminates
    // the terminal. Mirror the pattern used by terminal_shell_test_pty_operations.c
    // setUp(): fork returns a sentinel PID (no child process), waitpid returns
    // 0 (process still running) so pty_is_running() stays true and the raw-input
    // path in process_terminal_websocket_message proceeds normally.
    mock_system_set_fork_result(99999);
    mock_system_set_waitpid_result(0);

    // Use the real session subsystem with the cleanup thread disabled so the
    // upgrade path exercises genuine session creation.
    terminal_session_disable_cleanup_thread();
    cleanup_session_manager();
}

void tearDown(void) {
    if (test_config.web_path) {
        free(test_config.web_path);
        test_config.web_path = NULL;
    }
    if (test_config.shell_command) {
        free(test_config.shell_command);
        test_config.shell_command = NULL;
    }
    pthread_mutex_destroy(&test_session.session_mutex);

    // Tear down any sessions created during the test.
    cleanup_session_manager();

    mock_session_reset_all();
    mock_pthread_reset_all();
    mock_system_reset_all();
    mock_lws_reset_all();
}

/*
 * TEST: start_terminal_websocket_bridge pthread_create failure directly
 * Target: lines 572-574 (create failure -> bridge_thread = 0, return false)
 */
void test_start_bridge_pthread_create_failure(void) {
    TerminalWSConnection conn;
    memset(&conn, 0, sizeof(conn));
    conn.session = &test_session;
    conn.active = true;
    strncpy(conn.session_id, test_session.session_id, sizeof(conn.session_id) - 1);
    conn.bridge_thread = (pthread_t)0xdeadbeef; // Non-zero to verify it is reset

    mock_pthread_set_create_failure(1);

    bool result = start_terminal_websocket_bridge(&conn);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_UINT64((uint64_t)0, (uint64_t)conn.bridge_thread);
}

/*
 * TEST: process_terminal_websocket_message raw (non-JSON) input
 * Target: line 266 (update_session_activity after successful raw send)
 *
 * A non-JSON payload takes the raw-input branch. Using a real session with a
 * live PTY, send_data_to_session writes to the PTY master (>= 0), so the
 * function updates activity and returns true.
 */
void test_process_message_raw_input_activity_update(void) {
    TEST_ASSERT_TRUE(init_session_manager(10, 300));
    TerminalSession *session = create_terminal_session(test_config.shell_command, 24, 80);
    TEST_ASSERT_NOT_NULL(session);

    TerminalWSConnection conn;
    memset(&conn, 0, sizeof(conn));
    conn.session = session;
    conn.active = true;
    strncpy(conn.session_id, session->session_id, sizeof(conn.session_id) - 1);

    const char *raw = "not json data\n";
    bool result = process_terminal_websocket_message(&conn, raw, strlen(raw));

    TEST_ASSERT_TRUE(result);
    // session is owned by the manager; reaped by cleanup_session_manager().
}

/*
 * TEST: read_pty_with_select select() error (errno != EINTR)
 * Target: lines 446, 452-453 (select() returns -1, non-EINTR -> log + return -1)
 *
 * The source's select() is the mock_system version. Forcing it to return -1
 * with errno set to a non-EINTR value drives the error branch.
 */
void test_read_pty_with_select_error(void) {
    test_pty_shell.master_fd = 5; // Arbitrary valid-looking fd (mock ignores it)

    TerminalWSConnection conn;
    memset(&conn, 0, sizeof(conn));
    conn.session = &test_session;
    conn.active = true;
    strncpy(conn.session_id, test_session.session_id, sizeof(conn.session_id) - 1);

    mock_select_result = -1;
    errno = EBADF; // Non-EINTR so the else branch (452-453) is taken

    char buffer[128];
    int result = read_pty_with_select(&conn, buffer, sizeof(buffer));

    TEST_ASSERT_EQUAL_INT(-1, result);
}

/*
 * TEST: read_pty_with_select select() interrupted by signal
 * Target: lines 446-449 (select() returns -1 with errno == EINTR -> return -2)
 */
void test_read_pty_with_select_interrupted(void) {
    test_pty_shell.master_fd = 5;

    TerminalWSConnection conn;
    memset(&conn, 0, sizeof(conn));
    conn.session = &test_session;
    conn.active = true;
    strncpy(conn.session_id, test_session.session_id, sizeof(conn.session_id) - 1);

    mock_select_result = -1;
    errno = EINTR; // Interrupted -> return -2

    char buffer[128];
    int result = read_pty_with_select(&conn, buffer, sizeof(buffer));

    TEST_ASSERT_EQUAL_INT(-2, result);
}

/*
 * TEST: process_pty_read_result when send fails
 * Target: lines 479-480 (send_terminal_websocket_output returns false -> false)
 *
 * With bytes_read > 0 and the session marked disconnected, the send helper
 * returns false, so process_pty_read_result logs the error and returns false.
 */
void test_process_pty_read_result_send_failure(void) {
    TerminalWSConnection conn;
    memset(&conn, 0, sizeof(conn));
    conn.session = &test_session;
    conn.active = true;
    strncpy(conn.session_id, test_session.session_id, sizeof(conn.session_id) - 1);

    // Disconnected session forces send_terminal_websocket_output to fail early.
    test_session.connected = false;

    const char buffer[] = "some pty output";
    bool result = process_pty_read_result(&conn, buffer, (int)(sizeof(buffer) - 1));

    TEST_ASSERT_FALSE(result);
}

/*
 * TEST: terminal_io_bridge_thread with a NULL session
 * Target: lines 508-509 (invalid connection/session guard -> return NULL)
 *
 * Passing a connection whose session is NULL hits the early guard and returns
 * immediately without entering the I/O loop.
 */
void test_io_bridge_thread_null_session(void) {
    TerminalWSConnection conn;
    memset(&conn, 0, sizeof(conn));
    conn.session = NULL; // Triggers the guard
    conn.active = true;
    strncpy(conn.session_id, "no_session", sizeof(conn.session_id) - 1);

    void *ret = terminal_io_bridge_thread(&conn);
    TEST_ASSERT_NULL(ret);
}

// Trivial thread body used to exercise the join path in the close handler.
static void *quick_exit_thread(void *arg) {
    (void)arg;
    return NULL;
}

/*
 * TEST: handle_terminal_websocket_close joins the bridge thread
 * Target: lines 615-620 (bridge_thread != 0 -> clock_gettime + pthread_timedjoin_np)
 *
 * pthread_create in this test file is the real implementation, so we create a
 * genuine joinable thread that exits immediately and assign it as the
 * connection's bridge_thread handle. handle_terminal_websocket_close then joins
 * it (well within the 5s timeout) and frees the heap-allocated connection.
 */
void test_close_joins_real_bridge_thread(void) {
    TerminalWSConnection *conn = calloc(1, sizeof(TerminalWSConnection));
    TEST_ASSERT_NOT_NULL(conn);

    conn->session = NULL; // remove_terminal_session not needed; avoid touching session
    conn->active = true;
    strncpy(conn->session_id, "join_test", sizeof(conn->session_id) - 1);
    conn->incoming_buffer = NULL;

    // Real, joinable thread that returns right away.
    int rc = pthread_create(&conn->bridge_thread, NULL, quick_exit_thread, NULL);
    TEST_ASSERT_EQUAL_INT(0, rc);

    // Joins the thread (615-620) and frees conn. No assertion beyond no-crash.
    handle_terminal_websocket_close(conn);

    TEST_PASS();
}

/*
 * TEST: send_terminal_websocket_output when lws_write reports failure
 * Target: lines 329-337 (result < 0 -> drop frame, backpressure, return true)
 *
 * With a non-NULL wsi (session->websocket_connection) and mock_lws_write
 * returning -1, the source treats the frame as droppable and returns true.
 */
void test_send_output_lws_write_failure_drops_frame(void) {
    test_session.websocket_connection = (void *)0x1; // Non-NULL -> wsi is used
    test_session.connected = true;

    TerminalWSConnection conn;
    memset(&conn, 0, sizeof(conn));
    conn.session = &test_session;
    conn.active = true;
    strncpy(conn.session_id, test_session.session_id, sizeof(conn.session_id) - 1);

    mock_lws_set_write_result(-1); // Send failure

    const char *data = "pty output data";
    bool result = send_terminal_websocket_output(&conn, data, strlen(data));

    TEST_ASSERT_TRUE(result); // Frame dropped, session preserved
}

/*
 * TEST: send_terminal_websocket_output partial write
 * Target: lines 338-344 (0 <= result < response_len -> alert, return true)
 */
void test_send_output_lws_write_partial(void) {
    test_session.websocket_connection = (void *)0x1;
    test_session.connected = true;

    TerminalWSConnection conn;
    memset(&conn, 0, sizeof(conn));
    conn.session = &test_session;
    conn.active = true;
    strncpy(conn.session_id, test_session.session_id, sizeof(conn.session_id) - 1);

    // 1 byte written is far less than the JSON response length -> partial path.
    mock_lws_set_write_result(1);

    const char *data = "some reasonably sized terminal output payload";
    bool result = send_terminal_websocket_output(&conn, data, strlen(data));

    TEST_ASSERT_TRUE(result); // Partial write noted, session preserved
}

/*
 * TEST: send_terminal_websocket_output buffer allocation failure
 * Target: line 349 (malloc of the send buffer fails -> log error)
 *
 * The source's malloc is the mock_system version; forcing it to fail makes the
 * buffer allocation return NULL and take the error-logging branch.
 */
void test_send_output_buffer_malloc_failure(void) {
    test_session.websocket_connection = (void *)0x1;
    test_session.connected = true;

    TerminalWSConnection conn;
    memset(&conn, 0, sizeof(conn));
    conn.session = &test_session;
    conn.active = true;
    strncpy(conn.session_id, test_session.session_id, sizeof(conn.session_id) - 1);

    mock_system_set_malloc_failure(1); // buf malloc returns NULL

    const char *data = "output";
    bool result = send_terminal_websocket_output(&conn, data, strlen(data));

    mock_system_set_malloc_failure(0);

    // Source returns true overall (logs the allocation failure but does not
    // treat it as a session-fatal error).
    TEST_ASSERT_TRUE(result);
}

/*
 * TEST: terminal_io_bridge_thread I/O buffer allocation failure
 * Target: lines 521-522 (malloc of the read buffer fails -> log, return NULL)
 *
 * A valid connection/session passes the initial guard, then the source's
 * malloc for the read buffer (mock_malloc) is forced to fail.
 */
void test_io_bridge_thread_buffer_malloc_failure(void) {
    TerminalWSConnection conn;
    memset(&conn, 0, sizeof(conn));
    conn.session = &test_session;
    conn.active = true;
    strncpy(conn.session_id, test_session.session_id, sizeof(conn.session_id) - 1);

    mock_system_set_malloc_failure(1); // I/O buffer malloc returns NULL

    void *ret = terminal_io_bridge_thread(&conn);

    mock_system_set_malloc_failure(0);

    TEST_ASSERT_NULL(ret);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_start_bridge_pthread_create_failure);
    RUN_TEST(test_process_message_raw_input_activity_update);
    RUN_TEST(test_read_pty_with_select_error);
    RUN_TEST(test_read_pty_with_select_interrupted);
    RUN_TEST(test_process_pty_read_result_send_failure);
    RUN_TEST(test_io_bridge_thread_null_session);
    RUN_TEST(test_close_joins_real_bridge_thread);
    RUN_TEST(test_send_output_lws_write_failure_drops_frame);
    RUN_TEST(test_send_output_lws_write_partial);
    RUN_TEST(test_send_output_buffer_malloc_failure);
    RUN_TEST(test_io_bridge_thread_buffer_malloc_failure);

    return UNITY_END();
}
