/*
 * Terminal PTY Shell Spawning
 *
 * This module handles PTY (pseudo-terminal) creation and shell process spawning.
 * It provides functions to create PTY pairs and spawn shell processes.
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

    // Execute shell
    char *const args[] = {
        (char *)shell_command,
        NULL
    };

    execv(shell_command, args);

    // If exec fails
    perror("execv failed");
    _exit(1);
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
    if (!configure_master_fd(master_fd)) {
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
        setup_child_process(shell_command, shell->slave_fd, master_fd);
        // Never returns
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