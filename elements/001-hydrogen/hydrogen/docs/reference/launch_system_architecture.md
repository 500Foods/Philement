# Launch System Architecture

This document describes the Launch System architecture in Hydrogen, which manages pre-launch checks to ensure subsystem dependencies are met before attempting to start each component. The system evaluates each subsystem's prerequisites and determines whether it's safe to proceed with initialization.

## Subsystem Launch Documentation

Detailed launch process documentation for each subsystem:

| Subsystem | Description | Documentation |
|-----------|-------------|---------------|
| PAYLOAD | Manages embedded payloads, minimal dependencies | [Launch Process](launch/payload_subsystem.md) |
| WebServer | HTTP services, depends on Network | [Launch Process](launch/webserver_subsystem.md) |
| Network | Network connectivity foundation | [Coming Soon] |
| Logging | System-wide logging services | [Coming Soon] |
| Database | Data persistence layer | [Coming Soon] |
| WebSocket | Real-time communication | [Coming Soon] |
| Terminal | Command interface | [Coming Soon] |
| mDNS Server/Client | Service discovery | [Coming Soon] |
| Mail Relay | Email services | [Coming Soon] |
| Print Queue | Print job management | [Coming Soon] |
| Swagger | API documentation | [Coming Soon] |

Each subsystem's documentation details:
- Complete launch process flow
- Launch readiness checks
- Landing (shutdown) process
- Error handling
- Logging practices
- Integration points

## Overview

The Launch System is a critical component of Hydrogen that manages the controlled initialization of server subsystems through a series of readiness checks. It implements a go/no-go decision process to ensure:

- Subsystems only start when their dependencies and requirements are met
- Failed subsystems don't cause cascading failures
- The server starts in a consistent and predictable state
- Configuration errors are detected early in the startup process

The Launch System works in tandem with the Subsystem Registry to orchestrate a well-coordinated server startup sequence.

## System Architecture

The Launch System follows a phased approach to subsystem initialization:

```diagram
┌──────────────────────────────────────────────────────────────┐
│                      Launch System                           │
│                                                              │
│  ┌─────────────────┐     ┌─────────────────┐                 │
│  │ Launch Readiness│────►│  Launch Go/No-Go│                 │
│  │    Checks       │     │    Decision     │                 │
│  └─────────────────┘     └────────┬────────┘                 │
│                                   │                          │
│                                   ▼                          │
│          ┌─────────────────────────────────────────┐         │
│          │          Subsystem Registry             │         │
│          │                                         │         │
│          │  ┌─────────────┐     ┌─────────────┐    │         │
│          │  │  Register   │     │   Start     │    │         │
│          │  │  Subsystem  │────►│  Subsystem  │    │         │
│          │  └─────────────┘     └─────────────┘    │         │
│          │                                         │         │
│          └─────────────────────────────────────────┘         │
│                                                              │
└──────────────────────────────────────────────────────────────┘
```

### Key Components

1. **Launch Readiness Checks**: Subsystem-specific verification routines that assess if a subsystem can be safely started
2. **Launch Go/No-Go Decision**: Central decision-making process that evaluates readiness checks and determines whether to proceed
3. **Subsystem Registry Integration**: Process for registering and starting subsystems that pass their readiness checks

## Launch Readiness Process

The Launch System implements a thorough readiness check process for each subsystem. This process follows a progressive validation pattern where critical checks are performed first to fail fast and avoid unnecessary work.

```diagram
┌─────────────────┐        ┌─────────────────┐        ┌─────────────────┐
│  System State   │        │  Configuration  │        │   Resources     │
│     Check       ├───────►│     Check       ├───────►│     Check       │
└─────────────────┘        └─────────────────┘        └─────────────────┘
                                                               │
                                                               ▼
┌─────────────────┐        ┌─────────────────┐        ┌─────────────────┐
│   Final "Go"    │◄───────┤  Dependencies   │◄───────┤  Subsystem      │
│    Decision     │        │     Check       │        │  Specific       │
└─────────────────┘        └─────────────────┘        └─────────────────┘
```

### Why This Order Matters

1. **System State First**: Checks if system is stopping/starting/running
   - Prevents wasted work during shutdown
   - Ensures consistent system state
   - Example: Payload subsystem exits early if server_stopping is true

2. **Configuration Second**: Verifies config is loaded and valid
   - Required for most other checks
   - Validates essential parameters
   - Example: Payload key configuration check

3. **Resources Third**: Checks system resources
   - File system access
   - Memory availability
   - Network ports (if needed)
   - Example: Payload checks executable readability

4. **Subsystem-Specific Fourth**: Custom validation
   - Unique requirements per subsystem
   - Complex validation logic
   - Example: Payload searches for marker in executable

5. **Dependencies Fifth**: Verify other subsystems
   - Only check if previous validations pass
   - Prevents cascade of error messages
   - Example: WebServer checks if Logging is running

6. **Final Decision**: Aggregate all results
   - Clear Go/No-Go decision
   - Detailed status messages
   - Example: "Go/No-Go For Launch of X Subsystem"

### Best Practices for Launch Readiness

1. **Early Exit Pattern**
   ```c
   // Check system state first
   if (server_stopping || web_server_shutdown) {
       add_message("No-Go: System shutting down");
       return (LaunchReadiness){.ready = false, ...};
   }
   
   // Check configuration next
   if (!app_config) {
       add_message("No-Go: Configuration not loaded");
       return (LaunchReadiness){.ready = false, ...};
   }
   ```

2. **Performance Optimization**
   ```c
   // Example from Payload: Check end of file first
   size_t tail_search_size = file_size < 64 ? file_size : 64;
   marker_pos = memmem(data + file_size - tail_search_size, 
                      tail_search_size, marker, strlen(marker));
   
   // Only search whole file if not found in tail
   if (!marker_pos) {
       marker_pos = memmem(data, file_size, marker, strlen(marker));
   }
   ```

3. **Clear Status Messages**
   ```c
   // Format for consistency
   add_message("  Go:      Check Description (details)");
   add_message("  No-Go:   Check Description (reason)");
   add_message("  Decide:  Go/No-Go For Launch of X Subsystem");
   ```

4. **Resource Management**
   ```c
   char* path = get_executable_path();
   if (path) {
       // Use path...
       free(path);  // Clean up allocated resources
       path = NULL; // Prevent use-after-free
   }
   ```

## Launch and Landing Phases

The launch system operates in two distinct phases:

### LAUNCH Phase
- Performs readiness checks
- Initializes subsystem resources
- Registers with subsystem registry
- Example from Payload:
  ```c
  bool launch_payload_subsystem(void) {
      // Launch core functionality
      bool success = launch_payload(app_config, DEFAULT_PAYLOAD_MARKER);
      
      // Register success/failure
      if (success) {
          update_subsystem_on_startup("Payload", true);
      } else {
          log_this("Payload", "Launch failed", LOG_LEVEL_ERROR);
          update_subsystem_on_startup("Payload", false);
      }
      return success;
  }
  ```

### LANDING Phase
- Graceful shutdown of subsystem
- Cleanup of allocated resources
- Unregisters from subsystem registry
- Example from Payload:
  ```c
  void free_payload_resources(void) {
      log_this("Payload", "Freeing resources", LOG_LEVEL_STATE);
      cleanup_openssl();  // Subsystem-specific cleanup
      
      // Prevent double-cleanup during registry shutdown
      int id = get_subsystem_id_by_name("Payload");
      if (id >= 0) {
          update_subsystem_state(id, SUBSYSTEM_INACTIVE);
      }
  }
  ```

```c
// Core structure for launch readiness results
typedef struct {
    const char* subsystem;  // Name of the subsystem
    bool ready;            // Is the subsystem ready to launch?
    const char** messages; // Array of readiness messages (NULL-terminated)
} LaunchReadiness;

// Example readiness check implementation
LaunchReadiness check_subsystem_launch_readiness(void) {
    LaunchReadiness result = {
        .subsystem = "ExampleSubsystem",
        .ready = false,
        .messages = NULL
    };
    
    // Allocate message array (NULL-terminated)
    result.messages = malloc(4 * sizeof(char*));
    if (result.messages) {
        result.messages[0] = strdup("ExampleSubsystem");
        result.messages[1] = strdup("  Go:      System check passed");
        result.messages[2] = strdup("  Decide:  Ready for launch");
        result.messages[3] = NULL;  // NULL terminator
    }
    
    return result;
}
```

Each subsystem has a specific readiness check function that:

1. Populates a LaunchReadiness structure with detailed check results
2. Provides human-readable status messages for each check
3. Makes a final go/no-go decision based on all check results

## Integration with Subsystem Registry

The Launch System and Subsystem Registry work together to ensure proper subsystem initialization:

```diagram
┌────────────────────┐                 ┌────────────────────┐
│   Launch System    │                 │ Subsystem Registry │
│                    │                 │                    │
│  ┌──────────────┐  │                 │ ┌──────────────┐   │
│  │   Readiness  │  │  Registration   │ │  Subsystem   │   │
│  │    Check     │──┼─────────────────► │   Record     │   │
│  └──────────────┘  │                 │ └──────────────┘   │
│                    │                 │        │           │
│  ┌──────────────┐  │                 │        ▼           │
│  │  Dependency  │◄─┼─────────────────┼─┐ ┌──────────────┐ │
│  │  Declaration │  │  Dependencies   │ │ │  Dependency  │ │
│  └──────────────┘  │                 │ └─┤    Graph     │ │
│                    │                 │   └──────────────┘ │
│  ┌──────────────┐  │                 │        │           │
│  │  Initialize  │◄─┼─────────────────┼────────┘           │
│  │  Subsystem   │  │   Initialization│                    │
│  └──────────────┘  │                 │                    │
└────────────────────┘                 └────────────────────┘
```

The integration process follows these steps:

1. **Readiness Check**: Launch System verifies a subsystem is ready to start
2. **Registration**: Subsystem is registered with the Subsystem Registry
3. **Dependency Declaration**: Dependencies are declared to the registry
4. **Initialization**: Subsystem is initialized through the registry

## Launch Sequence and Logging

The launch sequence in Hydrogen is carefully orchestrated and thoroughly logged for debugging and monitoring. The sequence follows this flow:

```diagram
┌─────────────────────┐
│  Server Start       │ ─────┐
└──────────┬──────────┘      │
           │                  │
           ▼                  │
┌─────────────────────┐      │
│  Begin Launch       │      │ LAUNCH READINESS
│  Readiness Logging  │      │ - Subsystem checks
└──────────┬──────────┘      │ - Detailed status
           │                  │
           ▼                  │
┌─────────────────────┐      │
│  Load Configuration │      │
└──────────┬──────────┘      │
           │                  │
           ▼                  │
┌─────────────────────┐      │
│ Initialize Registry │      │
└──────────┬──────────┘      │
           │                  │
           ▼                  │
┌─────────────────────┐      │
│  Check All Launch   │      │
│    Readiness        │      │
└──────────┬──────────┘      │
           │                  ▼
           ▼            ┌─────────────────┐
┌─────────────────────┐│ STARTUP COMPLETE │
│  Process Results    ││ - Go/No-Go status│
└──────────┬──────────┘│ - Per subsystem  │
           │           └─────────────────┘
           ▼                  ▲
┌─────────────────────┐      │
│ Register Subsystems │      │
└──────────┬──────────┘      │ LAUNCH REVIEW
           │                  │ - Total counts
           ▼                  │ - Running time
┌─────────────────────┐      │ - Thread counts
│  Start Subsystems   │      │
│  (Dependency Order) │      │
└──────────┬──────────┘      │
           │                  │
           ▼                  │
┌─────────────────────┐      │
│  Enter Main Loop    │ ─────┘
└─────────────────────┘
```

### Launch Order and Dependencies

The launch system follows a specific order to ensure proper initialization:

1. **Subsystem Registry** (First, Always)
   - Core component for tracking all other subsystems
   - Must be ready before any other subsystem

2. **Payload** (Early)
   - Minimal dependencies
   - Required for other subsystems

3. **Network** (Foundation)
   - Required by many other subsystems
   - Handles basic connectivity

4. **Core Services**
   - Logging
   - Database
   - Configuration

5. **Web Infrastructure**
   - WebServer (depends on Network)
   - API Layer
   - Swagger Documentation

6. **Additional Services**
   - WebSocket (depends on Logging)
   - Terminal (depends on WebServer, WebSocket)
   - mDNS Server/Client (depends on Network)
   - Mail Relay (depends on Network)
   - Print Queue (depends on Logging)

### Launch Status Tracking

The launch system maintains detailed status information:

```c
// Example status tracking for running subsystems
if (state == SUBSYSTEM_RUNNING) {
    time_t running_time = now - subsystem_registry.subsystems[id].state_changed;
    int thread_count = subsystem_registry.subsystems[id].threads->thread_count;
    
    log_this("Launch", "  - %s - Running for %02d:%02d:%02d - Threads: %d",
             LOG_LEVEL_STATE, subsystem_name, hours, minutes, seconds, thread_count);
}
```

Key metrics tracked:
- Running time per subsystem
- Thread count for multi-threaded subsystems
- Current state (STARTING, RUNNING, ERROR)
- Dependency satisfaction status

### Implementation Files and Structure

The Launch System is implemented across several key files:

| File | Purpose |
|------|---------|
| `src/config/launch/launch.h` | Public interface for the launch system |
| `src/config/launch/launch.c` | Core implementation of the launch framework |
| `src/config/launch/*.c` | Subsystem-specific launch readiness checks |
| `src/state/subsystem_registry_integration.c` | Integration with subsystem registry |

## Launch Readiness Checks

Each subsystem has its own launch readiness check implementation. The currently supported subsystems are:

| Subsystem | Check Function | File |
|-----------|---------------|------|
| Logging | `check_logging_launch_readiness()` | `src/config/launch/logging.c` |
| Terminal | `check_terminal_launch_readiness()` | `src/config/launch/terminal.c` |
| mDNS Client | `check_mdns_client_launch_readiness()` | `src/config/launch/mdns_client.c` |
| SMTP Relay | `check_smtp_relay_launch_readiness()` | `src/config/launch/smtp_relay.c` |
| Swagger | `check_swagger_launch_readiness()` | `src/config/launch/swagger.c` |
| WebServer | `check_webserver_launch_readiness()` | `src/config/launch/webserver.c` |
| WebSocket | `check_websocket_launch_readiness()` | `src/config/launch/websocket.c` |
| Print Queue | `check_print_launch_readiness()` | `src/config/launch/print.c` |

## Example: Launch Readiness Check

Below is a typical implementation of a launch readiness check function:

```c
bool check_webserver_launch_readiness(LaunchReadiness* readiness) {
    if (!readiness) return false;
    
    // Initialize the readiness structure
    readiness->subsystem_name = "WebServer";
    readiness->go_config = false;
    readiness->go_dependencies = false;
    readiness->go_resources = false;
    readiness->go_system = false;
    readiness->go_state = false;
    readiness->go_final = false;
    
    // Check 1: Configuration
    if (!app_config) {
        readiness->status_messages[0] = strdup("No-Go: Configuration not loaded");
    } else if (!app_config->webserver.enabled) {
        readiness->status_messages[0] = strdup("No-Go: WebServer is disabled in configuration");
    } else if (app_config->webserver.port < 1 || app_config->webserver.port > 65535) {
        readiness->status_messages[0] = strdup("No-Go: Invalid port configuration");
    } else {
        readiness->go_config = true;
        readiness->status_messages[0] = strdup("Go: Configuration valid");
    }
    
    // Check 2: Dependencies
    if (!is_subsystem_running_by_name("Logging")) {
        readiness->status_messages[1] = strdup("No-Go: Required dependency 'Logging' not running");
    } else {
        readiness->go_dependencies = true;
        readiness->status_messages[1] = strdup("Go: Dependencies available");
    }
    
    // Check 3: Resources
    // (Resource checks like available memory, file descriptors, etc.)
    readiness->go_resources = true;
    readiness->status_messages[2] = strdup("Go: Resources available");
    
    // Check 4: System
    // (System checks like network availability, port availability)
    readiness->go_system = true;
    readiness->status_messages[3] = strdup("Go: System conditions favorable");
    
    // Check 5: State
    if (web_server_shutdown) {
        readiness->status_messages[4] = strdup("No-Go: Subsystem is in shutdown state");
    } else {
        readiness->go_state = true;
        readiness->status_messages[4] = strdup("Go: Subsystem ready to start");
    }
    
    // Final decision
    readiness->go_final = 
        readiness->go_config && 
        readiness->go_dependencies && 
        readiness->go_resources && 
        readiness->go_system && 
        readiness->go_state;
    
    if (readiness->go_final) {
        readiness->status_messages[5] = strdup("GO for launch");
    } else {
        readiness->status_messages[5] = strdup("NO-GO: Launch criteria not met");
    }
    
    return readiness->go_final;
}
```

## Example: Registration from Launch

When a subsystem passes its launch readiness check, it is registered with the subsystem registry:

```c
void check_all_launch_readiness(void) {
    // Initialize a readiness structure
    LaunchReadiness readiness = {0};
    
    // Check and register logging
    if (check_logging_launch_readiness(&readiness)) {
        int id = register_subsystem_from_launch(
            readiness.subsystem_name,
            &logging_threads,
            &logging_thread,
            &logging_shutdown,
            init_logging_subsystem,
            shutdown_logging_subsystem
        );
        
        // Log the result
        if (id >= 0) {
            log_this("Launch", "Logging subsystem registered successfully with ID %d", 
                    LOG_LEVEL_STATE, id);
        } else {
            log_this("Launch", "Failed to register Logging subsystem", LOG_LEVEL_ERROR);
        }
    } else {
        log_this("Launch", "Logging subsystem failed launch readiness check: %s", 
                LOG_LEVEL_ERROR, readiness.status_messages[5]);
    }
    
    // Free allocated memory
    for (int i = 0; i < 6; i++) {
        free(readiness.status_messages[i]);
        readiness.status_messages[i] = NULL;
    }
    
    // (Similar checks for other subsystems...)
}
```

## Launch System Benefits

The Launch System provides several important benefits:

1. **Increased Reliability**: Thorough readiness checks prevent starting subsystems in invalid states
2. **Improved Diagnostics**: Detailed status messages pinpoint exactly why a subsystem couldn't start
3. **Flexible Configuration**: Subsystems can be enabled/disabled without code changes
4. **Controlled Startup**: Dependencies are properly managed to ensure correct startup order
5. **Graceful Degradation**: Non-critical subsystem failures don't prevent server startup

## Related Documentation

For more information on individual subsystems and how they integrate with the Launch System, see:

- [Subsystem Registry Architecture](subsystem_registry_architecture.md) - Registry details
- [WebServer Subsystem](webserver_subsystem.md) - Web server implementation
- [WebSocket Subsystem](websocket_subsystem.md) - WebSocket server implementation
- [Print Subsystem](print_subsystem.md) - Print queue implementation
- [System Architecture](system_architecture.md) - Overall system design