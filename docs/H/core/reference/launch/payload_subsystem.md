# PAYLOAD Subsystem Launch Process

This document details the complete launch and landing process for the PAYLOAD subsystem in Hydrogen.

## Overview

The PAYLOAD subsystem is responsible for managing embedded payloads within the Hydrogen executable. It's one of the first subsystems to launch due to its minimal dependencies and critical role in providing resources to other subsystems.

## Launch Process

```diagram
┌─────────────────────┐
│  Begin PAYLOAD      │
│  Launch Process     │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  System State Check │
│  - Not stopping     │
│  - Starting/Running │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Config Check       │
│  - Config loaded    │
│  - Payload key set  │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Executable Check   │
│  - Get exec path    │
│  - Check readable   │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Payload Search     │
│  - Check last 64B   │
│  - Full file search │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Size Validation    │
│  - Read size bytes  │
│  - Verify limits    │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Launch Decision    │──────┐
└──────────┬──────────┘      │
           │                  │
           ▼                  ▼
    [Success Path]     [Failure Path]
           │                  │
           ▼                  ▼
┌─────────────────┐  ┌───────────────┐
│ Register with   │  │ Log Error &   │
│ Registry        │  │ Exit Launch   │
└─────────────────┘  └───────────────┘
```

### Launch Readiness Checks

The PAYLOAD subsystem performs these checks in sequence:

1. **System State Check**

   ```c
   if (server_stopping || web_server_shutdown) {
       add_message("No-Go: System shutting down");
       return false;
   }
   ```

   - Ensures system isn't in shutdown
   - Verifies system is starting or running

2. **Configuration Check**

   ```c
   const char* payload_key = app_config->server.payload_key;
   if (!payload_key || strcmp(payload_key, "Missing Key") == 0) {
       add_message("No-Go: Decryption Key not configured");
       return false;
   }
   ```

   - Verifies configuration is loaded
   - Checks for valid payload key

3. **Executable Check**

   ```c
   char* executable_path = get_executable_path();
   if (!executable_path) {
       add_message("No-Go: Failed to determine executable path");
       return false;
   }
   ```

   - Gets executable path
   - Verifies file is readable

4. **Payload Search**

   ```c
   // Performance optimization: Check last 64 bytes first
   size_t tail_search_size = file_size < 64 ? file_size : 64;
   marker_pos = memmem(data + file_size - tail_search_size, 
                      tail_search_size, marker, strlen(marker));
   ```

   - Optimized search starting with last 64 bytes
   - Falls back to full file search if needed

5. **Size Validation**

   ```c
   if (*payload_size > 100 * 1024 * 1024) {  // 100MB limit
       add_message("No-Go: Payload size exceeds limit");
       return false;
   }
   ```

   - Reads payload size bytes
   - Validates against size limits

### Launch Subsystem Process

After passing readiness checks, the PAYLOAD subsystem:

1. **Extracts Payload**

   ```c
   bool launch_payload_subsystem(void) {
       bool success = launch_payload(app_config, DEFAULT_PAYLOAD_MARKER);
       if (success) {
           update_subsystem_on_startup("Payload", true);
       }
       return success;
   }
   ```

2. **Registers with Registry**

   ```c
   register_subsystem_from_launch(
       "Payload",
       NULL,  // No thread structure
       NULL,  // No thread handle
       NULL,  // No shutdown flag
       NULL,  // No init function
       free_payload_resources
   );
   ```

## Landing Process

```diagram
┌─────────────────────┐
│  Begin PAYLOAD      │
│  Landing Process    │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Log Landing State  │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Cleanup OpenSSL    │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Get Subsystem ID   │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Mark Inactive      │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│  Landing Complete   │
└─────────────────────┘
```

The landing process is handled by `free_payload_resources()`:

```c
void free_payload_resources(void) {
    // Log landing state
    log_this("Payload", "Freeing resources", LOG_LEVEL_STATE);
    
    // Cleanup crypto resources
    cleanup_openssl();
    
    // Mark subsystem as inactive
    int id = get_subsystem_id_by_name("Payload");
    if (id >= 0) {
        update_subsystem_state(id, SUBSYSTEM_INACTIVE);
    }
}
```

## Error Handling

The PAYLOAD subsystem implements robust error handling:

1. **Memory Management**
   - All allocated resources are properly freed
   - Paths and buffers are cleaned up

2. **File Operations**
   - File descriptor leaks are prevented
   - Memory mappings are properly unmapped

3. **Crypto Operations**
   - OpenSSL resources are cleaned up
   - Keys are securely wiped

## Logging

The subsystem provides detailed logging at key points:

```c
// Launch logging
log_this("PayloadLaunch", "Checking executable: %s", LOG_LEVEL_STATE, path);
log_this("PayloadLaunch", "Found payload: %zu bytes", LOG_LEVEL_STATE, size);

// Error logging
log_this("PayloadLaunch", "Failed to open executable: %s", LOG_LEVEL_ERROR, 
         strerror(errno));

// Landing logging
log_this("Payload", "Beginning resource cleanup", LOG_LEVEL_STATE);
log_this("Payload", "Resources freed", LOG_LEVEL_STATE);
```

## Integration Points

The PAYLOAD subsystem integrates with:

1. **Configuration System**
   - Reads payload key configuration
   - Validates configuration values

2. **Subsystem Registry**
   - Registers as a subsystem
   - Updates state during launch/landing

3. **Logging System**
   - Provides detailed status messages
   - Records errors and warnings

## Related Documentation

- [Launch System Architecture](/docs/H/core/reference/launch_system_architecture.md)
- [Configuration System](/docs/H/core/reference/configuration.md)
- [Subsystem Registry](/docs/H/core/reference//subsystem_registry_architecture.md)
