# Subsystem Registry Architecture

The Subsystem Registry is the foundational subsystem of Hydrogen, being the first to be checked for launch readiness and the first to be registered. It provides the core infrastructure for managing all other subsystems throughout their lifecycle, from launch through operation to landing (shutdown).

## Overview

The Subsystem Registry is unique in being both a subsystem itself and the manager of all subsystems. As the first subsystem to be initialized, it provides these key capabilities:

1. **First Subsystem**: First to be checked for launch readiness and registered
2. **Centralized Registration**: Maintains a single source of truth for all subsystems
3. **State Tracking**: Monitors the state of each subsystem (inactive, starting, running, stopping, error)
4. **Dependency Management**: Enforces rules about subsystem dependencies
5. **Lifecycle Control**: Provides standard interfaces for starting and stopping subsystems
6. **Status Reporting**: Offers comprehensive system status information

This architecture enables Hydrogen to maintain a robust and fault-tolerant system by:

- Ensuring proper initialization sequence starting with the registry itself
- Managing subsystem dependencies without imposing hierarchy of importance
- Coordinating both launch and landing processes in the correct order

```diagram
┌─────────────────────────────────────────────────────────────┐
│                    Subsystem Registry                       │
│                                                             │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐      │
│  │  Subsystem  │    │  Subsystem  │    │  Subsystem  │      │
│  │   Info #1   │    │   Info #2   │    │   Info #3   │      │
│  └──────┬──────┘    └──────┬──────┘    └──────┬──────┘      │
│         │                  │                  │             │
│         └──────────────────┼──────────────────┘             │
│                            │                                │
│                            ▼                                │
│  ┌───────────────────────────────────────────────────────┐  │
│  │               Registry Operations                     │  │
│  │                                                       │  │
│  │  ┌─────────────┐   ┌─────────────┐   ┌─────────────┐  │  │
│  │  │  Register   │   │    Start    │   │    Stop     │  │  │
│  │  │  Subsystem  │   │  Subsystem  │   │  Subsystem  │  │  │
│  │  └─────────────┘   └─────────────┘   └─────────────┘  │  │
│  │                                                       │  │
│  │  ┌─────────────┐   ┌──────────────┐   ┌─────────────┐ │  │
│  │  │    Add      │   │    Check     │   │   Update    │ │  │
│  │  │ Dependency  │   │ Dependencies │   │    State    │ │  │
│  │  └─────────────┘   └──────────────┘   └─────────────┘ │  │
│  └───────────────────────────────────────────────────────┘  │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## Core Components

### SubsystemState Enumeration

```c
typedef enum {
    SUBSYSTEM_INACTIVE,  // Not started
    SUBSYSTEM_STARTING,  // In the process of starting
    SUBSYSTEM_RUNNING,   // Running normally
    SUBSYSTEM_STOPPING,  // In the process of stopping
    SUBSYSTEM_ERROR      // Error state
} SubsystemState;
```

This enumeration tracks the lifecycle state of each subsystem.

### SubsystemInfo Structure

```c
typedef struct {
    const char* name;              // Subsystem name
    SubsystemState state;          // Current state
    time_t state_changed;          // When the state last changed
    ServiceThreads* threads;       // Pointer to thread tracking structure
    pthread_t* main_thread;        // Pointer to main thread handle
    volatile sig_atomic_t* shutdown_flag; // Pointer to shutdown flag
    
    // Dependencies
    int dependency_count;
    const char* dependencies[MAX_DEPENDENCIES];
    
    // Callbacks
    bool (*init_function)(void);   // Init function pointer
    void (*shutdown_function)(void); // Shutdown function pointer
} SubsystemInfo;
```

This structure stores all information about a registered subsystem, including:

- Basic identification (name)
- Current state information
- Thread tracking for monitoring
- Dependency information
- Function pointers for lifecycle management

### SubsystemRegistry Structure

```c
typedef struct {
    SubsystemInfo subsystems[MAX_SUBSYSTEMS];
    int count;
    pthread_mutex_t mutex;         // For thread-safe access
} SubsystemRegistry;
```

The registry itself maintains an array of subsystems and provides thread-safe access through a mutex.

## Key Operations

### Subsystem Registration

When a subsystem is registered with the registry, it provides:

1. A unique name for identification
2. Pointers to thread tracking structures
3. Function pointers for initialization and shutdown
4. Pointers to flags for coordinated shutdown

Example registration:

```c
int id = register_subsystem(
    "WebServer",            // Subsystem name
    &webserver_threads,           // Thread tracking structure
    &webserver_thread,            // Main thread handle
    &web_server_shutdown,   // Shutdown flag
    init_webserver_subsystem, // Initialization function
    shutdown_web_server     // Shutdown function
);
```

The registration process:

1. Validates the provided information
2. Ensures the subsystem name is unique
3. Initializes the subsystem record with the provided information
4. Returns a unique identifier for the subsystem

### Dependency Management

Dependencies between subsystems are managed through the `add_subsystem_dependency` function:

```c
bool add_subsystem_dependency(int subsystem_id, const char* dependency_name);
```

For example:

```c
// WebSocket depends on WebServer
add_subsystem_dependency(websocket_id, "WebServer");
```

Dependencies create a directed graph of relationships that enforce:

1. Start order: Dependencies must be started before dependent subsystems
2. Stop protection: A subsystem cannot be stopped while other subsystems depend on it

### Subsystem Starting

Starting a subsystem is handled by the `start_subsystem` function:

```c
bool start_subsystem(int subsystem_id);
```

The start process:

1. Checks if the subsystem is already running
2. Verifies all dependencies are running
3. Updates the subsystem state to SUBSYSTEM_STARTING
4. Calls the subsystem's initialization function
5. Updates the state to SUBSYSTEM_RUNNING on success or SUBSYSTEM_ERROR on failure

Example:

```c
if (!start_subsystem(web_server_id)) {
    log_this("Startup", "Failed to start Web Server subsystem", LOG_LEVEL_ERROR);
    return false;
}
```

### Subsystem Stopping

Stopping a subsystem is handled by the `stop_subsystem` function:

```c
bool stop_subsystem(int subsystem_id);
```

The stop process:

1. Checks if the subsystem is already stopped
2. Verifies no other running subsystems depend on this one
3. Updates the subsystem state to SUBSYSTEM_STOPPING
4. Sets the subsystem's shutdown flag
5. Calls the subsystem's shutdown function
6. Waits for the main thread to exit (if applicable)
7. Updates the state to SUBSYSTEM_INACTIVE

Example:

```c
if (!stop_subsystem(web_server_id)) {
    log_this("Shutdown", "Failed to stop Web Server subsystem", LOG_LEVEL_ERROR);
    return false;
}
```

## Thread Safety

All registry operations are thread-safe, protected by a mutex to prevent concurrent modifications. This allows the registry to be safely accessed and modified from different threads during both normal operation and shutdown sequences.

```c
pthread_mutex_lock(&subsystem_registry.mutex);
// Perform registry operations
pthread_mutex_unlock(&subsystem_registry.mutex);
```

## Integration with System Startup and Shutdown

The subsystem registry integrates with Hydrogen's startup and shutdown processes through the subsystem_registry_integration.c module, which:

1. Registers all standard subsystems during early initialization
2. Establishes dependency relationships between subsystems
3. Updates the registry during startup as subsystems initialize
4. Updates the registry during shutdown as subsystems terminate

### Standard Subsystems Registration

During system initialization, all standard subsystems are registered in a specific order:

```c
void register_standard_subsystems(void) {
    // Initialize the registry
    init_subsystem_registry();
    
    // Register subsystems
    int logging_id = register_standard_subsystem("Logging", ...);
    int web_id = register_standard_subsystem("WebServer", ...);
    
    // Establish dependencies
    if (web_id >= 0) {
        add_subsystem_dependency(web_id, "Logging");
    }
    
    // ...additional subsystems and dependencies
}
```

### Startup Integration

During the startup sequence, the registry is updated to reflect the actual state of each subsystem:

```c
void update_subsystem_registry_on_startup(void) {
    // Update subsystem states based on runtime evidence
    update_service_thread_metrics(&logging_threads);
    update_subsystem_on_startup("Logging", logging_threads.thread_count > 0);
    
    // ...update other subsystems
}
```

### Shutdown Integration

During the shutdown sequence, the registry is used to track the shutdown progress:

```c
void update_subsystem_registry_on_shutdown(void) {
    // Mark subsystems as stopping or inactive
    update_service_thread_metrics(&print_threads);
    if (print_threads.thread_count > 0) {
        update_subsystem_on_shutdown("PrintQueue");
    } else {
        update_subsystem_after_shutdown("PrintQueue");
    }
    
    // ...update other subsystems
}
```

## Status Reporting

The registry provides comprehensive status reporting through:

1. Real-time status queries
2. Detailed logging of state changes
3. Formatted status reports

Example status output:

```log
SUBSYSTEM STATUS REPORT
--------------------------------------------------
Subsystem: Logging - State: Running - Time: 01:23:45
  Threads: 2 - Memory: 1024 bytes
Subsystem: WebServer - State: Running - Time: 01:23:40
  Dependencies: Logging
  Threads: 8 - Memory: 4096 bytes
Subsystem: WebSocket - State: Running - Time: 01:23:35
  Dependencies: Logging, WebServer
  Threads: 4 - Memory: 2048 bytes
--------------------------------------------------
Total subsystems: 9 - Running: 8
```

## Example Usage Patterns

### Registering a New Subsystem

```c
// Define subsystem-specific data
static ServiceThreads custom_threads = {0};
static pthread_t custom_thread;
static volatile sig_atomic_t custom_shutdown = 0;

// Define initialization and shutdown functions
bool init_custom_subsystem(void) {
    // Initialization logic
    return true;
}

void shutdown_custom_subsystem(void) {
    // Shutdown logic
}

// Register the subsystem
int custom_id = register_subsystem(
    "CustomService",
    &custom_threads,
    &custom_thread,
    &custom_shutdown,
    init_custom_subsystem,
    shutdown_custom_subsystem
);

// Add dependencies
add_subsystem_dependency(custom_id, "Logging");
add_subsystem_dependency(custom_id, "WebServer");
```

### Starting All Subsystems in Dependency Order

```c
// Helper function to start all subsystems in proper order
bool start_all_subsystems(void) {
    bool all_started = true;
    
    // Process all subsystems
    for (int i = 0; i < subsystem_registry.count; i++) {
        // Skip already running subsystems
        if (get_subsystem_state(i) == SUBSYSTEM_RUNNING) {
            continue;
        }
        
        // Try to start if dependencies are met
        if (check_subsystem_dependencies(i)) {
            all_started &= start_subsystem(i);
        }
    }
    
    return all_started;
}

// Call repeatedly until all subsystems are started
while (!start_all_subsystems()) {
    // Handle any subsystems that failed to start
    if (get_subsystem_state(i) == SUBSYSTEM_ERROR) {
        // Recovery logic
    }
}
```

### Implementing Graceful Shutdown

```c
// Helper function to stop subsystems in reverse dependency order
void stop_all_subsystems(void) {
    // First, mark all subsystems for shutdown
    for (int i = 0; i < subsystem_registry.count; i++) {
        SubsystemInfo* subsystem = &subsystem_registry.subsystems[i];
        if (subsystem->shutdown_flag) {
            *subsystem->shutdown_flag = 1;
        }
    }
    
    // Process subsystems in reverse registration order (approximate dependency order)
    for (int i = subsystem_registry.count - 1; i >= 0; i--) {
        stop_subsystem(i);
    }
}
```

## Best Practices

When working with the Subsystem Registry, follow these guidelines:

### 1. Clear Dependency Management

- Make all dependencies explicit through `add_subsystem_dependency`
- Keep dependency chains as short as practical
- Avoid circular dependencies
- Document the reasoning for each dependency

### 2. Proper Error Handling

- Check return values from registry operations
- Implement proper cleanup in initialization functions
- Handle the case where a dependency fails to start
- Log detailed error information

### 3. Consistent State Management

- Always update the registry when subsystem states change
- Use `update_subsystem_state` to ensure proper tracking
- Periodically verify registry state matches actual subsystem states
- Implement automated recovery for subsystems in error states

### 4. Thread Safety

- Never bypass the registry's mutex protection
- Keep critical sections short to prevent deadlocks
- Be aware of potential lock ordering issues with other mutexes
- Avoid holding the registry mutex while performing long operations

## Related Documentation

For more information about how the Subsystem Registry integrates with other components, refer to:

- [System Architecture](/docs/H/core/reference/system_architecture.md) - Overall system design
- [Thread Monitoring](/docs/H/core/thread_monitoring.md) - Thread management
- [Shutdown Architecture](/docs/H/core/shutdown_architecture.md) - Shutdown process
- [State Management](/docs/H/core/reference/state_management.md) - System state management

## Example Code: Custom Subsystem Implementation

Below is a complete example of implementing a custom subsystem that integrates with the Subsystem Registry:

```c
// subsystem_example.h
#ifndef SUBSYSTEM_EXAMPLE_H
#define SUBSYSTEM_EXAMPLE_H

// Initialize example subsystem
bool init_example_subsystem(void);

// Shutdown example subsystem
void shutdown_example_subsystem(void);

#endif /* SUBSYSTEM_EXAMPLE_H */
```

```c
// subsystem_example.c
#include "subsystem_example.h"
#include "../state/state.h"
#include "../logging/logging.h"
#include "../utils/utils_threads.h"

// Thread and state management
static ServiceThreads example_threads = {0};
static pthread_t example_thread;
static volatile sig_atomic_t example_shutdown = 0;

// Thread function
static void* example_thread_func(void* arg) {
    log_this("Example", "Example subsystem thread started", LOG_LEVEL_STATE);
    
    while (!example_shutdown) {
        // Subsystem work here
        sleep(1);
    }
    
    log_this("Example", "Example subsystem thread exiting", LOG_LEVEL_STATE);
    return NULL;
}

// Initialize example subsystem
bool init_example_subsystem(void) {
    extern volatile sig_atomic_t server_stopping;
    
    // Prevent initialization during shutdown
    if (server_stopping || example_shutdown) {
        log_this("Example", "Cannot initialize during shutdown", LOG_LEVEL_ERROR);
        return false;
    }
    
    log_this("Example", "Initializing example subsystem", LOG_LEVEL_STATE);
    
    // Create the worker thread
    if (pthread_create(&example_thread, NULL, example_thread_func, NULL) != 0) {
        log_this("Example", "Failed to create example thread", LOG_LEVEL_ERROR);
        return false;
    }
    
    // Register thread with tracking system
    register_thread(&example_threads, example_thread, "example_main");
    
    log_this("Example", "Example subsystem initialized successfully", LOG_LEVEL_STATE);
    return true;
}

// Shutdown example subsystem
void shutdown_example_subsystem(void) {
    log_this("Example", "Shutting down example subsystem", LOG_LEVEL_STATE);
    
    // Signal thread to stop
    example_shutdown = 1;
    
    // Wait for thread to exit
    if (example_thread != 0) {
        pthread_join(example_thread, NULL);
        example_thread = 0;
    }
    
    log_this("Example", "Example subsystem shutdown complete", LOG_LEVEL_STATE);
}

// To register with the subsystem registry:
// In subsystem_registry_integration.c:

// Declare the example subsystem functions
extern bool init_example_subsystem(void);
extern void shutdown_example_subsystem(void);

// In register_standard_subsystems():
int example_id = register_standard_subsystem(
    "Example",
    &example_threads,
    &example_thread,
    &example_shutdown,
    init_example_subsystem,
    shutdown_example_subsystem
);

// Add dependencies
add_subsystem_dependency(example_id, "Logging");
```

This example demonstrates:

1. Proper thread creation and management
2. Safe initialization checks
3. Clean shutdown handling
4. Integration with the Subsystem Registry
5. Logging at appropriate points
6. Dependency management
