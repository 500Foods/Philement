# Lead DQM (Database Queue Manager) Developer Guide

## Overview

The **Lead DQM** is the central coordinator for database operations within Hydrogen's database subsystem. Each database gets exactly one Lead DQM that manages the entire database lifecycle, from connection establishment through query processing to graceful shutdown.

## Architecture

The Lead DQM implements a **hierarchical queue management** pattern:

```hierarchy
Database Instance
└── Lead DQM (Queue 00)
    ├── Slow Queue (Queue 01+)
    ├── Medium Queue (Queue 02+)
    ├── Fast Queue (Queue 03+)
    └── Cache Queue (Queue 04+)
```

## Launch Sequence

### 1. Initialization Phase

When `database_add_database()` is called:

```c
// Core initialization sequence
bool database_add_database(const char* name, const char* engine, const char* connection_string) {
    // 1. Validate parameters and engine interface
    // 2. Build engine-specific connection string
    // 3. Create Lead queue with connection string
    // 4. Start Lead queue worker thread
    // 5. Register with global queue manager
}
```

**Key Components Created:**

- **DatabaseQueue** structure with Lead queue designation
- **Worker thread** for the Lead queue (single thread per queue)
- **Connection to database** using specified engine
- **Heartbeat monitoring** system for connection health

### 2. Lead Queue Creation

The Lead queue is created with these characteristics:

```c
typedef struct DatabaseQueue {
    char* database_name;           // e.g., "Acuranzo"
    char* connection_string;       // Engine-specific connection string
    char* queue_type;              // "Lead" - identifies as Lead queue

    // Lead-specific properties
    volatile bool is_lead_queue;   // Always true for Lead DQM
    volatile bool can_spawn_queues; // Always true for Lead DQM
    DatabaseQueue** child_queues;  // Array for spawned child queues
    int max_child_queues;          // Configurable limit

    // Threading
    pthread_t worker_thread;       // Single worker thread
    pthread_mutex_t queue_access_lock;
    sem_t worker_semaphore;

    // Statistics
    volatile int active_connections;
    volatile int total_queries_processed;
    volatile int current_queue_depth;
}
```

### 3. Worker Thread Launch

Each Lead DQM launches exactly **one worker thread**:

```c
// Simplified worker thread lifecycle
void* database_queue_worker_thread(void* arg) {
    DatabaseQueue* queue = (DatabaseQueue*)arg;

    while (!queue->shutdown_requested) {
        // 1. Process queries from queue
        // 2. Execute database operations
        // 3. Handle connection management
        // 4. Monitor child queue health
        // 5. Participate in heartbeat system
    }
}
```

## Operational Lifecycle

### Connection Establishment

The Lead DQM handles the initial database connection:

```c
// Connection process (simplified)
bool establish_database_connection(DatabaseQueue* lead_queue) {
    // 1. Load engine-specific library (dlopen)
    // 2. Get function pointers (dlsym)
    // 3. Build connection configuration
    // 4. Establish connection using engine interface
    // 5. Test connection with simple query
    // 6. Set up connection pooling if needed
}
```

**Connection String Examples:**

- **PostgreSQL**: `postgresql://user:pass@host:5432/database`
- **SQLite**: `/path/to/database.db` or `:memory:`
- **MySQL**: `mysql://user:pass@host:3306/database`

### Heartbeat System

The Lead DQM implements a **30-second heartbeat cycle**:

```c
// Heartbeat monitoring (simplified)
void database_queue_heartbeat(DatabaseQueue* queue) {
    while (!queue->shutdown_requested) {
        // 1. Check database connectivity
        // 2. Attempt reconnection if needed
        // 3. Monitor child queue health
        // 4. Update connection statistics
        // 5. Log status every 30 seconds

        sleep(30); // Configurable interval
    }
}
```

**Heartbeat Logging Format:**

```log
DQM-Acuranzo-00-LSMFC [Connected] - Active connections: 1, Queries processed: 150
DQM-Acuranzo-00-LMC [Reconnecting] - Connection lost, attempting to reconnect...
```

### Child Queue Management

The Lead DQM dynamically manages child queues based on workload:

#### Spawning Child Queues

```c
bool database_queue_spawn_child_queue(DatabaseQueue* lead_queue, const char* queue_type) {
    // 1. Check if child queue type already exists
    // 2. Verify we haven't hit max_child_queues limit
    // 3. Create new DatabaseQueue with specified type
    // 4. Assign next available queue number (01, 02, 03...)
    // 5. Start worker thread for child queue
    // 6. Update Lead queue tags (remove tag assigned to child)
    // 7. Add child to child_queues array
}
```

**Tag Management:**

- **Initial State**: Lead queue starts with all tags: `LSMFC`
- **After Spawning Fast Queue**: Lead becomes `LSMC`, Fast queue gets `F`
- **Queue Numbering**: Lead is always `00`, children start from `01`

#### Scaling Logic

The Lead DQM implements intelligent scaling:

```c
// Dynamic scaling decision (simplified)
void evaluate_scaling_needs(DatabaseQueue* lead_queue) {
    for each queue_type (slow, medium, fast, cache) {
        // Scale UP when:
        // - All queues of type are non-empty
        // - Max queues for type not reached
        // - Configuration allows scaling

        // Scale DOWN when:
        // - All queues of type are empty
        // - Above minimum configured count
    }
}
```

### Query Processing Flow

When a query arrives at the Lead DQM:

```c
// Query routing (simplified)
DatabaseQueryResult process_query(DatabaseQueue* lead_queue, QueryRequest* request) {
    // 1. Determine appropriate child queue based on:
    //    - Query priority hints
    //    - Queue depth and availability
    //    - Configuration-based routing rules

    // 2. Route to child queue or handle directly if no children

    // 3. Monitor query completion and update statistics

    // 4. Handle any child queue failures or overload
}
```

## Queue Labeling System

The Lead DQM uses a structured labeling system for logging and monitoring:

**Format:** `DQM-<DatabaseName>-<QueueNumber>-<TagLetters>`

**Examples:**

- `DQM-Acuranzo-00-LSMFC` - Lead queue with all tags
- `DQM-Acuranzo-00-LMC` - Lead after spawning Fast and Slow queues
- `DQM-Acuranzo-01-F` - Fast child queue
- `DQM-Acuranzo-02-S` - Slow child queue

**Tag Meanings:**

- `L` = Lead queue (always present on Lead DQM)
- `S` = Slow queue capability
- `M` = Medium queue capability
- `F` = Fast queue capability
- `C` = Cache queue capability

## Thread Safety

The Lead DQM implements comprehensive thread safety:

```c
// Key synchronization primitives
typedef struct DatabaseQueue {
    // Queue access protection
    pthread_mutex_t queue_access_lock;

    // Child queue management protection
    pthread_mutex_t children_lock;

    // Worker thread coordination
    sem_t worker_semaphore;

    // Volatile fields for atomic access
    volatile bool shutdown_requested;
    volatile bool is_connected;
    volatile int active_connections;
    volatile int total_queries_processed;
}
```

**Lock Hierarchy:**

1. `queue_access_lock` - Protects queue operations
2. `children_lock` - Protects child queue array modifications
3. `worker_semaphore` - Coordinates worker thread execution

## Error Handling and Recovery

### Connection Failures

```c
// Connection recovery (simplified)
void handle_connection_failure(DatabaseQueue* lead_queue) {
    // 1. Mark connection as failed
    // 2. Log connection error with context
    // 3. Attempt reconnection with exponential backoff
    // 4. If reconnection fails repeatedly:
    //    - Notify child queues
    //    - Queue incoming queries for later processing
    //    - Continue heartbeat monitoring
}
```

### Child Queue Failures

```c
// Child queue recovery (simplified)
void handle_child_queue_failure(DatabaseQueue* lead_queue, DatabaseQueue* failed_child) {
    // 1. Log child queue failure
    // 2. Determine if replacement needed based on:
    //    - Minimum queue requirements
    //    - Current workload demands
    //    - Configuration settings

    // 3. If replacement needed, spawn new child queue
    // 4. Update Lead queue statistics and tags
}
```

## Shutdown Sequence

When the Lead DQM shuts down:

```c
// Graceful shutdown (simplified)
void shutdown_lead_queue(DatabaseQueue* lead_queue) {
    // 1. Set shutdown_requested flag
    // 2. Stop accepting new queries
    // 3. Wait for current queries to complete
    // 4. Shutdown all child queues
    // 5. Close database connections
    // 6. Clean up child queue array
    // 7. Join worker thread
    // 8. Free all resources
}
```

**Shutdown Order:**

1. Lead queue signals shutdown
2. Child queues complete current work
3. Child queues shut down in reverse spawn order
4. Lead queue closes database connections
5. Lead queue joins worker thread
6. Memory cleanup and resource deallocation

## Monitoring and Debugging

### Key Metrics

The Lead DQM tracks these metrics:

```c
// Statistics tracked by Lead DQM
volatile int active_connections;      // Current active connections
volatile int total_queries_processed; // Total queries handled
volatile int current_queue_depth;     // Current queue depth
volatile int child_queue_count;       // Number of active child queues
time_t connection_established;        // When connection was established
int consecutive_failures;             // Connection failure count
```

### Debug Logging

Enable debug logging to see:

```bash
# Set log level to DEBUG for database operations
export LOG_LEVEL_DATABASE=1

# Monitor specific database
tail -f /var/log/hydrogen.log | grep "DQM-Acuranzo"
```

**Common Debug Scenarios:**

- Connection establishment failures
- Child queue spawning issues
- Query routing problems
- Heartbeat irregularities
- Shutdown hangs or deadlocks

## Configuration

The Lead DQM is configured through the main database configuration:

```json
{
  "Databases": {
    "DefaultWorkers": 2,
    "MaxChildQueues": 10,
    "HeartbeatIntervalSeconds": 30,
    "Connections": [
      {
        "Name": "Acuranzo",
        "Engine": "postgresql",
        "Host": "localhost",
        "Port": 5432,
        "Database": "acuranzo_prod",
        "User": "hydrogen_user",
        "Queues": {
          "Slow": {"Min": 1, "Max": 3},
          "Medium": {"Min": 2, "Max": 5},
          "Fast": {"Min": 1, "Max": 8},
          "Cache": {"Min": 1, "Max": 2}
        }
      }
    ]
  }
}
```

## Performance Considerations

### Threading Model

- **Single Thread per Queue**: Each Lead DQM and child queue runs in exactly one thread
- **Non-Blocking Operations**: Database operations should not block the worker thread
- **Async Processing**: Long-running queries should be handled asynchronously

### Memory Management

- **Connection Reuse**: Database connections are reused across queries
- **Resource Cleanup**: Proper cleanup prevents memory leaks
- **Child Queue Limits**: Configurable limits prevent resource exhaustion

### Scaling Strategy

- **Horizontal Scaling**: Scale by adding more server instances, not more threads
- **Dynamic Adjustment**: Child queues spawn/destroy based on workload
- **Configuration Limits**: Prevent runaway resource consumption

## Integration Points

### Launch/Landing System

The Lead DQM integrates with Hydrogen's launch/landing system:

```c
// Launch sequence integration
void launch_database_subsystem(void) {
    // 1. Initialize database subsystem
    // 2. Load database configurations
    // 3. Create Lead DQMs for each database
    // 4. Start Lead queue worker threads
    // 5. Register with global queue manager
}

// Landing sequence integration
void landing_database_subsystem(void) {
    // 1. Signal shutdown to all Lead DQMs
    // 2. Wait for child queues to complete
    // 3. Close all database connections
    // 4. Clean up Lead DQM resources
}
```

### Global Queue Manager

The Lead DQM registers with the global queue manager:

```c
// Registration process
bool register_with_global_manager(DatabaseQueue* lead_queue) {
    // 1. Add Lead queue to global queue registry
    // 2. Set up cross-queue communication
    // 3. Configure load balancing if needed
    // 4. Enable monitoring and statistics collection
}
```

## Best Practices

### Development Guidelines

1. **Always Use Lead DQM Pattern**: Don't create multiple queues per database manually
2. **Monitor Queue Depths**: Watch for queues that consistently hit limits
3. **Configure Appropriate Limits**: Set realistic min/max queue counts
4. **Handle Connection Failures**: Implement proper retry and recovery logic
5. **Log Appropriately**: Use structured logging with DQM labels

### Operational Guidelines

1. **Monitor Heartbeat Logs**: Check that heartbeats occur every 30 seconds
2. **Watch Child Queue Creation**: Ensure child queues spawn when needed
3. **Check Resource Usage**: Monitor memory and thread usage
4. **Plan Scaling**: Use multiple Hydrogen instances for horizontal scaling
5. **Test Failures**: Verify behavior when database connections fail

### Troubleshooting Common Issues

#### Issue: Lead DQM Won't Start

- Check database connectivity and credentials
- Verify engine library is available (dlopen)
- Check configuration file syntax
- Review launch sequence logs

#### Issue: Child Queues Not Spawning

- Verify configuration allows queue creation
- Check if max_child_queues limit reached
- Review Lead DQM logs for spawn attempts
- Ensure sufficient system resources

#### Issue: Poor Query Performance

- Check queue depth and routing logic
- Verify database connection health
- Review query complexity and optimization
- Consider adjusting queue min/max settings

#### Issue: Memory Leaks

- Check proper cleanup in shutdown sequence
- Verify connection and thread resource cleanup
- Monitor child queue creation/destruction
- Review prepared statement management

This document provides a comprehensive guide to understanding, implementing, and troubleshooting Lead DQM functionality in the Hydrogen database subsystem.