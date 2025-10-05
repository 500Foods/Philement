/*
 * Mock system functions for unit testing
 *
 * This file provides mock implementations of system functions
 * like malloc, gethostname, etc. to enable testing of error conditions.
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <poll.h>
#include <sys/socket.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <pty.h>
#include <stdarg.h>
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>

// Define types if not already defined
#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned long size_t;
#endif

#ifndef _SSIZE_T
#define _SSIZE_T
typedef long ssize_t;
#endif

// Include the header but undefine the macros to access real functions
#include "mock_system.h"

// Undefine the macros in this file so we can call the real functions
#undef malloc
#undef realloc
#undef free
#undef strdup
#undef gethostname
#undef nanosleep
#undef clock_gettime
#undef poll
#undef recvfrom
#undef dlopen
#undef dlclose
#undef dlerror
#undef access
#undef openpty
#undef fcntl
#undef fork
#undef ioctl
#undef read
#undef write
#undef waitpid
#undef kill
#undef close

// Function prototypes - these are defined in the header when USE_MOCK_SYSTEM is set
void *mock_malloc(size_t size);
void *mock_realloc(void *ptr, size_t size);
void mock_free(void *ptr);
char *mock_strdup(const char *s);
int mock_gethostname(char *name, size_t len);
int mock_nanosleep(const struct timespec *req, struct timespec *rem);
int mock_clock_gettime(int clk_id, struct timespec *tp);
int mock_poll(struct pollfd *fds, nfds_t nfds, int timeout);
ssize_t mock_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
void *mock_dlopen(const char *filename, int flags);
int mock_dlclose(void *handle);
char *mock_dlerror(void);
int mock_access(const char *pathname, int mode);
int mock_openpty(int *amaster, int *aslave, char *name, const struct termios *termp, const struct winsize *winp);
int mock_fcntl(int fd, int cmd, ...);
pid_t mock_fork(void);
int mock_ioctl(int fd, unsigned long request, ...);
ssize_t mock_read(int fd, void *buf, size_t count);
ssize_t mock_write(int fd, const void *buf, size_t count);
pid_t mock_waitpid(pid_t pid, int *wstatus, int options);
int mock_kill(pid_t pid, int sig);
int mock_close(int fd);
void mock_system_set_malloc_failure(int should_fail);
void mock_system_set_realloc_failure(int should_fail);
void mock_system_set_gethostname_failure(int should_fail);
void mock_system_set_gethostname_result(const char *result);
void mock_system_set_nanosleep_failure(int should_fail);
void mock_system_set_clock_gettime_failure(int should_fail);
void mock_system_set_poll_failure(int should_fail);
void mock_system_set_recvfrom_failure(int should_fail);
void mock_system_set_dlopen_result(void *result);
void mock_system_set_dlopen_failure(int should_fail);
void mock_system_set_dlerror_result(const char *result);
void mock_system_set_access_result(int result);
void mock_system_set_openpty_failure(int should_fail);
void mock_system_set_fcntl_failure(int should_fail);
void mock_system_set_fork_result(pid_t result);
void mock_system_set_ioctl_failure(int should_fail);
void mock_system_set_read_result(ssize_t result);
void mock_system_set_read_should_fail(int should_fail);
void mock_system_set_write_result(ssize_t result);
void mock_system_set_write_should_fail(int should_fail);
void mock_system_set_waitpid_result(pid_t result);
void mock_system_set_waitpid_status(int status);
void mock_system_set_kill_failure(int should_fail);
void mock_system_set_close_failure(int should_fail);
void mock_system_reset_all(void);

// Static variables to store mock state
static int mock_malloc_should_fail = 0;
static int mock_realloc_should_fail = 0;
static int mock_gethostname_should_fail = 0;
static const char *mock_gethostname_result = NULL;
static int mock_nanosleep_should_fail = 0;
static int mock_clock_gettime_should_fail = 0;
static int mock_poll_should_fail = 0;
static int mock_recvfrom_should_fail = 0;
static void *mock_dlopen_result = NULL;
static int mock_dlopen_should_fail = 0;
static const char *mock_dlerror_result = NULL;
static int mock_access_result = 0;
static int mock_openpty_should_fail = 0;
static int mock_fcntl_should_fail = 0;
static pid_t mock_fork_result = 0;
static int mock_ioctl_should_fail = 0;
static ssize_t mock_read_result = 0;
static int mock_read_should_fail = 0;
static ssize_t mock_write_result = 0;
static int mock_write_should_fail = 0;
static pid_t mock_waitpid_result = 0;
static int mock_waitpid_status = 0;
static int mock_kill_should_fail = 0;
static int mock_close_should_fail = 0;

// Mock implementation of malloc
void *mock_malloc(size_t size) {
    if (mock_malloc_should_fail) {
        return NULL;
    }
    // Now we can call the real malloc since we undefined the macro
    return malloc(size);
}

// Mock implementation of realloc
void *mock_realloc(void *ptr, size_t size) {
    if (mock_realloc_should_fail) {
        return NULL;
    }
    // Now we can call the real realloc since we undefined the macro
    return realloc(ptr, size);
}

// Mock implementation of free
void mock_free(void *ptr) {
    // Call the real free function
    free(ptr);
}

// Mock implementation of strdup
char *mock_strdup(const char *s) {
    if (mock_malloc_should_fail) {
        return NULL;
    }
    // Call the real strdup function
    return strdup(s);
}

// Mock implementation of gethostname
int mock_gethostname(char *name, size_t len) {
    if (mock_gethostname_should_fail) {
        return -1;
    }

    if (mock_gethostname_result) {
        size_t result_len = strlen(mock_gethostname_result);
        if (result_len >= len) {
            return -1; // Buffer too small
        }
        strcpy(name, mock_gethostname_result);
        return 0;
    }

    // Default behavior - return "testhost"
    if (len > 9) {
        strcpy(name, "testhost");
        return 0;
    }
    return -1;
}

// Mock implementation of nanosleep
int mock_nanosleep(const struct timespec *req, struct timespec *rem) {
    (void)req;  // Suppress unused parameter
    (void)rem;  // Suppress unused parameter

    if (mock_nanosleep_should_fail) {
        return -1;
    }

    // Call the real nanosleep function
    return nanosleep(req, rem);
}

// Mock implementation of clock_gettime
int mock_clock_gettime(int clk_id, struct timespec *tp) {
    (void)clk_id;  // Suppress unused parameter

    if (mock_clock_gettime_should_fail) {
        return -1;
    }

    // Call the real clock_gettime function
    return clock_gettime(clk_id, tp);
}

// Mock implementation of poll
int mock_poll(struct pollfd *fds, nfds_t nfds, int timeout) {
    (void)fds;    // Suppress unused parameter
    (void)nfds;   // Suppress unused parameter
    (void)timeout; // Suppress unused parameter

    if (mock_poll_should_fail) {
        return -1;
    }

    // Default behavior - return 0 (timeout)
    return 0;
}

// Mock implementation of recvfrom
ssize_t mock_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    (void)sockfd;  // Suppress unused parameter
    (void)buf;     // Suppress unused parameter
    (void)len;     // Suppress unused parameter
    (void)flags;   // Suppress unused parameter
    (void)src_addr; // Suppress unused parameter
    (void)addrlen;  // Suppress unused parameter

    if (mock_recvfrom_should_fail) {
        return -1;
    }

    // Default behavior - return 0 (no data)
    return 0;
}

// Mock control functions
void mock_system_set_malloc_failure(int should_fail) {
    mock_malloc_should_fail = should_fail;
}

void mock_system_set_realloc_failure(int should_fail) {
    mock_realloc_should_fail = should_fail;
}

void mock_system_set_gethostname_failure(int should_fail) {
    mock_gethostname_should_fail = should_fail;
}

void mock_system_set_gethostname_result(const char *result) {
    mock_gethostname_result = result;
}

void mock_system_set_nanosleep_failure(int should_fail) {
    mock_nanosleep_should_fail = should_fail;
}

void mock_system_set_clock_gettime_failure(int should_fail) {
    mock_clock_gettime_should_fail = should_fail;
}

void mock_system_set_poll_failure(int should_fail) {
    mock_poll_should_fail = should_fail;
}

void mock_system_set_recvfrom_failure(int should_fail) {
    mock_recvfrom_should_fail = should_fail;
}

void mock_system_set_dlopen_result(void *result) {
    mock_dlopen_result = result;
}

void mock_system_set_dlopen_failure(int should_fail) {
    mock_dlopen_should_fail = should_fail;
}

void mock_system_set_dlerror_result(const char *result) {
    mock_dlerror_result = result;
}

void mock_system_set_access_result(int result) {
    mock_access_result = result;
}

void mock_system_set_openpty_failure(int should_fail) {
    mock_openpty_should_fail = should_fail;
}

void mock_system_set_fcntl_failure(int should_fail) {
    mock_fcntl_should_fail = should_fail;
}

void mock_system_set_fork_result(pid_t result) {
    mock_fork_result = result;
}

void mock_system_set_ioctl_failure(int should_fail) {
    mock_ioctl_should_fail = should_fail;
}

void mock_system_set_read_result(ssize_t result) {
    mock_read_result = result;
}

void mock_system_set_read_should_fail(int should_fail) {
    mock_read_should_fail = should_fail;
}

void mock_system_set_write_result(ssize_t result) {
    mock_write_result = result;
}

void mock_system_set_write_should_fail(int should_fail) {
    mock_write_should_fail = should_fail;
}

void mock_system_set_waitpid_result(pid_t result) {
    mock_waitpid_result = result;
}

void mock_system_set_waitpid_status(int status) {
    mock_waitpid_status = status;
}

void mock_system_set_kill_failure(int should_fail) {
    mock_kill_should_fail = should_fail;
}

void mock_system_set_close_failure(int should_fail) {
    mock_close_should_fail = should_fail;
}

void mock_system_reset_all(void) {
    mock_malloc_should_fail = 0;
    mock_realloc_should_fail = 0;
    mock_gethostname_should_fail = 0;
    mock_gethostname_result = NULL;
    mock_nanosleep_should_fail = 0;
    mock_clock_gettime_should_fail = 0;
    mock_poll_should_fail = 0;
    mock_recvfrom_should_fail = 0;
    mock_dlopen_result = NULL;
    mock_dlopen_should_fail = 0;
    mock_dlerror_result = NULL;
    mock_access_result = 0;
    mock_openpty_should_fail = 0;
    mock_fcntl_should_fail = 0;
    mock_fork_result = 0;
    mock_ioctl_should_fail = 0;
    mock_read_result = 0;
    mock_read_should_fail = 0;
    mock_write_result = 0;
    mock_write_should_fail = 0;
    mock_waitpid_result = 0;
    mock_waitpid_status = 0;
    mock_kill_should_fail = 0;
    mock_close_should_fail = 0;
}

// Mock implementation of dlopen
void *mock_dlopen(const char *filename, int flags) {
    (void)filename;  // Suppress unused parameter
    (void)flags;     // Suppress unused parameter

    if (mock_dlopen_should_fail) {
        return NULL;
    }

    return mock_dlopen_result;
}

// Mock implementation of dlclose
int mock_dlclose(void *handle) {
    (void)handle;  // Suppress unused parameter

    if (mock_dlopen_should_fail) {
        return -1;
    }

    return 0;
}

// Mock implementation of dlerror
char *mock_dlerror(void) {
    if (mock_dlerror_result) {
        return strdup(mock_dlerror_result);
    }

    return strdup("Mock dlerror");
}

// Mock implementation of access
int mock_access(const char *pathname, int mode) {
    (void)pathname;  // Suppress unused parameter
    (void)mode;      // Suppress unused parameter

    return mock_access_result;
}

// Mock implementation of openpty
int mock_openpty(int *amaster, int *aslave, char *name, const struct termios *termp, const struct winsize *winp) {
    (void)termp;  // Suppress unused parameter
    (void)winp;   // Suppress unused parameter

    if (mock_openpty_should_fail) {
        return -1;
    }

    // Set mock file descriptors
    if (amaster) *amaster = 42;
    if (aslave) *aslave = 43;
    if (name) strcpy(name, "/dev/pts/5");

    return 0;
}

// Mock implementation of fcntl
int mock_fcntl(int fd, int cmd, ...) {
    (void)fd;   // Suppress unused parameter
    (void)cmd;  // Suppress unused parameter

    if (mock_fcntl_should_fail) {
        return -1;
    }

    return 0;
}

// Mock implementation of fork
pid_t mock_fork(void) {
    return mock_fork_result;
}

// Mock implementation of ioctl
int mock_ioctl(int fd, unsigned long request, ...) {
    (void)fd;      // Suppress unused parameter
    (void)request; // Suppress unused parameter

    if (mock_ioctl_should_fail) {
        return -1;
    }

    return 0;
}

// Mock implementation of read
ssize_t mock_read(int fd, void *buf, size_t count) {
    (void)fd;   // Suppress unused parameter
    (void)buf;  // Suppress unused parameter
    (void)count; // Suppress unused parameter

    if (mock_read_should_fail) {
        return -1;
    }

    return mock_read_result;
}

// Mock implementation of write
ssize_t mock_write(int fd, const void *buf, size_t count) {
    (void)fd;   // Suppress unused parameter
    (void)buf;  // Suppress unused parameter
    (void)count; // Suppress unused parameter

    if (mock_write_should_fail) {
        return -1;
    }

    return mock_write_result;
}

// Mock implementation of waitpid
pid_t mock_waitpid(pid_t pid, int *wstatus, int options) {
    (void)pid;    // Suppress unused parameter
    (void)options; // Suppress unused parameter

    if (wstatus) {
        *wstatus = mock_waitpid_status;
    }

    return mock_waitpid_result;
}

// Mock implementation of kill
int mock_kill(pid_t pid, int sig) {
    (void)pid; // Suppress unused parameter
    (void)sig; // Suppress unused parameter

    if (mock_kill_should_fail) {
        return -1;
    }

    return 0;
}

// Mock implementation of close
int mock_close(int fd) {
    (void)fd; // Suppress unused parameter

    if (mock_close_should_fail) {
        return -1;
    }

    return 0;
}