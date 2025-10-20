/*
 * Terminal Session Management Header
 *
 * Defines structures and functions for managing terminal sessions, including
 * session lifecycle, concurrent session tracking, and thread-safe operations
 * with mutexes for protecting shared state.
 */

#ifndef TERMINAL_SESSION_H
#define TERMINAL_SESSION_H

#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include "../globals.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
struct PtyShell;

/**
 * Terminal Session Structure
 *
 * Represents an individual terminal session with all necessary state
 * information for lifecycle management and I/O bridging.
 */
typedef struct TerminalSession {
    char session_id[64];          /**< Unique session identifier */
    time_t created_time;          /**< Session creation timestamp */
    time_t last_activity;         /**< Last activity timestamp for idle detection */

    struct PtyShell *pty_shell;    /**< Associated PTY shell process */

    // WebSocket connection state (to be populated later)
    void *websocket_connection;   /**< WebSocket connection handle */
    void *pty_bridge_context;     /**< PTY bridge context for cleanup */

    // Session configuration
    int terminal_rows;            /**< Current terminal rows */
    int terminal_cols;            /**< Current terminal columns */

    // Thread safety
    pthread_mutex_t session_mutex; /**< Mutex for thread-safe operations */

    // Session state
    bool active;                  /**< Whether session is active */
    bool connected;               /**< Whether WebSocket is connected */

    // Memory management
    struct TerminalSession *next;  /**< Next session in linked list */
    struct TerminalSession *prev;  /**< Previous session in linked list */
} TerminalSession;

/**
 * Session Manager Structure
 *
 * Manages all active terminal sessions with proper resource management,
 * thread safety, and lifecycle tracking.
 */
typedef struct SessionManager {
    TerminalSession *active_sessions;  /**< Linked list of active sessions */
    size_t session_count;              /**< Current number of active sessions */

    int max_sessions;                  /**< Maximum allowed concurrent sessions */
    int idle_timeout_seconds;          /**< Session idle timeout in seconds */

    // Global thread safety
    pthread_mutex_t manager_mutex;     /**< Mutex for manager operations */
    pthread_rwlock_t sessions_lock;    /**< Read-write lock for sessions */

    // Cleanup thread for expired sessions
    pthread_t cleanup_thread;          /**< Background cleanup thread */
    bool cleanup_thread_running;       /**< Whether cleanup thread is active */
} SessionManager;

// Global session manager instance
extern SessionManager *global_session_manager;

/**
 * Initialize session manager
 *
 * Creates and initializes the global session manager with the specified
 * configuration values.
 *
 * @param max_sessions Maximum number of concurrent sessions
 * @param idle_timeout_seconds Session idle timeout in seconds
 * @return true on success, false on failure
 */
bool init_session_manager(int max_sessions, int idle_timeout_seconds);

/**
 * Cleanup session manager
 *
 * Terminates all active sessions and cleans up resources used by the session manager.
 */
void cleanup_session_manager(void);

/**
 * Create new terminal session
 *
 * Creates a new terminal session with a unique ID and initializes all
 * necessary state. Launches PTY shell in background.
 *
 * @param shell_command The shell command to execute (e.g., "/bin/bash")
 * @param initial_rows Initial terminal rows
 * @param initial_cols Initial terminal columns
 * @return TerminalSession pointer on success, NULL on failure
 */
TerminalSession *create_terminal_session(const char *shell_command,
                                        int initial_rows, int initial_cols);

/**
 * Get terminal session by ID
 *
 * Retrieves a terminal session by its unique identifier.
 *
 * @param session_id The session identifier to search for
 * @return TerminalSession pointer if found, NULL if not found
 */
TerminalSession *get_terminal_session(const char *session_id);

/**
 * Remove terminal session
 *
 * Removes and cleans up a terminal session. If the session is active,
 * it will be properly terminated first.
 *
 * @param session The session to remove
 * @return true on success, false on failure
 */
bool remove_terminal_session(TerminalSession *session);

/**
 * Update session activity timestamp
 *
 * Updates the last activity timestamp for a session to prevent idle timeout.
 *
 * @param session The session to update
 */
void update_session_activity(TerminalSession *session);

/**
 * Check and cleanup expired sessions
 *
 * Iterates through all sessions and removes those that have exceeded
 * the idle timeout threshold.
 *
 * @return Number of sessions cleaned up
 */
int cleanup_expired_sessions(void);

/**
 * Resize terminal session
 *
 * Updates the terminal dimensions for a session and propagates the
 * changes to the underlying PTY.
 *
 * @param session The session to resize
 * @param rows New number of rows
 * @param cols New number of columns
 * @return true on success, false on failure
 */
bool resize_terminal_session(TerminalSession *session, int rows, int cols);

/**
 * Send data to terminal session
 *
 * Sends data to the PTY shell associated with the session.
 *
 * @param session The target session
 * @param data The data to send
 * @param size Size of the data buffer
 * @return Number of bytes written, or -1 on error
 */
int send_data_to_session(TerminalSession *session, const char *data, size_t size);

/**
 * Read data from terminal session
 *
 * Reads available data from the PTY shell associated with the session.
 *
 * @param session The source session
 * @param buffer Buffer to store read data
 * @param buffer_size Size of the buffer
 * @return Number of bytes read, 0 if no data available, or -1 on error
 */
int read_data_from_session(TerminalSession *session, char *buffer, size_t buffer_size);

/**
 * Get session manager statistics
 *
 * Returns information about the current state of the session manager.
 *
 * @param active_sessions Pointer to store number of active sessions
 * @param max_sessions Pointer to store maximum allowed sessions
 * @return true on success, false on failure
 */
bool get_session_manager_stats(size_t *active_sessions, size_t *max_sessions);

/**
 * List active session IDs
 *
 * Returns an array of all active session IDs. The caller is responsible
 * for freeing the array when done.
 *
 * @param session_ids Pointer to store array of session ID strings
 * @param count Pointer to store number of session IDs
 * @return true on success, false on failure
 */
bool list_active_sessions(char ***session_ids, size_t *count);

/**
 * Terminate all active sessions
 *
 * Forces termination of all active terminal sessions. Used during shutdown.
 *
 * @return Number of sessions terminated
 */
int terminate_all_sessions(void);

/**
 * Check if session manager has capacity
 *
 * Verifies whether the session manager can accommodate a new session
 * based on the maximum session limit.
 *
 * @return true if capacity is available, false if at limit
 */
bool session_manager_has_capacity(void);

/**
 * Generate unique session ID
 *
 * Creates a unique session identifier using UUID. Exposed for testing.
 *
 * @param session_id Output buffer to store the session ID
 * @param size Size of the output buffer (must be at least 37 bytes)
 * @return true on success, false on failure
 */
bool generate_session_id(char *session_id, size_t size);

/**
 * Test control functions - for making the code more testable
 *
 * These functions allow tests to control the behavior of the session manager
 * to make testing more reliable and faster.
 */

/**
 * Set cleanup thread sleep interval for testing
 *
 * @param seconds Sleep interval in seconds (default is 30)
 */
void terminal_session_set_test_cleanup_interval(int seconds);

/**
 * Disable cleanup thread for testing
 *
 * Prevents the cleanup thread from starting, which avoids hanging tests
 */
void terminal_session_disable_cleanup_thread(void);

/**
 * Enable cleanup thread
 *
 * Re-enables the cleanup thread (default behavior)
 */
void terminal_session_enable_cleanup_thread(void);

/**
 * Background cleanup thread function
 *
 * Periodically cleans up expired sessions in the background.
 */
void *session_cleanup_thread(void *arg);

#ifdef __cplusplus
}
#endif

#endif /* TERMINAL_SESSION_H */
