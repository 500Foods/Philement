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

// Function prototypes
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