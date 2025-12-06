# Data Structures and Types

This document describes the core data structures and types used in the Hydrogen project.

## Queue System

### QueueElement

Core structure for queue entries:

```c
typedef struct QueueElement {
    char* data;              // Message data buffer
    size_t size;            // Size of data
    int priority;           // Message priority
    struct QueueElement* next;
} QueueElement;
```

### Queue

Queue management structure:

```c
typedef struct Queue {
    char* name;              // Queue identifier
    QueueElement* head;      // First element
    QueueElement* tail;      // Last element
    size_t size;            // Current queue size
    pthread_mutex_t mutex;   // Thread safety
} Queue;
```

### QueueAttributes

Queue configuration:

```c
typedef struct QueueAttributes {
    size_t initial_memory;   // Initial memory allocation
    size_t max_memory;       // Maximum memory limit
    int max_priority;        // Highest allowed priority
} QueueAttributes;
```

## Network and WebSocket

### WebSocketMetrics

Connection statistics:

```c
typedef struct {
    time_t server_start_time;   // Server start timestamp
    int active_connections;     // Current connections
    int total_connections;      // Total historical
    int total_requests;         // Total requests
} WebSocketMetrics;
```

### ConnectionInfo

File upload handling:

```c
struct ConnectionInfo {
    FILE *fp;                  // File handle
    char *original_filename;   // Source filename
    char *new_filename;        // Storage filename
    size_t total_size;        // Upload size
    bool print_after_upload;   // Auto-print flag
};
```

## Configuration Structures

### WebConfig

Web server configuration:

```c
typedef struct {
    int enabled;               // Server enabled
    int enable_ipv6;          // IPv6 support
    unsigned int port;        // Listen port
    char *web_root;          // Web files path
    char *upload_path;       // Upload endpoint
    char *upload_dir;        // Upload storage
    size_t max_upload_size;  // Upload limit
    char *log_level;         // Logging detail
} WebConfig;
```

### PrintQueueConfig

Print queue settings:

```c
typedef struct {
    int enabled;              // Queue enabled
    char *log_level;         // Log verbosity
} PrintQueueConfig;
```

## Resource Management

### FileDescriptorInfo

File handle tracking:

```c
typedef struct {
    int fd;                  // File descriptor
    char *type;             // Handle type
    char *description;      // Usage details
} FileDescriptorInfo;
```

### MemoryMetrics

Memory usage tracking:

```c
typedef struct {
    size_t virtual_bytes;    // Virtual memory
    size_t resident_bytes;   // Physical memory
    float allocation_percent; // Usage percentage
} MemoryMetrics;
```

## Usage Guidelines

1. **Thread Safety**
   - Always use mutex protection when accessing Queue structures
   - Follow locking order to prevent deadlocks
   - Use atomic operations for metrics updates

2. **Memory Management**
   - Check QueueAttributes limits before allocations
   - Free resources in reverse allocation order
   - Use WebConfig settings to limit resource usage

3. **Error Handling**
   - Check all memory allocations
   - Validate configuration values
   - Clean up partially initialized structures

4. **Best Practices**
   - Initialize all structure fields
   - Use const for read-only parameters
   - Follow memory alignment requirements

## Related Documentation

- [Coding Guidelines](../development/coding_guidelines.md)
- [API Documentation](./api.md)
- [Configuration Guide](./configuration.md)
