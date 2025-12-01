/*
 * Terminal PTY Shell Management
 *
 * This module handles PTY (pseudo-terminal) creation, shell process spawning,
 * and I/O operations. It provides functions for creating PTY pairs, spawning
 * shell processes, and managing data flow between terminals and shells.
 */

// System includes
#include <pty.h>
#include <sys/wait.h>

 // Project includes
#include <src/hydrogen.h>
#include <src/globals.h>
#include <src/logging/logging.h>
#include <src/utils/utils.h>
                    
// Local includes
#include "terminal.h"
#include "terminal_session.h"
#include "terminal_shell.h"

// Test mode variables for forcing failures in unit tests
#ifdef UNITY_TEST_MODE
bool test_mode_force_openpty_failure = false;
bool test_mode_force_calloc_failure = false;
bool test_mode_force_strdup_failure = false;
bool test_mode_force_fcntl_failure = false;
bool test_mode_force_fork_failure = false;
#endif
                 
/**
 * Create and configure a PTY pair
 *
 * @param master_fd Pointer to store master file descriptor
 * @param slave_fd Pointer to store slave file descriptor
 * @param slave_name Buffer to store slave device name (at least 256 bytes)
 * @return true on success, false on failure
 */
bool create_pty_pair(int *master_fd, int *slave_fd, char *slave_name) {
    if (!master_fd || !slave_fd || !slave_name) {
        return false;
    }

#ifdef UNITY_TEST_MODE
    if (test_mode_force_openpty_failure) {
        errno = ENOMEM; // Simulate failure
        log_this(SR_TERMINAL, "Failed to create PTY pair: %s", LOG_LEVEL_ERROR, 1, strerror(errno));
        return false;
    }
#endif

    if (openpty(master_fd, slave_fd, slave_name, NULL, NULL) == -1) {
        log_this(SR_TERMINAL, "Failed to create PTY pair: %s", LOG_LEVEL_ERROR, 1, strerror(errno));
        return false;
    }

    return true;
}

/**
 * Configure master file descriptor as non-blocking
 *
 * @param master_fd File descriptor to configure
 * @return true on success, false on failure
 */
bool configure_master_fd(int master_fd) {
#ifdef UNITY_TEST_MODE
    if (test_mode_force_fcntl_failure) {
        errno = EBADF; // Simulate failure
        log_this(SR_TERMINAL, "Failed to set master FD non-blocking", LOG_LEVEL_ERROR, 0);
        return false;
    }
#endif

    if (fcntl(master_fd, F_SETFL, O_NONBLOCK) == -1) {
        log_this(SR_TERMINAL, "Failed to set master FD non-blocking", LOG_LEVEL_ERROR, 0);
        return false;
    }
    return true;
}

/**
 * Setup child process with PTY and execute shell
 *
 * @param shell_command The shell command to execute
 * @param slave_fd The slave PTY file descriptor
 * @param master_fd The master PTY file descriptor to close in child
 */
void setup_child_process(const char *shell_command, int slave_fd, int master_fd) {
    // Child process
    close(master_fd); // Close master in child

    // Become session leader
    setsid();

    // Set controlling terminal
    if (ioctl(slave_fd, TIOCSCTTY, NULL) == -1) {
        log_this(SR_TERMINAL, "Failed to set controlling terminal", LOG_LEVEL_ERROR, 0);
        exit(1);
    }

    // Redirect stdin, stdout, stderr to slave PTY
    dup2(slave_fd, STDIN_FILENO);
    dup2(slave_fd, STDOUT_FILENO);
    dup2(slave_fd, STDERR_FILENO);

    // Close the duplicate slave file descriptor
    if (slave_fd > STDERR_FILENO) {
        close(slave_fd);
    }

    // Set up environment
    setenv("TERM", "xterm-256color", 1);
    setenv("COLORTERM", "truecolor", 1);

    // Try the configured shell first
    if (access(shell_command, X_OK) == 0) {
        // Shell exists and is executable
        setenv("SHELL", shell_command, 1);
        
        // Execute shell as interactive login shell
        // For login shell, argv[0] should start with '-'
        const char *shell_name = strrchr(shell_command, '/');
        shell_name = shell_name ? shell_name + 1 : shell_command;
        
        char login_arg[256];
        snprintf(login_arg, sizeof(login_arg), "-%s", shell_name);
        
        char *const args[] = {
            login_arg,  // argv[0] with leading dash for login shell
            NULL
        };

        execv(shell_command, args);
        // If we get here, exec failed - fall through to fallback
    }
    
    // Fallback to bash if configured shell doesn't exist or exec failed
    const char *fallback_shell = "/bin/bash";
    if (access(fallback_shell, X_OK) == 0) {
        setenv("SHELL", fallback_shell, 1);
        
        char fallback_login_arg[256];
        snprintf(fallback_login_arg, sizeof(fallback_login_arg), "-bash");
        
        char *const fallback_args[] = {
            fallback_login_arg,  // Login bash
            NULL
        };
        
        execv(fallback_shell, fallback_args);
    }

    // If both failed
    perror("execv failed for both configured and fallback shells");
    _exit(1);
}

/**
 * Clean up PTY resources on error during spawn
 *
 * @param master_fd Master file descriptor to close
 * @param slave_fd Slave file descriptor to close
 * @param slave_name Slave name string to free
 * @param shell Shell structure to free
 */
void cleanup_pty_resources(int master_fd, int slave_fd, char *slave_name, PtyShell *shell) {
    if (master_fd >= 0) {
        close(master_fd);
    }
    if (slave_fd >= 0) {
        close(slave_fd);
    }
    if (slave_name) {
        free(slave_name);
    }
    if (shell) {
        free(shell);
    }
}

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
PtyShell *pty_spawn_shell(const char *shell_command, TerminalSession *session) {
    if (!shell_command || !session) {
        log_this(SR_TERMINAL, "Invalid parameters for pty_spawn_shell", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    log_this(SR_TERMINAL, "Attempting to spawn shell: %s", LOG_LEVEL_STATE, 1, shell_command);

    PtyShell *shell = calloc(1, sizeof(PtyShell));
#ifdef UNITY_TEST_MODE
    if (test_mode_force_calloc_failure) {
        log_this(SR_TERMINAL, "Failed to allocate PtyShell structure", LOG_LEVEL_ERROR, 0);
        return NULL;
    }
#endif
    if (!shell) {
        log_this(SR_TERMINAL, "Failed to allocate PtyShell structure", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    shell->session = session;
    shell->running = false;

    // Open a new pseudo-terminal pair
    int master_fd;
    char slave_name[256];

    if (!create_pty_pair(&master_fd, &shell->slave_fd, slave_name)) {
        cleanup_pty_resources(-1, -1, NULL, shell);
        return NULL;
    }

    // Store PTY information
    shell->master_fd = master_fd;
    shell->slave_name = strdup(slave_name);
#ifdef UNITY_TEST_MODE
    if (test_mode_force_strdup_failure) {
        free(shell->slave_name); // Free what was allocated
        shell->slave_name = NULL;
    }
#endif
    if (!shell->slave_name) {
        log_this(SR_TERMINAL, "Failed to allocate slave name", LOG_LEVEL_ERROR, 0);
        cleanup_pty_resources(master_fd, shell->slave_fd, NULL, shell);
        return NULL;
    }

    // Make master FD non-blocking
    if (!configure_master_fd(master_fd)) {
        cleanup_pty_resources(master_fd, shell->slave_fd, shell->slave_name, shell);
        return NULL;
    }

    // Fork child process
    pid_t pid = fork();
#ifdef UNITY_TEST_MODE
    if (test_mode_force_fork_failure) {
        pid = -1;
        errno = EAGAIN; // Simulate fork failure
    }
#endif
    if (pid == -1) {
        log_this(SR_TERMINAL, "Fork failed: %s", LOG_LEVEL_ERROR, 1, strerror(errno));
        cleanup_pty_resources(master_fd, shell->slave_fd, shell->slave_name, shell);
        return NULL;
    }

    if (pid == 0) {
        setup_child_process(shell_command, shell->slave_fd, master_fd);
        // Never returns
    } else {
        // Parent process
        shell->pid = pid;
        close(shell->slave_fd); // Close slave in parent

        log_this(SR_TERMINAL, "Shell spawned successfully - PID: %d, PTY: %s", LOG_LEVEL_STATE, 2, pid, slave_name);

        // Wait briefly to ensure shell starts successfully
        // Increased from 10ms to 100ms to handle race condition where
        // fast-exiting processes (like /bin/false) may not have terminated
        // and had their status updated within the original 10ms window
        usleep(100000); // 100ms

        // Check if process is still running
        int status;
        pid_t result = waitpid(pid, &status, WNOHANG);
        if (result == pid) {
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            log_this(SR_TERMINAL, "Shell process terminated prematurely", LOG_LEVEL_ERROR, 0);
                cleanup_pty_resources(master_fd, -1, shell->slave_name, shell);
                return NULL;
            }
        }

        shell->running = true;
    }

    return shell;
}

/**
 * Send data to the PTY master (towards the shell)
 *
 * @param shell Pointer to PtyShell structure
 * @param data Buffer containing data to send
 * @param size Size of data buffer
 * @return Number of bytes written, or -1 on error
 */
int pty_write_data(PtyShell *shell, const char *data, size_t size) {
    if (!shell || !shell->running || !data || size == 0) {
        return -1;
    }

    ssize_t written = write(shell->master_fd, data, size);
    if (written == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            log_this(SR_TERMINAL, "Failed to write to PTY: %s", LOG_LEVEL_ERROR, 1, strerror(errno));
        }
        return -1;
    }

    return (int)written;
}

/**
 * Read data from the PTY master (from the shell)
 *
 * @param shell Pointer to PtyShell structure
 * @param buffer Buffer to store read data
 * @param size Size of buffer
 * @return Number of bytes read, 0 on EWOULDBLOCK, or -1 on error
 */
int pty_read_data(PtyShell *shell, char *buffer, size_t size) {
    if (!shell || !shell->running || !buffer || size == 0) {
        return -1;
    }

    ssize_t read_bytes = read(shell->master_fd, buffer, size);
    if (read_bytes == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0; // No data available
        } else {
            log_this(SR_TERMINAL, "Failed to read from PTY: %s", LOG_LEVEL_ERROR, 1, strerror(errno));
            return -1;
        }
    }

    return (int)read_bytes;
}

/**
 * Set terminal window size for PTY
 *
 * @param shell Pointer to PtyShell structure
 * @param rows Number of rows
 * @param cols Number of columns
 * @return true on success, false on failure
 */
bool pty_set_size(PtyShell *shell, unsigned short rows, unsigned short cols) {
    if (!shell || !shell->running) {
        return false;
    }

    struct winsize ws = {
        .ws_row = rows,
        .ws_col = cols,
        .ws_xpixel = 0,
        .ws_ypixel = 0
    };

    if (ioctl(shell->master_fd, TIOCSWINSZ, &ws) == -1) {
        log_this(SR_TERMINAL, "Failed to set terminal size: %s", LOG_LEVEL_ERROR, 1, strerror(errno));
        return false;
    }

    return true;
}

/**
 * Check if shell process is still running
 *
 * @param shell Pointer to PtyShell structure
 * @return true if running, false if terminated
 */
bool pty_is_running(PtyShell *shell) {
    if (!shell) {
        return false;
    }

    if (!shell->running) {
        return false;
    }

    // Check if process exists
    int status;
    pid_t result = waitpid(shell->pid, &status, WNOHANG);

    if (result == shell->pid) {
        // Process terminated
        shell->running = false;
        return false;
    } else if (result == -1) {
        // Error (likely process doesn't exist)
        if (errno == ECHILD) {
            shell->running = false;
            return false;
        }
    }

    return shell->running;
}

/**
 * Terminate the shell process gracefully
 *
 * @param shell Pointer to PtyShell structure
 * @return true on success, false on failure
 */
bool pty_terminate_shell(PtyShell *shell) {
    if (!shell || !shell->running) {
        return false;
    }

    // Send SIGTERM first
    if (kill(shell->pid, SIGTERM) == -1) {
        log_this(SR_TERMINAL, "Failed to send SIGTERM to process %d: %s", LOG_LEVEL_ERROR, 2, shell->pid, strerror(errno));
        return false;
    }

    // Process termination will be handled by the system

    shell->running = false;
    log_this(SR_TERMINAL, "Shell process terminated successfully", LOG_LEVEL_STATE, 0);

    return true;
}

/**
 * Clean up PTY shell resources
 *
 * @param shell Pointer to PtyShell structure (will be freed)
 */
void pty_cleanup_shell(PtyShell *shell) {
    if (!shell) {
        return;
    }

    log_this(SR_TERMINAL, "Cleaning up PTY shell resources", LOG_LEVEL_STATE, 0);

    // Terminate process if still running
    if (shell->running) {
        pty_terminate_shell(shell);
    }

    // Close file descriptors
    if (shell->master_fd >= 0) {
        close(shell->master_fd);
    }

    // Free allocated memory
    if (shell->slave_name) {
        free(shell->slave_name);
    }

    // Don't free session here - it's managed elsewhere

    free(shell);
}