/*
 * Thread-safe logging system interface for the Hydrogen server.
 * 
 * Provides a flexible logging facility that can output messages to multiple
 * destinations (console, database, file) with priority levels. Messages are
 * queued for asynchronous processing to avoid I/O bottlenecks.
 */

#ifndef LOGGING_H
#define LOGGING_H

#include <stdbool.h>

#define LOG_LINE_BREAK "----------------------------------"

void log_this(const char* subsystem, const char* format, int priority, bool LogConsole, bool LogDatabase, bool LogFile, ...);

#endif // LOGGING_H
