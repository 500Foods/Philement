# C Coding Guidelines for Hydrogen

This document outlines the C programming guidelines and patterns used in the Hydrogen project. It serves as a reference for developers who may not work with C on a daily basis.

## Project Structure

### Header Files
- Each `.c` file should have a corresponding `.h` file (except for main program files)
- Use header guards in all header files
- Keep interface declarations in headers, implementations in source files

Example header guard:
```c
#ifndef HYDROGEN_COMPONENT_H
#define HYDROGEN_COMPONENT_H

// Declarations here

#endif // HYDROGEN_COMPONENT_H
```

## Required Feature Test Macros

At the start of source files that use POSIX features:
```c
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
```

These must come before any includes.

## Standard Library Usage

### Common Standard Headers
```c
#include <signal.h>    // Signal handling
#include <pthread.h>   // POSIX threading
#include <errno.h>     // Error codes
#include <time.h>      // Time functions
#include <stdbool.h>   // Boolean type
#include <stdint.h>    // Fixed-width integers
#include <string.h>    // String manipulation
#include <stdlib.h>    // General utilities
```

### Project-Specific Headers
```c
#include "logging.h"   // Logging functions
#include "state.h"     // State management
#include "config.h"    // Configuration
```

## Error Handling

1. Use return codes for function status
2. Set errno when appropriate
3. Log errors with context
4. Clean up resources on error paths

Example:
```c
if (operation_failed) {
    log_this("Component", "Operation failed: specific reason", 3, true, false, true);
    cleanup_resources();
    return false;
}
```

## Threading Practices

1. Use POSIX threads (pthread)
2. Protect shared resources with mutexes
3. Use condition variables for signaling
4. Follow consistent locking order to prevent deadlocks

Example:
```c
pthread_mutex_lock(&resource_mutex);
// Access shared resource
pthread_mutex_unlock(&resource_mutex);
```

## Logging Conventions

Use the project's logging system with appropriate severity levels:
```c
log_this("Component", "Message", severity, to_console, to_file, to_websocket);
```

Severity levels:
- 1: Debug
- 2: Info
- 3: Warning
- 4: Error
- 5: Critical

## Memory Management

1. Always check malloc/calloc return values
2. Free resources in reverse order of allocation
3. Use valgrind for memory leak detection
4. Consider using static allocation for fixed-size resources

Example:
```c
void* ptr = malloc(size);
if (!ptr) {
    log_this("Component", "Memory allocation failed", 4, true, false, true);
    return false;
}
```

## Code Documentation

1. Each file should start with a comment block explaining its purpose
2. Document architectural decisions with "Why" explanations
3. Use clear function comments explaining:
   - Purpose
   - Parameters
   - Return values
   - Error conditions

Example:
```c
/*
 * Why This Architecture Matters:
 * 1. Safety-Critical Design
 *    - Controlled startup sequence
 *    - Graceful shutdown handling
 * 
 * 2. Real-time Requirements
 *    - Immediate command response
 *    - Continuous monitoring
 */
```

## Build System

The project uses Make for building. Key targets:
- `make` - Build the project
- `make debug` - Build with debug symbols
- `make clean` - Clean build artifacts
- `make valgrind` - Run with memory checking

## Related Documentation

- [API Documentation](./api.md)
- [Configuration Guide](./configuration.md)
- [Service Documentation](./service.md)