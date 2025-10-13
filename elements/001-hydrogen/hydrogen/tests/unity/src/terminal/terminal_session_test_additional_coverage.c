/*
 * Unity Test File: Terminal Session Additional Coverage Tests
 * 
 * This file targets specific uncovered lines in terminal_session.c to reach 75% coverage:
 * - generate_session_id() error paths
 * - list_active_sessions() with sessions
 * - cleanup_expired_sessions() with expired sessions  
 * - remove_terminal_session() linked list manipulation
 */

#include <src/hydrogen.h>
#include <unity.h>
#include <src/terminal/terminal_session.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Test control functions
void terminal_session_disable_cleanup_thread(void);
void terminal_session_enable_cleanup_thread(void);

// Forward declarations for test functions
void test_generate_session_id_null_buffer(void);
void test_generate_session_id_buffer_too_small(void);
void test_generate_session_id_exact_size(void);
void test_generate_session_id_success(void);
void test_generate_session_id_unique(void);
void test_list_active_sessions_with_one_session(void);
void test_list_active_sessions_with_multiple_sessions(void);
void test_cleanup_expired_sessions_with_one_expired(void);
void test_cleanup_expired_sessions_mixed_expired_and_active(void);
void test_remove_terminal_session_from_head(void);
void test_remove_terminal_session_from_middle(void);
void test_remove_terminal_session_from_tail(void);

// Test fixtures
static SessionManager *original_manager = NULL;

void setUp(void) {
    // Save original manager state
    original_manager = global_session_manager;
    
    // Reset global state
    global_session_manager = NULL;
    
    // Disable cleanup thread for testing
    terminal_session_disable_cleanup_thread();
}

void tearDown(void) {
    // Clean up test manager
    if (global_session_manager) {
        cleanup_session_manager();
    }
    
    // Restore original manager
    global_session_manager = original_manager;
}

/*
 * TEST SUITE: generate_session_id()
 */

void test_generate_session_id_null_buffer(void) {
    bool result = generate_session_id(NULL, 100);
    TEST_ASSERT_FALSE(result);
}

void test_generate_session_id_buffer_too_small(void) {
    char buffer[10];
    bool result = generate_session_id(buffer, sizeof(buffer));
    TEST_ASSERT_FALSE(result);
}

void test_generate_session_id_exact_size(void) {
    char buffer[37]; // Exactly 37 bytes
    bool result = generate_session_id(buffer, sizeof(buffer));
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(36, strlen(buffer)); // UUID is 36 chars
}

void test_generate_session_id_success(void) {
    char buffer[100];
    bool result = generate_session_id(buffer, sizeof(buffer));
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(36, strlen(buffer));
    
    // Verify UUID format (8-4-4-4-12 with hyphens)
    TEST_ASSERT_EQUAL('-', buffer[8]);
    TEST_ASSERT_EQUAL('-', buffer[13]);
    TEST_ASSERT_EQUAL('-', buffer[18]);
    TEST_ASSERT_EQUAL('-', buffer[23]);
}

void test_generate_session_id_unique(void) {
    char id1[100], id2[100];
    
    bool result1 = generate_session_id(id1, sizeof(id1));
    bool result2 = generate_session_id(id2, sizeof(id2));
    
    TEST_ASSERT_TRUE(result1);
    TEST_ASSERT_TRUE(result2);
    TEST_ASSERT_NOT_EQUAL(0, strcmp(id1, id2)); // Should be different
}

/*
 * TEST SUITE: list_active_sessions() with sessions
 */

void test_list_active_sessions_with_one_session(void) {
    // Initialize manager
    init_session_manager(10, 300);
    
    // Create a mock session manually
    TerminalSession *session = calloc(1, sizeof(TerminalSession));
    TEST_ASSERT_NOT_NULL(session);
    
    strcpy(session->session_id, "test-session-001");
    session->active = true;
    pthread_mutex_init(&session->session_mutex, NULL);
    
    // Add to manager's list
    global_session_manager->active_sessions = session;
    global_session_manager->session_count = 1;
    
    // Call list_active_sessions
    char **session_ids = NULL;
    size_t count = 0;
    bool result = list_active_sessions(&session_ids, &count);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, count);
    TEST_ASSERT_NOT_NULL(session_ids);
    TEST_ASSERT_EQUAL_STRING("test-session-001", session_ids[0]);
    
    // Cleanup
    free(session_ids[0]);
    free(session_ids);
    
    pthread_mutex_destroy(&session->session_mutex);
    free(session);
    global_session_manager->active_sessions = NULL;
    global_session_manager->session_count = 0;
}

void test_list_active_sessions_with_multiple_sessions(void) {
    // Initialize manager
    init_session_manager(10, 300);
    
    // Create 3 mock sessions
    TerminalSession *session1 = calloc(1, sizeof(TerminalSession));
    TerminalSession *session2 = calloc(1, sizeof(TerminalSession));
    TerminalSession *session3 = calloc(1, sizeof(TerminalSession));
    
    TEST_ASSERT_NOT_NULL(session1);
    TEST_ASSERT_NOT_NULL(session2);
    TEST_ASSERT_NOT_NULL(session3);
    
    strcpy(session1->session_id, "session-001");
    strcpy(session2->session_id, "session-002");
    strcpy(session3->session_id, "session-003");
    
    session1->active = true;
    session2->active = true;
    session3->active = true;
    
    pthread_mutex_init(&session1->session_mutex, NULL);
    pthread_mutex_init(&session2->session_mutex, NULL);
    pthread_mutex_init(&session3->session_mutex, NULL);
    
    // Link sessions: 1 -> 2 -> 3
    session1->next = session2;
    session1->prev = NULL;
    
    session2->next = session3;
    session2->prev = session1;
    
    session3->next = NULL;
    session3->prev = session2;
    
    // Add to manager's list (head is session1)
    global_session_manager->active_sessions = session1;
    global_session_manager->session_count = 3;
    
    // Call list_active_sessions
    char **session_ids = NULL;
    size_t count = 0;
    bool result = list_active_sessions(&session_ids, &count);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(3, count);
    TEST_ASSERT_NOT_NULL(session_ids);
    TEST_ASSERT_EQUAL_STRING("session-001", session_ids[0]);
    TEST_ASSERT_EQUAL_STRING("session-002", session_ids[1]);
    TEST_ASSERT_EQUAL_STRING("session-003", session_ids[2]);
    
    // Cleanup
    for (size_t i = 0; i < count; i++) {
        free(session_ids[i]);
    }
    free(session_ids);
    
    pthread_mutex_destroy(&session1->session_mutex);
    pthread_mutex_destroy(&session2->session_mutex);
    pthread_mutex_destroy(&session3->session_mutex);
    
    free(session1);
    free(session2);
    free(session3);
    
    global_session_manager->active_sessions = NULL;
    global_session_manager->session_count = 0;
}

/*
 * TEST SUITE: cleanup_expired_sessions() with expired sessions
 */

void test_cleanup_expired_sessions_with_one_expired(void) {
    // Initialize manager with 1 second timeout
    init_session_manager(10, 1);
    
    // Create a mock session with old activity time
    TerminalSession *session = calloc(1, sizeof(TerminalSession));
    TEST_ASSERT_NOT_NULL(session);
    
    strcpy(session->session_id, "expired-session");
    session->active = true;
    session->last_activity = time(NULL) - 10; // 10 seconds ago
    pthread_mutex_init(&session->session_mutex, NULL);
    
    // Add to manager's list
    global_session_manager->active_sessions = session;
    global_session_manager->session_count = 1;
    
    // Call cleanup - should remove the expired session
    int cleaned = cleanup_expired_sessions();
    
    TEST_ASSERT_EQUAL(1, cleaned);
    TEST_ASSERT_NULL(global_session_manager->active_sessions);
    TEST_ASSERT_EQUAL(0, global_session_manager->session_count);
}

void test_cleanup_expired_sessions_mixed_expired_and_active(void) {
    // Initialize manager with 2 second timeout
    init_session_manager(10, 2);
    
    time_t now = time(NULL);
    
    // Create 3 sessions: expired, active, expired
    TerminalSession *session1 = calloc(1, sizeof(TerminalSession));
    TerminalSession *session2 = calloc(1, sizeof(TerminalSession));
    TerminalSession *session3 = calloc(1, sizeof(TerminalSession));
    
    TEST_ASSERT_NOT_NULL(session1);
    TEST_ASSERT_NOT_NULL(session2);
    TEST_ASSERT_NOT_NULL(session3);
    
    strcpy(session1->session_id, "expired-001");
    strcpy(session2->session_id, "active-001");
    strcpy(session3->session_id, "expired-002");
    
    session1->active = true;
    session2->active = true;
    session3->active = true;
    
    session1->last_activity = now - 10; // Expired
    session2->last_activity = now;      // Active
    session3->last_activity = now - 10; // Expired
    
    pthread_mutex_init(&session1->session_mutex, NULL);
    pthread_mutex_init(&session2->session_mutex, NULL);
    pthread_mutex_init(&session3->session_mutex, NULL);
    
    // Link sessions: 1 -> 2 -> 3
    session1->next = session2;
    session1->prev = NULL;
    
    session2->next = session3;
    session2->prev = session1;
    
    session3->next = NULL;
    session3->prev = session2;
    
    // Add to manager's list
    global_session_manager->active_sessions = session1;
    global_session_manager->session_count = 3;
    
    // Call cleanup - should remove 2 expired sessions
    int cleaned = cleanup_expired_sessions();
    
    TEST_ASSERT_EQUAL(2, cleaned);
    TEST_ASSERT_NOT_NULL(global_session_manager->active_sessions);
    TEST_ASSERT_EQUAL(1, global_session_manager->session_count);
    TEST_ASSERT_EQUAL_STRING("active-001", global_session_manager->active_sessions->session_id);
    
    // Cleanup remaining session
    pthread_mutex_destroy(&session2->session_mutex);
    free(session2);
    global_session_manager->active_sessions = NULL;
    global_session_manager->session_count = 0;
}

/*
 * TEST SUITE: remove_terminal_session() linked list manipulation
 */

void test_remove_terminal_session_from_head(void) {
    // Initialize manager
    init_session_manager(10, 300);
    
    // Create 2 sessions
    TerminalSession *session1 = calloc(1, sizeof(TerminalSession));
    TerminalSession *session2 = calloc(1, sizeof(TerminalSession));
    
    TEST_ASSERT_NOT_NULL(session1);
    TEST_ASSERT_NOT_NULL(session2);
    
    strcpy(session1->session_id, "session-head");
    strcpy(session2->session_id, "session-tail");
    
    session1->active = true;
    session2->active = true;
    
    pthread_mutex_init(&session1->session_mutex, NULL);
    pthread_mutex_init(&session2->session_mutex, NULL);
    
    // Link: 1 -> 2
    session1->next = session2;
    session1->prev = NULL;
    session2->next = NULL;
    session2->prev = session1;
    
    // Add to manager (head is session1)
    global_session_manager->active_sessions = session1;
    global_session_manager->session_count = 2;
    
    // Remove head session
    bool result = remove_terminal_session(session1);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(session2, global_session_manager->active_sessions);
    TEST_ASSERT_EQUAL(1, global_session_manager->session_count);
    TEST_ASSERT_NULL(session2->prev); // session2 is now head
    
    // Cleanup
    pthread_mutex_destroy(&session2->session_mutex);
    free(session2);
    global_session_manager->active_sessions = NULL;
    global_session_manager->session_count = 0;
}

void test_remove_terminal_session_from_middle(void) {
    // Initialize manager
    init_session_manager(10, 300);
    
    // Create 3 sessions
    TerminalSession *session1 = calloc(1, sizeof(TerminalSession));
    TerminalSession *session2 = calloc(1, sizeof(TerminalSession));
    TerminalSession *session3 = calloc(1, sizeof(TerminalSession));
    
    TEST_ASSERT_NOT_NULL(session1);
    TEST_ASSERT_NOT_NULL(session2);
    TEST_ASSERT_NOT_NULL(session3);
    
    strcpy(session1->session_id, "session-001");
    strcpy(session2->session_id, "session-002");
    strcpy(session3->session_id, "session-003");
    
    session1->active = true;
    session2->active = true;
    session3->active = true;
    
    pthread_mutex_init(&session1->session_mutex, NULL);
    pthread_mutex_init(&session2->session_mutex, NULL);
    pthread_mutex_init(&session3->session_mutex, NULL);
    
    // Link: 1 -> 2 -> 3
    session1->next = session2;
    session1->prev = NULL;
    
    session2->next = session3;
    session2->prev = session1;
    
    session3->next = NULL;
    session3->prev = session2;
    
    // Add to manager (head is session1)
    global_session_manager->active_sessions = session1;
    global_session_manager->session_count = 3;
    
    // Remove middle session (session2)
    bool result = remove_terminal_session(session2);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(session1, global_session_manager->active_sessions);
    TEST_ASSERT_EQUAL(2, global_session_manager->session_count);
    
    // Verify links: 1 -> 3
    TEST_ASSERT_EQUAL(session3, session1->next);
    TEST_ASSERT_EQUAL(session1, session3->prev);
    
    // Cleanup remaining sessions
    pthread_mutex_destroy(&session1->session_mutex);
    pthread_mutex_destroy(&session3->session_mutex);
    free(session1);
    free(session3);
    global_session_manager->active_sessions = NULL;
    global_session_manager->session_count = 0;
}

void test_remove_terminal_session_from_tail(void) {
    // Initialize manager
    init_session_manager(10, 300);
    
    // Create 2 sessions
    TerminalSession *session1 = calloc(1, sizeof(TerminalSession));
    TerminalSession *session2 = calloc(1, sizeof(TerminalSession));
    
    TEST_ASSERT_NOT_NULL(session1);
    TEST_ASSERT_NOT_NULL(session2);
    
    strcpy(session1->session_id, "session-head");
    strcpy(session2->session_id, "session-tail");
    
    session1->active = true;
    session2->active = true;
    
    pthread_mutex_init(&session1->session_mutex, NULL);
    pthread_mutex_init(&session2->session_mutex, NULL);
    
    // Link: 1 -> 2
    session1->next = session2;
    session1->prev = NULL;
    session2->next = NULL;
    session2->prev = session1;
    
    // Add to manager (head is session1)
    global_session_manager->active_sessions = session1;
    global_session_manager->session_count = 2;
    
    // Remove tail session
    bool result = remove_terminal_session(session2);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(session1, global_session_manager->active_sessions);
    TEST_ASSERT_EQUAL(1, global_session_manager->session_count);
    TEST_ASSERT_NULL(session1->next); // session1 is now tail
    
    // Cleanup
    pthread_mutex_destroy(&session1->session_mutex);
    free(session1);
    global_session_manager->active_sessions = NULL;
    global_session_manager->session_count = 0;
}

int main(void) {
    UNITY_BEGIN();
    
    // generate_session_id() tests
    RUN_TEST(test_generate_session_id_null_buffer);
    RUN_TEST(test_generate_session_id_buffer_too_small);
    RUN_TEST(test_generate_session_id_exact_size);
    RUN_TEST(test_generate_session_id_success);
    RUN_TEST(test_generate_session_id_unique);
    
    // list_active_sessions() tests
    RUN_TEST(test_list_active_sessions_with_one_session);
    RUN_TEST(test_list_active_sessions_with_multiple_sessions);
    
    // cleanup_expired_sessions() tests
    RUN_TEST(test_cleanup_expired_sessions_with_one_expired);
    RUN_TEST(test_cleanup_expired_sessions_mixed_expired_and_active);
    
    // remove_terminal_session() linked list tests
    RUN_TEST(test_remove_terminal_session_from_head);
    RUN_TEST(test_remove_terminal_session_from_middle);
    RUN_TEST(test_remove_terminal_session_from_tail);
    
    return UNITY_END();
}