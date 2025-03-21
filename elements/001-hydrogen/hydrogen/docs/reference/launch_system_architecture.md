# Launch System Architecture

This document describes the Launch System architecture in Hydrogen, detailing the go/no-go decision process for subsystems and how it integrates with the Subsystem Registry.

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

The Launch System implements a thorough readiness check process for each subsystem:

```diagram
┌─────────────────┐        ┌─────────────────┐        ┌─────────────────┐
│  Configuration  │        │   Dependency    │        │   Resources     │
│     Check       ├───────►│     Check       ├───────►│     Check       │
└─────────────────┘        └─────────────────┘        └─────────────────┘
                                                               │
                                                               ▼
┌─────────────────┐        ┌─────────────────┐        ┌─────────────────┐
│   Final "Go"    │◄───────┤  State Check    │◄───────┤  System Check   │
│    Decision     │        │                 │        │                 │
└─────────────────┘        └─────────────────┘        └─────────────────┘
```

Each subsystem undergoes these standard checks:

1. **Configuration Check**: Verifies all required configuration values are present and valid
2. **Dependency Check**: Confirms all dependencies are available and properly initialized
3. **Resources Check**: Ensures required system resources (memory, disk space, etc.) are available
4. **System Check**: Verifies system-wide conditions are favorable for the subsystem
5. **State Check**: Confirms the subsystem is not already in a shutdown state
6. **Final Decision**: Aggregates all checks into a final go/no-go decision

## Go/No-Go Decision Logic

The go/no-go decision process is implemented through a series of readiness check functions:

```c
typedef struct LaunchReadiness {
    const char* subsystem_name;
    bool go_config;            // Configuration is valid
    bool go_dependencies;      // Dependencies are available
    bool go_resources;         // Resources are available
    bool go_system;            // System conditions are favorable
    bool go_state;             // Not in shutdown state
    bool go_final;             // Final decision
    char* status_messages[6];  // Status messages for each check
} LaunchReadiness;

bool check_subsystem_launch_readiness(LaunchReadiness* readiness);
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

## Launch Sequence

The complete launch sequence in Hydrogen follows this flow:

```diagram
┌─────────────────────┐
│  Server Start       │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Load Configuration │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│ Initialize Registry │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Check All Launch   │
│    Readiness        │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐  No  ┌─────────────────────┐
│  All Checks Pass?   ├─────►│  Log Failures and   │
└──────────┬──────────┘      │  Continue with      │
           │ Yes             │  Available Subsys.  │
           ▼                 └─────────────────────┘
┌─────────────────────┐
│ Register Subsystems │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Start Subsystems   │
│  (Dependency Order) │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Enter Main Loop    │
└─────────────────────┘
```

## Implementation Files

The Launch System is implemented in these primary files:

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