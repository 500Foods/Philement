/*
 * Terminal PTY Shell Management
 *
 * This module handles PTY (pseudo-terminal) process management for terminal sessions.
 * It provides functions to spawn shell processes, manage their lifecycle, track
 * child processes, and handle signal propagation.
 */

#include "terminal_shell.h"
#include <src/hydrogen.h>
#include "../globals.h"
#include <src/logging/logging.h>
#include <src/utils/utils.h>
#include "terminal.h"
#include "terminal_session.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pty.h>
#include <utmp.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>

// PtyShell structure is defined in header file for WebSocket access to master_fd

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
    if (!shell) {
        log_this(SR_TERMINAL, "Failed to allocate PtyShell structure", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    shell->session = session;
    shell->running = false;

    // Open a new pseudo-terminal pair
    int master_fd;
    char slave_name[256];

    if (openpty(&master_fd, &shell->slave_fd, slave_name, NULL, NULL) == -1) {
        log_this(SR_TERMINAL, "Failed to create PTY pair: %s", LOG_LEVEL_ERROR, 1, strerror(errno));
        free(shell);
        return NULL;
    }

    // Store PTY information
    shell->master_fd = master_fd;
    shell->slave_name = strdup(slave_name);
    if (!shell->slave_name) {
        log_this(SR_TERMINAL, "Failed to allocate slave name", LOG_LEVEL_ERROR, 0);
        close(master_fd);
        close(shell->slave_fd);
        free(shell);
        return NULL;
    }

    // Make master FD non-blocking
    if (fcntl(master_fd, F_SETFL, O_NONBLOCK) == -1) {
        log_this(SR_TERMINAL, "Failed to set master FD non-blocking", LOG_LEVEL_ERROR, 0);
        close(master_fd);
        close(shell->slave_fd);
        free(shell->slave_name);
        free(shell);
        return NULL;
    }

    // Fork child process
    pid_t pid = fork();
    if (pid == -1) {
        log_this(SR_TERMINAL, "Fork failed: %s", LOG_LEVEL_ERROR, 1, strerror(errno));
        close(master_fd);
        close(shell->slave_fd);
        free(shell->slave_name);
        free(shell);
        return NULL;
    }

    if (pid == 0) {
        // Child process
        close(master_fd); // Close master in child

        // Become session leader
        setsid();

        // Set controlling terminal
        if (ioctl(shell->slave_fd, TIOCSCTTY, NULL) == -1) {
            log_this(SR_TERMINAL, "Failed to set controlling terminal", LOG_LEVEL_ERROR, 0);
            exit(1);
        }

        // Redirect stdin, stdout, stderr to slave PTY
        dup2(shell->slave_fd, STDIN_FILENO);
        dup2(shell->slave_fd, STDOUT_FILENO);
        dup2(shell->slave_fd, STDERR_FILENO);

        // Close the duplicate slave file descriptor
        if (shell->slave_fd > STDERR_FILENO) {
            close(shell->slave_fd);
        }

        // Set up environment
        setenv("TERM", "xterm-256color", 1);
        setenv("COLORTERM", "truecolor", 1);

        // Execute shell
        char *const args[] = {
            (char *)shell_command,
            NULL
        };

        execv(shell_command, args);

        // If exec fails
        perror("execv failed");
        _exit(1);
    } else {
        // Parent process
        shell->pid = pid;
        close(shell->slave_fd); // Close slave in parent

        log_this(SR_TERMINAL, "Shell spawned successfully - PID: %d, PTY: %s", LOG_LEVEL_STATE, 2, pid, slave_name);

        // Wait briefly to ensure shell starts successfully
        usleep(10000); // 10ms

        // Check if process is still running
        int status;
        pid_t result = waitpid(pid, &status, WNOHANG);
        if (result == pid) {
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            log_this(SR_TERMINAL, "Shell process terminated prematurely", LOG_LEVEL_ERROR, 0);
                close(master_fd);
                free(shell->slave_name);
                free(shell);
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
