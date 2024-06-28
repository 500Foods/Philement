#ifndef LOGGING_H
#define LOGGING_H

#include <stdbool.h>

void log_this(const char* subsystem, const char* details, int priority, bool LogConsole, bool LogDatabase, bool LogFile);

#endif // LOGGING_H
