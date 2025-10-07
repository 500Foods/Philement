/*
 * Mock system functions for unit testing
 *
 * This file provides mock implementations of system functions
 * like malloc, gethostname, etc. to enable testing of error conditions.
 */

#ifndef MOCK_SYSTEM_H
#define MOCK_SYSTEM_H

#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <pty.h>

// Mock function declarations - these will override the real ones when USE_MOCK_SYSTEM is defined
#ifdef USE_MOCK_SYSTEM

// Override system functions with our mocks
#define malloc mock_malloc
#define free mock_free
#define realloc mock_realloc
#define strdup mock_strdup
#define gethostname mock_gethostname
#define nanosleep mock_nanosleep
#define clock_gettime mock_clock_gettime
#define poll mock_poll
#define recvfrom mock_recvfrom
#define dlopen mock_dlopen
#define dlclose mock_dlclose
#define dlerror mock_dlerror
#define access mock_access
#define openpty mock_openpty
#define fcntl mock_fcntl
#define fork mock_fork
#define ioctl mock_ioctl
#define read mock_read
#define write mock_write
#define waitpid mock_waitpid
#define kill mock_kill
#define close mock_close
#define select mock_select
#define sem_init mock_sem_init

// Always declare mock function prototypes for the .c file
void *mock_malloc(size_t size);
void *mock_realloc(void *ptr, size_t size);
void mock_free(void *ptr);
char *mock_strdup(const char *s);
int mock_gethostname(char *name, size_t len);
int mock_nanosleep(const struct timespec *req, struct timespec *rem);
int mock_clock_gettime(clockid_t clk_id, struct timespec *tp);
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
int mock_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
int mock_sem_init(sem_t *sem, int pshared, unsigned int value);

// Mock control functions for tests - always available
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
void mock_system_set_select_result(int result);
void mock_system_set_sem_init_failure(int should_fail);
void mock_system_reset_all(void);

#endif // USE_MOCK_SYSTEM

#endif // MOCK_SYSTEM_H