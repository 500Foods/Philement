# Print Queue Architecture

This document provides an in-depth explanation of the Print Queue component in Hydrogen, detailing its internal architecture, data flow, and implementation patterns.

## Overview

The Print Queue is a core component of Hydrogen responsible for managing 3D print jobs throughout their lifecycle. It implements a thread-safe job management system that:

- Receives and prioritizes print jobs
- Manages job state transitions
- Handles communication with printer controllers
- Provides monitoring and control interfaces
- Maintains job persistence across system restarts

## System Architecture

The Print Queue follows a modular design pattern with clear separation of concerns:

```diagram
┌───────────────────────┐
│   Print Queue Manager │
│  ┌─────────────────┐  │
│  │  Job Scheduler  │  │
│  └─────────────────┘  │
│  ┌─────────────────┐  │      ┌───────────────────┐
│  │  State Machine  │◄─┼─────►│  Printer Control  │
│  └─────────────────┘  │      └───────────────────┘
│  ┌─────────────────┐  │      ┌───────────────────┐
│  │ Data Persistence│◄─┼─────►│  File Storage     │
│  └─────────────────┘  │      └───────────────────┘
│  ┌─────────────────┐  │      ┌───────────────────┐
│  │ Queue Analytics │  │      │  WebSocket API    │
│  └─────────────────┘  │      └───────────────────┘
└───────────────────────┘
```

### Key Components

1. **Job Scheduler**: Manages job ordering, priority, and execution timing
2. **State Machine**: Handles state transitions for print jobs
3. **Data Persistence**: Ensures queue state survives restarts
4. **Queue Analytics**: Provides usage statistics and performance data
5. **External Interfaces**: Printer Control and API connections

## Data Flow

The Print Queue maintains a strict data flow to ensure thread safety and system integrity:

```diagram
┌─────────────┐         ┌───────────────┐         ┌─────────────┐
│  REST API   │         │ Print Queue   │         │   Printer   │
│  Endpoints  │◄────────┤   Manager     ├────────►│   Control   │
└─────────────┘         └──────┬────────┘         └─────────────┘
                               │
                        ┌──────▼──────┐
                        │  Internal   │
                        │   Queue     │
                        └──────┬──────┘
                               │
┌─────────────┐         ┌──────▼─────┐         ┌─────────────┐
│  WebSocket  │         │  State     │         │    File     │
│  Updates    │◄────────┤  Updates   ├────────►│   Storage   │
└─────────────┘         └────────────┘         └─────────────┘
```

1. **Input Operations**:
   - Job submission via REST API
   - Control commands via WebSocket API
   - File uploads via HTTP handlers

2. **Processing Operations**:
   - State transitions through the state machine
   - G-code analysis via Beryllium
   - Resource allocation decisions

3. **Output Operations**:
   - Print commands to printer control systems
   - Status updates to WebSocket clients
   - Persistence operations to storage

## Job State Machine

Print jobs follow a well-defined state machine that ensures clear status tracking:

```diagram
                        ┌──────────┐
                        │ Uploaded │
                        └────┬─────┘
                             │
                             ▼
┌──────────┐           ┌───────────┐           ┌─────────┐
│  Failed  │◄──────────┤  Queued   ├──────────►│ Paused  │
└──────────┘           └─────┬─────┘           └────┬────┘
                             │                      │
                             ▼                      │
                       ┌───────────┐                │
                       │ Preparing │                │
                       └─────┬─────┘                │
                             │                      │
                             ▼                      │
                       ┌───────────┐                │
                       │ Printing  │◄───────────────┘
                       └─────┬─────┘
                             │
                             ▼
                      ┌─────────────┐
                      │  Complete   │
                      └─────────────┘
```

Each state transition triggers specific actions:

- **Uploaded → Queued**: Job validation and queue positioning
- **Queued → Preparing**: Resource allocation and printer preparation
- **Preparing → Printing**: Printer connection and initial commands
- **Printing → Complete**: Cleanup and statistics recording
- **Error Transitions**: Comprehensive error handling with recovery options

## Thread Safety Approach

The Print Queue implements multiple thread safety mechanisms:

1. **Mutex Protection**:
   - Queue-level mutex for access to job list
   - Job-level mutex for state changes
   - Fine-grained locking to minimize contention

2. **Two-Phase Commit**:
   - State changes follow a two-phase commit pattern
   - Prevents partial updates during failures

3. **Message Passing**:
   - Thread communication via lock-free message queues
   - Reduces lock contention for status updates

4. **Atomic Operations**:
   - Status flags use atomic operations
   - Ensures visibility across threads

## Storage and Persistence

Print jobs are persistent across system restarts through a combination of:

1. **JSON State Files**:
   - Queue state stored in structured JSON
   - Regular state snapshots during operations
   - Complete persistence on clean shutdown

2. **File Management**:
   - G-code files stored with unique identifiers
   - File lifecycle managed alongside job lifecycle
   - Automatic cleanup of completed/cancelled jobs (configurable)

## Configuration Options

The Print Queue behavior can be extensively customized through configuration:

```json
{
  "PrintQueue": {
    "Enabled": true,
    "MaxJobs": 100,
    "MaximumFileSize": 104857600,
    "PersistenceDirectory": "/var/lib/hydrogen/queue",
    "AutoStart": false,
    "RetentionPolicy": {
      "CompletedJobs": 14,
      "FailedJobs": 7
    },
    "Analytics": {
      "Enabled": true,
      "CollectionInterval": 60
    }
  }
}
```

## Integration with Beryllium

The Print Queue integrates with Beryllium (G-code analyzer) for intelligent job processing:

```diagram
┌───────────────┐                  ┌───────────────┐
│  Print Queue  │                  │   Beryllium   │
│   Manager     │                  │   Analyzer    │
└───────┬───────┘                  └───────┬───────┘
        │                                  │
        │ 1. Analyze File                  │
        ├─────────────────────────────────►│
        │                                  │
        │ 2. File Metadata                 │
        │◄─────────────────────────────────┤
        │                                  │
        │ 3. Extract Print Parameters      │
        ├─────────────────────────────────►│
        │                                  │
        │ 4. Parameter Data                │
        │◄─────────────────────────────────┤
        │                                  │
┌───────▼────────┐                 ┌───────▼───────┐
│  Queue Entry   │                 │  File Cache   │
│  With Metadata │                 │               │
└────────────────┘                 └───────────────┘
```

Beryllium provides the Print Queue with:

- Estimated print time
- Material usage calculations
- Print parameter settings
- Layer information
- Quality predictions

## API Integration

The Print Queue exposes both REST and WebSocket interfaces:

### REST API

**Base Endpoint**: `/api/print`

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/queue` | GET | List all print jobs |
| `/queue/{id}` | GET | Get job details |
| `/queue` | POST | Add new job |
| `/queue/{id}` | DELETE | Remove job |
| `/queue/{id}/pause` | POST | Pause job |
| `/queue/{id}/resume` | POST | Resume job |
| `/queue/reorder` | POST | Reorder queue |

### WebSocket Events

**Channel**: `print_queue`

| Event | Direction | Description |
|-------|-----------|-------------|
| `job_added` | Server → Client | New job added |
| `job_started` | Server → Client | Job began printing |
| `job_paused` | Server → Client | Job paused |
| `job_resumed` | Server → Client | Job resumed |
| `job_completed` | Server → Client | Job finished |
| `job_failed` | Server → Client | Job error |
| `queue_reordered` | Server → Client | Queue order changed |
| `pause_job` | Client → Server | Request job pause |
| `resume_job` | Client → Server | Request job resume |
| `cancel_job` | Client → Server | Request job cancellation |

## Concurrency Model

The Print Queue implements a sophisticated concurrency model:

```diagram
┌───────────────────────────────────────────────────┐
│ Print Queue Manager Thread                        │
│                                                   │
│  ┌─────────────────┐      ┌─────────────────┐     │
│  │ Command Handler │      │  Event Emitter  │     │
│  └────────┬────────┘      └────────▲────────┘     │
│           │                        │              │
│           ▼                        │              │
│  ┌─────────────────────────────────────────┐      │
│  │            Work Queue                   │      │
│  └─────────────────────────────────────────┘      │
│                                                   │
└───────────────────────────────────────────────────┘
                     │
                     │ (Multiple Worker Threads)
                     ▼
┌───────────────┐ ┌───────────────┐ ┌───────────────┐
│ Worker Thread │ │ Worker Thread │ │ Worker Thread │
│               │ │               │ │               │
│┌─────────────┐│ │┌─────────────┐│ │┌─────────────┐│
││ Job Handler ││ ││ Job Handler ││ ││ Job Handler ││
│└─────────────┘│ │└─────────────┘│ │└─────────────┘│
└───────────────┘ └───────────────┘ └───────────────┘
```

Key aspects of this model:

- Main thread handles queue operations and event propagation
- Worker threads manage individual printer connections
- Thread pools scale based on connected printer count
- Lock-free work distribution through message passing

## Error Handling Strategy

The Print Queue implements a robust error handling strategy:

1. **Categorized Errors**:
   - Connection errors
   - Printer response errors
   - G-code parsing errors
   - File system errors
   - Queue management errors

2. **Recovery Mechanisms**:
   - Auto-reconnect for transient network issues
   - Safe job state rollback on partial failures
   - Filesystem transaction safety

3. **Error Propagation**:
   - Structured error reporting to clients
   - Detailed error logging with context
   - Error aggregation for trend analysis

## Performance Considerations

The Print Queue is designed for optimal performance:

1. **Scalability**:
   - Support for hundreds of queued jobs
   - Multiple parallel print operations
   - Resource usage scales with active job count

2. **Memory Management**:
   - Paged job loading for large queues
   - File streaming for large G-code files
   - Memory-mapped access for print parameters

3. **Optimization Points**:
   - Job insertion (O(1) operations)
   - Status updates (lock-free when possible)
   - Query operations (indexed lookups)

## Implementation Files

The Print Queue is implemented in these key files:

| File | Purpose |
|------|---------|
| `src/print/print_queue_manager.c` | Core queue management implementation |
| `src/print/print_queue_manager.h` | Public interface definitions |
| `src/print/beryllium.c` | G-code analysis integration |
| `src/print/beryllium.h` | G-code analyzer interface |

## Usage Examples

### Adding a Job to the Queue

```c
// Initialize print job
print_job_t* job = print_queue_create_job();
if (!job) {
    log_this("PrintQueue", "Failed to create job", LOG_LEVEL_ERROR, true, true, true);
    return false;
}

// Set job properties
job->job_id = generate_unique_id();
job->file_path = strdup(uploaded_file_path);
job->job_name = strdup(job_name);
job->state = JOB_STATE_UPLOADED;

// Add job to queue
if (!print_queue_add_job(queue, job)) {
    log_this("PrintQueue", "Failed to add job to queue", LOG_LEVEL_ERROR, true, true, true);
    print_queue_free_job(job);
    return false;
}

// Analyze G-code (asynchronous)
if (!beryllium_analyze_file(job->file_path, job->job_id, beryllium_callback)) {
    log_this("PrintQueue", "Failed to start G-code analysis", LOG_LEVEL_ALERTING, true, true, true);
    // Job is still queued, but without analysis data
}

return true;
```

### Processing the Queue

```c
void print_queue_process(print_queue_t* queue) {
    if (!queue) return;
    
    pthread_mutex_lock(&queue->mutex);
    
    // Find next eligible job
    print_job_t* next_job = NULL;
    for (int i = 0; i < queue->job_count; i++) {
        if (queue->jobs[i]->state == JOB_STATE_QUEUED) {
            next_job = queue->jobs[i];
            break;
        }
    }
    
    pthread_mutex_unlock(&queue->mutex);
    
    if (next_job) {
        // Transition job state
        print_queue_update_job_state(queue, next_job, JOB_STATE_PREPARING);
        
        // Begin print process
        if (print_queue_connect_to_printer(next_job)) {
            print_queue_update_job_state(queue, next_job, JOB_STATE_PRINTING);
            print_queue_start_monitoring(next_job);
        } else {
            print_queue_update_job_state(queue, next_job, JOB_STATE_FAILED);
            log_this("PrintQueue", "Failed to connect to printer", LOG_LEVEL_ERROR, true, true, true);
        }
    }
}
```

## Related Documentation

- [Print Queue Management](/docs/H/core/subsystems/print//print.md) - User-focused documentation
- [API Documentation](/docs/H/core/reference/api.md) - API endpoint details
- [Configuration Guide](/docs/H/core/reference/configuration.md) - Configuration options
- [Beryllium](/docs/H/core/reference/beryllium.md) - G-code analysis system
