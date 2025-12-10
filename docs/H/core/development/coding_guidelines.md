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

Severity levels for logging in code:

- 1: INFO - General operational information, startup messages, and normal operations
- 2: WARN - Warning conditions that don't prevent operation but require attention
- 3: DEBUG - Detailed debug-level information for troubleshooting
- 4: ERROR - Error conditions that affect functionality but don't require shutdown
- 5: CRITICAL - Critical conditions requiring immediate attention or shutdown

IMPORTANT: The values 0 (ALL) and 6 (NONE) are special values used only for log filtering in configuration files. They must never be used in log_this() calls. Many existing calls using level 0 should be changed to level 1 (INFO).

Default log filtering by output:

- Console: Level 1 (INFO) and above
- Database: Level 4 (ERROR) and above
- File: Level 1 (INFO) and above

Example of correct usage:

```c
// Correct: Startup/operational info uses INFO level
log_this("WebServer", "Server started on port 5000", 1, true, true, true);

// Correct: Warning about resource usage uses WARN level
log_this("PrintQueue", "Print job queue approaching capacity", 2, true, true, true);

// Correct: Detailed debugging info uses DEBUG level
log_this("WebSocket", "Processing message type: %s", 3, true, true, true, type);

// Correct: Error condition uses ERROR level
log_this("Network", "Failed to bind socket: %s", 4, true, true, true, strerror(errno));

// Correct: Critical failure uses CRITICAL level
log_this("Initialization", "Failed to load configuration", 5, true, true, true);

// INCORRECT: Never use level 0 in code
// log_this("WebServer", "Server initialized", 0, true, true, true);  // Wrong!
```

Example:

```c
// Correct usage for operational info
log_this("WebServer", "Server started on port 5000", 1, true, true, false);

// Correct usage for warning condition
log_this("PrintQueue", "Print job queue approaching capacity", 2, true, true, true);

// Correct usage for error condition
log_this("WebSocket", "Failed to establish connection", 4, true, true, false);
```

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

## Data Formatting Standards

### JSON Output Formatting

#### Percentage Values

- Format all percentage values as strings with exactly 3 decimal places
- This applies to all metrics that represent percentages (CPU usage, memory usage, etc.)
- Use consistent formatting across all JSON responses for API consistency

Example:

```c
// Correct percentage formatting
char percent_str[16];
snprintf(percent_str, sizeof(percent_str), "%.3f", percentage_value);
json_object_set_new(obj, "usage_percent", json_string(percent_str));
```

Why This Matters:

- Ensures consistent precision across all percentage metrics
- Maintains API compatibility and predictability
- Allows accurate tracking of small changes in resource usage
- Prevents floating-point representation issues in JSON

#### Numeric Values

- Use integers for byte counts and absolute values
- Use strings for formatted durations and timestamps
- Document any special formatting requirements in the relevant function

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

```files
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

- [API Documentation](/docs/H/core/reference/api.md)
- [Configuration Guide](/docs/H/core/reference/configuration.md)
- [Service Documentation](/docs/H/core/reference/service.md)
