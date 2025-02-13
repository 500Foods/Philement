#ifndef HYDROGEN_SHUTDOWN_H
#define HYDROGEN_SHUTDOWN_H

#include <signal.h>

// Signal handler for Ctrl+C
void inthandler(int signum);

// Perform graceful shutdown of all components
void graceful_shutdown(void);

#endif // HYDROGEN_SHUTDOWN_H