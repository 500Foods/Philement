/*
 * Terminal Session Management Implementation
 *
 * This module implements terminal session lifecycle management, concurrent
 * session tracking, and thread-safe operations with mutexes for protecting
 * shared state.
 */

#include <src/hydrogen.h>
#include "../globals.h"
#include <src/logging/logging.h>
#include <src/utils/utils.h>
#include "terminal_session.h"
#include "terminal_shell.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <uuid/uuid.h>
#include <time.h>

#ifdef __linux__
#include <sys/syscall.h>
#include <linux/random.h>
#endif

// Global session manager instance
SessionManager *global_session_manager = NULL;

// Test-friendly configuration
static int test_mode_cleanup_interval = 30; // Default 30 seconds
static bool test_mode_disable_cleanup_thread = false;

/**
 * Background cleanup thread function
 *
 * Periodically cleans up expired sessions in the background.
 */
void *session_cleanup_thread(void *arg) {
    (void)arg; // Suppress unused parameter warning

    log_this(SR_TERMINAL, "Session cleanup thread started", LOG_LEVEL_STATE, 0);

    while (global_session_manager && global_session_manager->cleanup_thread_running) {
        // Sleep for configurable interval (default 30 seconds, shorter for testing)
        sleep((unsigned int)test_mode_cleanup_interval);

        if (!global_session_manager || !global_session_manager->cleanup_thread_running) {
            break;
        }

        // Perform cleanup
        int cleaned_count = cleanup_expired_sessions();
        if (cleaned_count > 0) {
            log_this(SR_TERMINAL, "Cleaned up %d expired sessions", LOG_LEVEL_STATE, 1, cleaned_count);
        }
    }

    log_this(SR_TERMINAL, "Session cleanup thread terminated", LOG_LEVEL_STATE, 0);
    return NULL;
}

/**
 * Generate unique session ID
 *
 * Creates a unique session identifier using UUID.
 *
 * @param session_id Output buffer to store the session ID
 * @param size Size of the output buffer
 * @return true on success, false on failure
 */
bool generate_session_id(char *session_id, size_t size) {
    if (!session_id || size < 37) { // UUID string is 36 chars + null terminator
        return false;
    }

    uuid_t uuid;
    uuid_generate(uuid);
    uuid_unparse(uuid, session_id);
    return true;
}

/**
 * Initialize session manager
 *
 * Creates and initializes the global session manager with the specified
 * configuration values.
 */
bool init_session_manager(int max_sessions, int idle_timeout_seconds) {
    if (global_session_manager != NULL) {
        log_this(SR_TERMINAL, "Session manager already initialized", LOG_LEVEL_ALERT, 0);
        return true;
    }

    global_session_manager = calloc(1, sizeof(SessionManager));
    if (!global_session_manager) {
        log_this(SR_TERMINAL, "Failed to allocate session manager", LOG_LEVEL_ERROR, 0);
        return false;
    }

    global_session_manager->max_sessions = max_sessions;
    global_session_manager->idle_timeout_seconds = idle_timeout_seconds;
    global_session_manager->active_sessions = NULL;
    global_session_manager->session_count = 0;
    global_session_manager->cleanup_thread_running = false;

    // Initialize mutexes and locks
    if (pthread_mutex_init(&global_session_manager->manager_mutex, NULL) != 0) {
        log_this(SR_TERMINAL, "Failed to initialize manager mutex", LOG_LEVEL_ERROR, 0);
        free(global_session_manager);
        global_session_manager = NULL;
        return false;
    }

    if (pthread_rwlock_init(&global_session_manager->sessions_lock, NULL) != 0) {
        log_this(SR_TERMINAL, "Failed to initialize sessions lock", LOG_LEVEL_ERROR, 0);
        pthread_mutex_destroy(&global_session_manager->manager_mutex);
        free(global_session_manager);
        global_session_manager = NULL;
        return false;
    }

    // Start cleanup thread (unless disabled for testing)
    if (!test_mode_disable_cleanup_thread) {
        global_session_manager->cleanup_thread_running = true;
        if (pthread_create(&global_session_manager->cleanup_thread, NULL,
                          session_cleanup_thread, NULL) != 0) {
            log_this(SR_TERMINAL, "Failed to create cleanup thread", LOG_LEVEL_ERROR, 0);
            global_session_manager->cleanup_thread_running = false;
            // Continue without cleanup thread - not critical
        }
    } else {
        global_session_manager->cleanup_thread_running = false;
    }

    log_this(SR_TERMINAL, "Session manager initialized - max_sessions: %d, idle_timeout: %d seconds", LOG_LEVEL_STATE, 2, max_sessions, idle_timeout_seconds);

    return true;
}

/**
 * Cleanup session manager
 *
 * Terminates all active sessions and cleans up resources used by the session manager.
 */
void cleanup_session_manager(void) {
    if (!global_session_manager) {
        return;
    }

    log_this(SR_TERMINAL, "Cleaning up session manager", LOG_LEVEL_STATE, 0);

    // Stop cleanup thread
    if (global_session_manager->cleanup_thread_running) {
        global_session_manager->cleanup_thread_running = false;
        pthread_join(global_session_manager->cleanup_thread, NULL);
    }

    pthread_mutex_lock(&global_session_manager->manager_mutex);

    // Terminate all sessions
    TerminalSession *session = global_session_manager->active_sessions;
    while (session) {
        TerminalSession *next = session->next;
        remove_terminal_session(session);
        session = next;
    }

    // Destroy locks
    pthread_rwlock_destroy(&global_session_manager->sessions_lock);
    pthread_mutex_destroy(&global_session_manager->manager_mutex);

    free(global_session_manager);
    global_session_manager = NULL;

    log_this(SR_TERMINAL, "Session manager cleanup completed", LOG_LEVEL_STATE, 0);
}

/**
 * Create new terminal session
 *
 * Creates a new terminal session with a unique ID and initializes all
 * necessary state. Launches PTY shell in background.
 */
TerminalSession *create_terminal_session(const char *shell_command,
                                        int initial_rows, int initial_cols) {
    if (!global_session_manager || !shell_command) {
        log_this(SR_TERMINAL, "Invalid parameters for session creation", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    pthread_mutex_lock(&global_session_manager->manager_mutex);

    // Check capacity
    if (!session_manager_has_capacity()) {
        pthread_mutex_unlock(&global_session_manager->manager_mutex);
        log_this(SR_TERMINAL, "Maximum session capacity reached", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Allocate session
    TerminalSession *session = calloc(1, sizeof(TerminalSession));
    if (!session) {
        pthread_mutex_unlock(&global_session_manager->manager_mutex);
        log_this(SR_TERMINAL, "Failed to allocate terminal session", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // Generate unique session ID
    if (!generate_session_id(session->session_id, sizeof(session->session_id))) {
        log_this(SR_TERMINAL, "Failed to generate session ID", LOG_LEVEL_ERROR, 0);
        free(session);
        pthread_mutex_unlock(&global_session_manager->manager_mutex);
        return NULL;
    }

    // Initialize session state
    session->created_time = time(NULL);
    session->last_activity = session->created_time;
    session->terminal_rows = initial_rows;
    session->terminal_cols = initial_cols;
    session->active = false;
    session->connected = false;

    // Initialize session mutex
    if (pthread_mutex_init(&session->session_mutex, NULL) != 0) {
        log_this(SR_TERMINAL, "Failed to initialize session mutex", LOG_LEVEL_ERROR, 0);
        free(session);
        pthread_mutex_unlock(&global_session_manager->manager_mutex);
        return NULL;
    }

    // Start PTY shell
    session->pty_shell = pty_spawn_shell(shell_command, session);
    if (!session->pty_shell) {
        log_this(SR_TERMINAL, "Failed to spawn PTY shell for session %s", LOG_LEVEL_ERROR, 1, session->session_id);
        pthread_mutex_destroy(&session->session_mutex);
        free(session);
        pthread_mutex_unlock(&global_session_manager->manager_mutex);
        return NULL;
    }

    session->active = true;

    // Add to sessions list
    pthread_rwlock_wrlock(&global_session_manager->sessions_lock);

    if (global_session_manager->active_sessions) {
        global_session_manager->active_sessions->prev = session;
        session->next = global_session_manager->active_sessions;
    }
    global_session_manager->active_sessions = session;
    global_session_manager->session_count++;

    pthread_rwlock_unlock(&global_session_manager->sessions_lock);
    pthread_mutex_unlock(&global_session_manager->manager_mutex);

    log_this(SR_TERMINAL, "Created terminal session %s (%dx%d)", LOG_LEVEL_STATE, 3, session->session_id, initial_cols, initial_rows);

    return session;
}

/**
 * Get terminal session by ID
 */
TerminalSession *get_terminal_session(const char *session_id) {
    if (!global_session_manager || !session_id) {
        return NULL;
    }

    pthread_rwlock_rdlock(&global_session_manager->sessions_lock);

    TerminalSession *session = global_session_manager->active_sessions;
    while (session) {
        if (strcmp(session->session_id, session_id) == 0) {
            // Update activity timestamp
            update_session_activity(session);
            break;
        }
        session = session->next;
    }

    pthread_rwlock_unlock(&global_session_manager->sessions_lock);
    return session;
}

/**
 * Remove terminal session
 */
bool remove_terminal_session(TerminalSession *session) {
    if (!global_session_manager || !session) {
        return false;
    }

    pthread_mutex_lock(&session->session_mutex);

    if (!session->active) {
        pthread_mutex_unlock(&session->session_mutex);
        return false;
    }

    log_this(SR_TERMINAL, "Removing terminal session %s", LOG_LEVEL_STATE, 1, session->session_id);

    // Terminate PTY shell
    if (session->pty_shell) {
        pty_cleanup_shell(session->pty_shell);
        session->pty_shell = NULL;
    }

    session->active = false;
    session->connected = false;

    pthread_mutex_unlock(&session->session_mutex);
    pthread_mutex_destroy(&session->session_mutex);

    // Remove from sessions list
    pthread_rwlock_wrlock(&global_session_manager->sessions_lock);

    if (session->prev) {
        session->prev->next = session->next;
    } else {
        global_session_manager->active_sessions = session->next;
    }

    if (session->next) {
        session->next->prev = session->prev;
    }

    global_session_manager->session_count--;
    pthread_rwlock_unlock(&global_session_manager->sessions_lock);

    free(session);
    return true;
}

/**
 * Update session activity timestamp
 */
void update_session_activity(TerminalSession *session) {
    if (session) {
        session->last_activity = time(NULL);
    }
}

/**
 * Check and cleanup expired sessions
 */
int cleanup_expired_sessions(void) {
    if (!global_session_manager) {
        return 0;
    }

    int cleaned_count = 0;
    time_t current_time = time(NULL);

    // Use write lock since we'll be modifying the list
    pthread_rwlock_wrlock(&global_session_manager->sessions_lock);

    TerminalSession *session = global_session_manager->active_sessions;
    while (session) {
        TerminalSession *next = session->next;

        // Check if session is expired
        if (global_session_manager->idle_timeout_seconds > 0 &&
            difftime(current_time, session->last_activity) >= global_session_manager->idle_timeout_seconds) {
            log_this(SR_TERMINAL, "Session %s expired due to idle timeout", LOG_LEVEL_STATE, 1, session->session_id);

            // Lock session mutex for cleanup
            pthread_mutex_lock(&session->session_mutex);

            // Terminate PTY shell if present
            if (session->pty_shell) {
                pty_cleanup_shell(session->pty_shell);
                session->pty_shell = NULL;
            }

            session->active = false;
            session->connected = false;

            pthread_mutex_unlock(&session->session_mutex);
            pthread_mutex_destroy(&session->session_mutex);

            // Remove from current position in list
            if (session->prev) {
                session->prev->next = session->next;
            } else {
                global_session_manager->active_sessions = session->next;
            }
            if (session->next) {
                session->next->prev = session->prev;
            }
            global_session_manager->session_count--;

            // Free the session
            free(session);
            cleaned_count++;
        }

        session = next;
    }

    pthread_rwlock_unlock(&global_session_manager->sessions_lock);
    return cleaned_count;
}

/**
 * Resize terminal session
 */
bool resize_terminal_session(TerminalSession *session, int rows, int cols) {
    if (!session) {
        return false;
    }

    pthread_mutex_lock(&session->session_mutex);

    session->terminal_rows = rows;
    session->terminal_cols = cols;

    bool success = false;
    if (session->pty_shell) {
        success = pty_set_size(session->pty_shell, rows > 0 ? (unsigned short)rows : 24,
                              cols > 0 ? (unsigned short)cols : 80);
    }

    pthread_mutex_unlock(&session->session_mutex);

    if (success) {
        update_session_activity(session);
        log_this(SR_TERMINAL, "Resized session %s to %dx%d", LOG_LEVEL_STATE, 3, session->session_id, cols, rows);
    }

    return success;
}

/**
 * Send data to terminal session
 */
int send_data_to_session(TerminalSession *session, const char *data, size_t size) {
    if (!session || !session->active) {
        return -1;
    }

    pthread_mutex_lock(&session->session_mutex);
    int result = -1;

    if (session->pty_shell && pty_is_running(session->pty_shell)) {
        result = pty_write_data(session->pty_shell, data, size);
        if (result > 0) {
            update_session_activity(session);
        }
    }

    pthread_mutex_unlock(&session->session_mutex);
    return result;
}

/**
 * Read data from terminal session
 */
int read_data_from_session(TerminalSession *session, char *buffer, size_t buffer_size) {
    if (!session || !session->active) {
        return -1;
    }

    pthread_mutex_lock(&session->session_mutex);
    int result = -1;

    if (session->pty_shell && pty_is_running(session->pty_shell)) {
        result = pty_read_data(session->pty_shell, buffer, buffer_size);
    }

    pthread_mutex_unlock(&session->session_mutex);
    return result;
}

/**
 * Get session manager statistics
 */
bool get_session_manager_stats(size_t *active_sessions, size_t *max_sessions) {
    if (!global_session_manager) {
        return false;
    }

    if (active_sessions) {
        *active_sessions = (size_t)global_session_manager->session_count;
    }

    if (max_sessions) {
        *max_sessions = (size_t)global_session_manager->max_sessions;
    }

    return true;
}

/**
 * List active session IDs
 */
bool list_active_sessions(char ***session_ids, size_t *count) {
    if (!global_session_manager || !session_ids || !count) {
        return false;
    }

    pthread_rwlock_rdlock(&global_session_manager->sessions_lock);

    *count = (size_t)global_session_manager->session_count;
    if (*count == 0) {
        *session_ids = NULL;
        pthread_rwlock_unlock(&global_session_manager->sessions_lock);
        return true;
    }

    *session_ids = calloc(*count, sizeof(char *));
    if (!*session_ids) {
        pthread_rwlock_unlock(&global_session_manager->sessions_lock);
        return false;
    }

    TerminalSession *session = global_session_manager->active_sessions;
    size_t index = 0;

        while (session && index < *count) {
            (*session_ids)[index] = strdup(session->session_id);
            if (!(*session_ids)[index]) {
                // Cleanup on failure
                size_t cleanup_idx;
                for (cleanup_idx = 0; cleanup_idx < index; cleanup_idx++) {
                    free((*session_ids)[cleanup_idx]);
                }
                free(*session_ids);
                *session_ids = NULL;
                *count = 0;
                return false;
            }
            index++;
            session = session->next;
        }

    pthread_rwlock_unlock(&global_session_manager->sessions_lock);
    return true;
}

/**
 * Terminate all active sessions
 */
int terminate_all_sessions(void) {
    if (!global_session_manager) {
        return 0;
    }

    int terminated_count = 0;
    TerminalSession *session = global_session_manager->active_sessions;

    while (session) {
        TerminalSession *next = session->next;
        if (remove_terminal_session(session)) {
            terminated_count++;
        }
        session = next;
    }

    return terminated_count;
}

/**
 * Check if session manager has capacity
 */
bool session_manager_has_capacity(void) {
    if (!global_session_manager) {
        return false;
    }

    return (size_t)global_session_manager->session_count < (size_t)global_session_manager->max_sessions;
}

/**
 * Test control functions - for making the code more testable
 */
void terminal_session_set_test_cleanup_interval(int seconds) {
    test_mode_cleanup_interval = seconds;
}

void terminal_session_disable_cleanup_thread(void) {
    test_mode_disable_cleanup_thread = true;
}

void terminal_session_enable_cleanup_thread(void) {
    test_mode_disable_cleanup_thread = false;
}