# Print Queue Management

This document describes the print queue system in the Hydrogen server, which manages 3D print jobs and their execution.

## Overview

The print queue system handles the scheduling and management of print jobs, ensuring orderly processing of print requests and proper resource management.

## Configuration

Print queue settings are managed in the configuration file. See the [Configuration Guide](/docs/H/core/configuration.md#print-job-management-printqueue) for detailed settings.

Basic configuration example:

```json
{
    "PrintQueue": {
        "Enabled": true,
        "LogLevel": "WARN"
    }
}
```

## Queue Interface

### Viewing the Queue

The print queue can be viewed through a web interface:

**Endpoint:** `/print/queue`  
**Method:** GET  
**Response:** HTML page showing current queue status

Example request:

```bash
curl http://localhost:5000/print/queue
```

### Queue Information

The queue interface displays:

- Original filename
- File size
- Print status
- Preview image (if available)
- G-code information

## Implementation Details

### Job Processing Pipeline

The print queue implements a robust job processing pipeline:

1. **Validation Phase**
   - G-code analysis and validation
   - Resource availability checks
   - Safety parameter verification
   - Hardware compatibility checks

2. **Execution Phase**
   - Real-time print monitoring
   - Status updates via WebSocket
   - Error detection and handling
   - Resource usage tracking

3. **Completion Phase**
   - Resource cleanup
   - Job status updates
   - Print statistics collection
   - Job archiving

### Safety Features

The queue system implements several safety measures:

1. **Hardware Safety**
   - Temperature limit enforcement
   - Material compatibility checks
   - Hardware readiness verification
   - Emergency stop handling

2. **Resource Management**
   - Print duration tracking
   - Material consumption monitoring
   - Power management
   - System resource tracking

3. **Error Recovery**
   - Print failure handling
   - State preservation
   - Job resumption capability
   - Automatic resource cleanup

### Technical Implementation

The queue system uses several technical features to ensure reliable operation:

1. **Thread Safety**
   - Mutex-protected operations
   - Condition variable signaling
   - Atomic state updates
   - Race condition prevention

2. **Priority Management**
   - Multi-level priority queue
   - Emergency task prioritization
   - Maintenance task scheduling
   - User priority support

3. **Resource Monitoring**
   - Memory usage tracking
   - File system monitoring
   - Network connection status
   - System load management

For additional details, see:

- [Configuration Guide](/docs/H/core/configuration.md) for setup
- [API Documentation](/docs/H/core/api.md) for endpoints
- [WebSocket Documentation](/docs/H/core/web_socket.md) for real-time updates

## Future Enhancements

Planned features for the print queue system:

1. Job prioritization
2. Print time estimation
3. Material usage tracking
4. Print history
5. Job cancellation and modification

## Contributing

When adding features to the print queue:

1. Update this documentation
2. Add configuration options to configuration.md
3. Document any new API endpoints
4. Include WebSocket events if applicable

Note: This documentation is under active development. Check the [Configuration Guide](/docs/H/core/configuration.md#print-job-management-printqueue) for the most up-to-date settings and options.
