/*
 * Terminal PTY Shell Operations
 *
 * This module handles PTY I/O operations and process management.
 * It provides functions for reading, writing, and managing shell processes.
 */

#include "terminal_shell.h"
#include <src/hydrogen.h>
#include <src/globals.h>
#include <src/logging/logging.h>
#include <src/utils/utils.h>
#include "terminal.h"
#include "terminal_session.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <signal.h>

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
