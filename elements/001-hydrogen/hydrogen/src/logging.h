#ifndef LOGGING_H
#define LOGGING_H

#include <stdbool.h>

#define LOG_LINE_BREAK "----------------------------------"

void log_this(const char* subsystem, const char* format, int priority, bool LogConsole, bool LogDatabase, bool LogFile, ...);

#endif // LOGGING_H
