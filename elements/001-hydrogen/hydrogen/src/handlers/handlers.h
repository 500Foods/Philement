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

/* ----------------------------------------------------------------------------
 * The following helpers are NOT part of the stable public API. They are
 * exposed (non-static) solely so the Unity test framework can call them
 * directly. CoreMapping and LoadSegment are defined privately in handlers.c
 * and are forward-declared opaque here.
 * -------------------------------------------------------------------------- */
typedef struct CoreMapping CoreMapping;
typedef struct LoadSegment LoadSegment;

size_t handlers_read_proc_maps(CoreMapping *out, size_t max);
void handlers_dump_segment(FILE *mem, FILE *out, const LoadSegment *seg);
size_t handlers_read_auxv_data(unsigned char *buf, size_t max_size);

#endif /* HANDLERS_H */