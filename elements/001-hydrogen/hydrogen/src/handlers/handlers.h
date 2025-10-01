/**
 * @file handlers.h
 */

#ifndef HANDLERS_H
#define HANDLERS_H

#include <signal.h>

/**
 * @brief Handles critical program crashes by generating a detailed core dump
 * @note This handler is async-signal-safe and uses only async-signal-safe functions
 * @note The generated core file can be analyzed with: gdb -q executable corefile
 */
void crash_handler(int sig, siginfo_t *info, void *ucontext);

/**
 * @brief Test function to simulate a crash for testing the crash handler
 * @param sig Signal number (unused, but required for signal handler signature)
 * @note This handler is only for testing and should not be enabled in production
 */
void test_crash_handler(int sig);

/**
 * @brief Signal handler to dump current configuration
 * @param sig Signal number (unused, but required for signal handler signature)
 */
void config_dump_handler(int sig);

#endif /* HANDLERS_H */