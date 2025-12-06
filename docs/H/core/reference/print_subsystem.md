# Print Subsystem

This document provides an in-depth overview of the Print subsystem in Hydrogen, detailing its architecture, implementation, and integration with the Launch System.

## Overview

The Print subsystem is a critical component of Hydrogen that manages 3D print jobs from submission to completion. It handles job queuing, printer communication, print monitoring, and status reporting. The Print subsystem provides a reliable and efficient mechanism for managing multiple print jobs across one or more 3D printers.

## System Architecture

The Print subsystem implements a sophisticated architecture with dedicated components for job management, printer communication, and monitoring:

```diagram
┌───────────────────────────────────────────────────────────────────┐
│                        Print Subsystem                            │
│                                                                   │
│  ┌─────────────────┐           ┌─────────────────┐                │
│  │  Print Queue    │           │    Print Job    │                │
│  │    Manager      │◄─────────►│    Processor    │                │
│  └─────────────────┘           └─────────────────┘                │
│          │                              │                         │
│          ▼                              ▼                         │
│  ┌─────────────────┐           ┌─────────────────┐                │
│  │   Job State     │           │    Beryllium    │                │
│  │     Machine     │           │   G-code Parser │                │
│  └─────────────────┘           └─────────────────┘                │
│          │                              │                         │
│          ▼                              ▼                         │
│  ┌─────────────────┐           ┌─────────────────┐                │
│  │ Job Persistence │           │ Printer Control │                │
│  │     Manager     │◄─────────►│  Communication  │                │
│  └─────────────────┘           └─────────────────┘                │
│                                        │                          │
│                      ┌─────────────────┼──────────────────┐       │
│                      │                 │                  │       │
│               ┌──────▼─────┐    ┌──────▼─────┐     ┌──────▼─────┐ │
│               │ Status     │    │ Command    │     │ Monitoring │ │
│               │ Reporting  │    │ Processing │     │  System    │ │
│               └────────────┘    └────────────┘     └────────────┘ │
│                                                                   │
└───────────────────────────────────────────────────────────────────┘
```

### Key Components

1. **Print Queue Manager**: Oversees the queue of print jobs
2. **Print Job Processor**: Handles the lifecycle of individual print jobs
3. **Job State Machine**: Manages print job state transitions
4. **Beryllium G-code Parser**: Analyzes G-code files for print parameters
5. **Job Persistence Manager**: Ensures job state survives system restarts
6. **Printer Control Communication**: Interfaces with 3D printer hardware
7. **Status Reporting**: Provides real-time status updates
8. **Command Processing**: Handles printer control commands
9. **Monitoring System**: Tracks printer health and job progress

## Implementation Details

The Print subsystem is implemented with these primary components:

### Threading Model

The Print subsystem uses a thread pool architecture for parallel job processing:

```c
// Print queue thread pool
static ServiceThreads print_threads = {0};

// Main thread handle
static pthread_t print_thread;

// Shutdown flag
volatile sig_atomic_t print_queue_shutdown = 0;
```

The system creates:

- A main queue management thread
- Worker threads for each active print job
- A persistence thread for state management

### Job State Machine

Print jobs follow a well-defined state machine:

```c
typedef enum {
    PRINT_JOB_UPLOADED,   // Initial state after upload
    PRINT_JOB_QUEUED,     // In queue, waiting to print
    PRINT_JOB_PREPARING,  // Preparing to start printing
    PRINT_JOB_PRINTING,   // Actively printing
    PRINT_JOB_PAUSED,     // Temporarily paused
    PRINT_JOB_COMPLETED,  // Successfully completed
    PRINT_JOB_CANCELLED,  // Cancelled by user
    PRINT_JOB_FAILED      // Failed due to error
} PrintJobState;
```

The state machine handles transitions based on system events and user commands:

```diagram
┌─────────────┐
│  UPLOADED   │
└──────┬──────┘
       │
       ▼
┌─────────────┐       ┌─────────────┐
│   QUEUED    │◄──────┤   PAUSED    │
└──────┬──────┘       └──────▲──────┘
       │                     │
       ▼                     │
┌─────────────┐              │
│  PREPARING  │              │
└──────┬──────┘              │
       │                     │
       ▼                     │
┌─────────────┐              │
│  PRINTING   ├──────────────┘
└──────┬──────┘
       │
       ├─────────────┬─────────────┐
       │             │             │
       ▼             ▼             ▼
┌─────────────┐ ┌─────────────┐ ┌─────────────┐
│  COMPLETED  │ │  CANCELLED  │ │   FAILED    │
└─────────────┘ └─────────────┘ └─────────────┘
```

### Print Queue Data Structure

The Print Queue maintains a priority-based job list:

```c
typedef struct {
    print_job_t** jobs;           // Array of job pointers
    size_t job_count;             // Current number of jobs
    size_t job_capacity;          // Maximum number of jobs
    pthread_mutex_t mutex;        // Queue access mutex
    pthread_cond_t job_added;     // Signal for new job
    pthread_cond_t job_removed;   // Signal for job removal
    bool enabled;                 // Queue enabled state
} print_queue_t;
```

### Buffer Management

The Print subsystem implements efficient buffer management for print commands:

```c
typedef struct {
    size_t command_buffer_size;   // G-code command buffer size
    size_t response_buffer_size;  // Printer response buffer size
    size_t status_buffer_size;    // Status update buffer size
    size_t max_file_buffer;       // Maximum file read buffer
} PrintQueueBuffersConfig;
```

## Integration with Launch System

The Print subsystem integrates with the Hydrogen Launch System through a dedicated launch readiness check:

```c
bool check_print_launch_readiness(LaunchReadiness* readiness) {
    // Check configuration
    readiness->go_config = app_config && 
                         app_config->print_queue.enabled &&
                         app_config->print_queue.buffers.command_buffer_size > 0;
    
    // Check dependencies
    readiness->go_dependencies = is_subsystem_running_by_name("Logging");
    
    // Check resources
    readiness->go_resources = true;  // Resource checks
    
    // Check system conditions
    readiness->go_system = true;  // System checks
    
    // Check state (not in shutdown)
    readiness->go_state = !print_queue_shutdown;
    
    // Final decision
    readiness->go_final = readiness->go_config && 
                        readiness->go_dependencies && 
                        readiness->go_resources && 
                        readiness->go_system && 
                        readiness->go_state;
    
    // Provide status messages
    // (Status message generation code)
    
    return readiness->go_final;
}
```

### Launch Prerequisites

The Print subsystem has the following launch requirements:

1. **Configuration**:
   - Print Queue must be enabled in configuration
   - Buffer sizes must be properly configured
   - Queue settings must be valid

2. **Dependencies**:
   - Logging subsystem must be running

3. **Resources**:
   - Sufficient memory for configured buffer sizes
   - File system access for job storage

4. **State**:
   - Not already in shutdown state

## Initialization Process

The Print subsystem initialization follows this sequence:

```c
bool init_print_subsystem(void) {
    // Block initialization during shutdown
    if (server_stopping || print_queue_shutdown) {
        return false;
    }
    
    // Apply configuration
    size_t command_buffer = app_config->print_queue.buffers.command_buffer_size;
    size_t max_jobs = app_config->print_queue.max_jobs;
    
    // Initialize queue structure
    if (!init_print_queue(max_jobs)) {
        log_this("PrintQueue", "Failed to initialize print queue", LOG_LEVEL_ERROR);
        return false;
    }
    
    // Initialize job storage
    if (!init_print_job_storage()) {
        log_this("PrintQueue", "Failed to initialize job storage", LOG_LEVEL_ERROR);
        cleanup_print_queue();
        return false;
    }
    
    // Initialize Beryllium parser
    if (!init_beryllium_parser()) {
        log_this("PrintQueue", "Failed to initialize G-code parser", LOG_LEVEL_ERROR);
        cleanup_print_job_storage();
        cleanup_print_queue();
        return false;
    }
    
    // Start main thread
    if (pthread_create(&print_thread, NULL, print_queue_main, NULL) != 0) {
        log_this("PrintQueue", "Failed to create main thread", LOG_LEVEL_ERROR);
        cleanup_beryllium_parser();
        cleanup_print_job_storage();
        cleanup_print_queue();
        return false;
    }
    
    // Register with thread tracking
    register_thread(&print_threads, print_thread, "print_main");
    
    log_this("PrintQueue", "Initialized with buffer size %zu and max %zu jobs", 
            LOG_LEVEL_STATE, command_buffer, max_jobs);
    return true;
}
```

## Shutdown Process

The Print subsystem implements a graceful shutdown sequence:

```c
void shutdown_print_queue(void) {
    // Signal shutdown
    print_queue_shutdown = 1;
    
    // Pause all active jobs
    print_queue_pause_all_jobs();
    
    // Wait for main thread to exit
    if (print_thread != 0) {
        pthread_join(print_thread, NULL);
        print_thread = 0;
    }
    
    // Clean up resources
    cleanup_beryllium_parser();
    cleanup_print_job_storage();
    cleanup_print_queue();
    
    log_this("PrintQueue", "Shutdown complete", LOG_LEVEL_STATE);
}
```

## Configuration Options

The Print subsystem is configured through the `PrintQueue` section in the Hydrogen configuration:

```json
{
  "PrintQueue": {
    "Enabled": true,
    "MaxJobs": 100,
    "PersistenceDirectory": "/var/lib/hydrogen/queue",
    "AutoStart": false,
    "Buffers": {
      "CommandBufferSize": 4096,
      "ResponseBufferSize": 4096,
      "StatusBufferSize": 8192,
      "MaxFileBuffer": 65536
    },
    "Timeouts": {
      "CommandTimeout": 5,
      "ConnectionTimeout": 30,
      "StatusInterval": 1
    },
    "Priorities": {
      "DefaultPriority": 50,
      "PrioritizationEnabled": true
    }
  }
}
```

### Key Configuration Parameters

| Parameter | Description | Default | Range |
|-----------|-------------|---------|-------|
| `Enabled` | Enable/disable the print queue | `true` | `true`/`false` |
| `MaxJobs` | Maximum number of jobs in queue | `100` | 1-1000 |
| `PersistenceDirectory` | Job storage directory | `/var/lib/hydrogen/queue` | Valid path |
| `AutoStart` | Auto-start jobs when added | `false` | `true`/`false` |
| `Buffers.CommandBufferSize` | G-code command buffer size | `4096` | 1024-65536 |
| `Timeouts.CommandTimeout` | Command timeout (seconds) | `5` | 1-60 |
| `Priorities.DefaultPriority` | Default job priority | `50` | 0-100 |

## Job Management API

The Print subsystem exposes APIs for job management:

```c
// Create a new print job
print_job_t* print_queue_create_job(void);

// Add a job to the queue
bool print_queue_add_job(print_queue_t* queue, print_job_t* job);

// Start a queued job
bool print_queue_start_job(print_queue_t* queue, const char* job_id);

// Pause a running job
bool print_queue_pause_job(print_queue_t* queue, const char* job_id);

// Resume a paused job
bool print_queue_resume_job(print_queue_t* queue, const char* job_id);

// Cancel a job
bool print_queue_cancel_job(print_queue_t* queue, const char* job_id);

// Get job status
bool print_queue_get_job_status(print_queue_t* queue, const char* job_id, 
                               print_job_status_t* status);
```

## G-code Processing with Beryllium

The Print subsystem integrates with Beryllium for G-code analysis:

```c
// Analyze a G-code file
bool beryllium_analyze_file(const char* file_path, const char* job_id,
                          beryllium_callback_func callback);

// Extract print parameters
bool beryllium_get_print_parameters(const char* file_path, 
                                  print_parameters_t* params);

// Estimate print time
bool beryllium_estimate_print_time(const char* file_path, 
                                 print_time_estimate_t* estimate);
```

## Implementation Files

The Print subsystem implementation is spread across these files:

| File | Purpose |
|------|---------|
| `src/print/print_queue_manager.c` | Core queue management |
| `src/print/print_queue_manager.h` | Public interface |
| `src/print/beryllium.c` | G-code analysis integration |
| `src/print/beryllium.h` | G-code analyzer interface |
| `src/state/startup_print.c` | Initialization integration |
| `src/config/launch/print.c` | Launch readiness check |
| `src/config/print/config_print_queue.c` | Configuration handling |
| `src/config/print/config_print_buffers.c` | Buffer configuration |
| `src/config/print/config_print_timeouts.c` | Timeout configuration |
| `src/config/print/config_print_priorities.c` | Priority configuration |

## Integration Example

Example of adding a print job to the queue:

```c
// Create a new job
print_job_t* job = print_queue_create_job();
if (!job) {
    log_this("Example", "Failed to create job", LOG_LEVEL_ERROR);
    return false;
}

// Set job properties
job->job_id = generate_unique_id();
job->file_path = strdup("/path/to/model.gcode");
job->name = strdup("Sample Print");
job->priority = 50;
job->state = PRINT_JOB_UPLOADED;

// Add to queue
if (!print_queue_add_job(print_queue, job)) {
    log_this("Example", "Failed to add job to queue", LOG_LEVEL_ERROR);
    print_queue_free_job(job);
    return false;
}

// Job ownership transferred to queue, don't free it

// Analyze with Beryllium (optional)
beryllium_analyze_file(job->file_path, job->job_id, analysis_callback);

return true;
```

## Status Monitoring

The Print subsystem provides comprehensive status monitoring:

1. **Real-time Updates**: Status changes propagated through events
2. **Progress Tracking**: Layer completion and percentage done
3. **Error Detection**: Automatic detection of print issues
4. **Statistics Collection**: Print time, material usage, etc.

## Error Handling

The Print subsystem implements robust error handling strategies:

1. **Connection Failures**: Automatic reconnection with exponential backoff
2. **Communication Errors**: Checksums and retransmission
3. **Printer Errors**: Error detection and response
4. **Job Recovery**: Ability to recover from interrupted prints
5. **State Consistency**: Ensures job state remains consistent

## Related Documentation

For more information on Print subsystem-related topics, see:

- [Print Queue Architecture](print_queue_architecture.md) - Detailed architecture
- [Print Queue Configuration](printqueue_configuration.md) - Configuration details
- [Launch System Architecture](launch_system_architecture.md) - Launch system integration
- [Subsystem Registry Architecture](subsystem_registry_architecture.md) - Subsystem management