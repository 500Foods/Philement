/**
 * @file handlers.h
 *
 * @brief Signal and crash handling functions for the Hydrogen Server
 *
 * This header declares the signal handler functions used for crash handling,
 * testing, and configuration dumping in the Hydrogen server.
 */

#ifndef HANDLERS_H
#define HANDLERS_H

#include <signal.h>

/**
 * @brief Handles critical program crashes by generating a detailed core dump
 *
 * This handler is triggered by fatal signals (SIGSEGV, SIGABRT, SIGFPE) and creates
 * a comprehensive core dump file that can be analyzed with GDB. The core dump includes:
 * - Process status (registers, signal info, etc.)
 * - Memory maps (stack and code segments)
 * - Process information
 *
 * @param sig Signal number that triggered the handler
 * @param info Detailed signal information
 * @param ucontext CPU context at time of crash
 *
 * @note This handler is async-signal-safe and uses only async-signal-safe functions
 * @note The generated core file can be analyzed with: gdb -q executable corefile
 */
void crash_handler(int sig, siginfo_t *info, void *ucontext);

/**
 * @brief Test function to simulate a crash for testing the crash handler
 *
 * This function is triggered by SIGUSR1 and deliberately causes a segmentation fault
 * by dereferencing a null pointer. This allows testing of the crash handler in a
 * controlled manner.
 *
 * @param sig Signal number (unused, but required for signal handler signature)
 * @note This handler is only for testing and should not be enabled in production
 */
void test_crash_handler(int sig);

/**
 * @brief Signal handler to dump current configuration
 *
 * This function is triggered by SIGUSR2 and dumps the current application
 * configuration to the log. It provides a way to inspect the running
 * configuration without restarting the server.
 *
 * @param sig Signal number (unused, but required for signal handler signature)
 */
void config_dump_handler(int sig);

#endif /* HANDLERS_H */