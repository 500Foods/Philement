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

## Documentation Standards

### Directory Structure

Documentation is organized into three main categories:
```
docs/
├── reference/          # Technical reference documentation
│   ├── api.md         # API specifications
│   └── ...            # Other technical references
├── guides/            # User-focused documentation
│   ├── use-cases/     # Real-world usage examples
│   └── ...            # How-to guides
└── development/       # Developer documentation
    └── ...            # Development standards
```

### Documentation Types

1. **Reference Documentation**
   - Technical specifications
   - API endpoints
   - Data structures
   - Configuration options
   - Must be complete and precise

2. **User Guides**
   - Task-oriented
   - Step-by-step instructions
   - Real-world examples
   - Focus on common use cases

3. **Development Guides**
   - Coding standards
   - Build instructions
   - Architecture decisions
   - Implementation details

### Markdown Standards

1. **File Names**
   - Use snake_case for all documentation files (e.g., `configuration.md`)
   - Group related files in directories
   - Maintain consistent naming across the project

2. **Content Structure**
   - Start with a clear title and introduction
   - Use hierarchical headings (H1 → H2 → H3)
   - Include a table of contents for longer documents
   - End with related documentation links

3. **Code Examples**
   - Use appropriate language tags for syntax highlighting
   - Include comments in examples
   - Show both basic and advanced usage
   - Provide complete, working examples

4. **Links and References**
   - Use relative paths for internal links
   - Check links when moving documents
   - Include section anchors for precise references
   - Document external dependencies

### Writing Style

1. **Technical Writing**
   - Be clear and concise
   - Use active voice
   - Define technical terms
   - Include "Why" explanations for decisions

2. **Examples and Use Cases**
   - Start with simple examples
   - Build up to complex scenarios
   - Include real-world use cases
   - Show common error scenarios

3. **Updates and Maintenance**
   - Keep documentation in sync with code
   - Mark outdated sections clearly
   - Include "Last Updated" dates
   - Review documentation regularly

### Documentation Process

1. **New Features**
   - Document before implementing
   - Include API specifications
   - Write user guides
   - Add to relevant examples

2. **Changes and Updates**
   - Update docs with code changes
   - Mark breaking changes clearly
   - Update all affected documents
   - Review cross-references

3. **Review Process**
   - Technical accuracy review
   - User comprehension review
   - Link verification
   - Example testing

## Related Documentation

- [API Documentation](../reference/api.md)
- [Configuration Guide](../reference/configuration.md)
- [Service Documentation](../reference/service.md)