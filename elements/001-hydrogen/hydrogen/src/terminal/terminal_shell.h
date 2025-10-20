/*
 * Terminal PTY Shell Management Header
 *
 * Defines the interface for PTY (pseudo-terminal) process management functions
 * used by terminal sessions to spawn and manage shell processes.
 */

#ifndef TERMINAL_SHELL_H
#define TERMINAL_SHELL_H

#include <stdbool.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
struct TerminalSession;

/**
 * PtyShell structure definition
 * Exposed to other modules for accessing master_fd in WebSocket bridging
 */
typedef struct PtyShell {
    pid_t pid;              /**< Child process PID */
    int master_fd;          /**< Master PTY file descriptor */
    int slave_fd;           /**< Slave PTY file descriptor - only used during spawn */
    char *slave_name;       /**< Slave PTY device name - only used during spawn */
    bool running;           /**< Whether the shell process is running */
    struct TerminalSession *session; /**< Associated terminal session */
} PtyShell;

/**
 * Create and configure a PTY pair
 *
 * @param master_fd Pointer to store master file descriptor
 * @param slave_fd Pointer to store slave file descriptor
 * @param slave_name Buffer to store slave device name (at least 256 bytes)
 * @return true on success, false on failure
 */
bool create_pty_pair(int *master_fd, int *slave_fd, char *slave_name);

/**
 * Configure master file descriptor as non-blocking
 *
 * @param master_fd File descriptor to configure
 * @return true on success, false on failure
 */
bool configure_master_fd(int master_fd);

/**
 * Spawn a new shell process using PTY
 *
 * This function creates a new pseudo-terminal pair, forks a child process,
 * and executes the configured shell command in the child process.
 *
 * @param shell_command The shell command to execute (e.g., "/bin/bash")
 * @param session Pointer to terminal session for logging and tracking
 * @return PtyShell structure on success, NULL on failure
 */
PtyShell *pty_spawn_shell(const char *shell_command, struct TerminalSession *session);

/**
 * Send data to the PTY master (towards the shell)
 *
 * @param shell Pointer to PtyShell structure
 * @param data Buffer containing data to send
 * @param size Size of data buffer
 * @return Number of bytes written, or -1 on error
 */
int pty_write_data(PtyShell *shell, const char *data, size_t size);

/**
 * Read data from the PTY master (from the shell)
 *
 * @param shell Pointer to PtyShell structure
 * @param buffer Buffer to store read data
 * @param size Size of buffer
 * @return Number of bytes read, 0 on EWOULDBLOCK, or -1 on error
 */
int pty_read_data(PtyShell *shell, char *buffer, size_t size);

/**
 * Set terminal window size for PTY
 *
 * @param shell Pointer to PtyShell structure
 * @param rows Number of rows
 * @param cols Number of columns
 * @return true on success, false on failure
 */
bool pty_set_size(PtyShell *shell, unsigned short rows, unsigned short cols);

/**
 * Check if shell process is still running
 *
 * @param shell Pointer to PtyShell structure
 * @return true if running, false if terminated
 */
bool pty_is_running(PtyShell *shell);

/**
 * Terminate the shell process gracefully
 *
 * @param shell Pointer to PtyShell structure
 * @return true on success, false on failure
 */
bool pty_terminate_shell(PtyShell *shell);

/**
 * Clean up PTY shell resources
 *
 * @param shell Pointer to PtyShell structure (will be freed)
 */
void pty_cleanup_shell(PtyShell *shell);

/**
 * Setup child process with PTY and execute shell
 *
 * @param shell_command The shell command to execute
 * @param slave_fd The slave PTY file descriptor
 * @param master_fd The master PTY file descriptor to close in child
 */
void setup_child_process(const char *shell_command, int slave_fd, int master_fd);

#ifdef __cplusplus
}
#endif

#endif /* TERMINAL_SHELL_H */
