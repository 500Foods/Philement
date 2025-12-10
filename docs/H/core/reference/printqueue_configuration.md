# Print Queue Configuration Guide

## Overview

The Print Queue system manages and schedules 3D print jobs in Hydrogen. It provides priority-based job scheduling, job status tracking, and queue management features. This guide explains how to configure the Print Queue through the `PrintQueue` section of your `hydrogen.json` configuration file.

## Basic Configuration

Minimal configuration to enable the Print Queue:

```json
{
    "PrintQueue": {
        "Enabled": true,
        "QueueSettings": {
            "DefaultPriority": 1,
            "EmergencyPriority": 0,
            "MaintenancePriority": 2,
            "SystemPriority": 3
        }
    }
}
```

## Available Settings

### Core Settings

| Setting | Description | Default | Notes |
|---------|-------------|---------|-------|
| `Enabled` | Turns Print Queue on/off | `true` | Required |
| `LogLevel` | Logging detail level | `"WARN"` | Optional |

### Priority Settings

| Setting | Description | Default | Notes |
|---------|-------------|---------|-------|
| `QueueSettings.DefaultPriority` | Standard job priority | `1` | Required |
| `QueueSettings.EmergencyPriority` | Emergency job priority | `0` | Required |
| `QueueSettings.MaintenancePriority` | Maintenance job priority | `2` | Required |
| `QueueSettings.SystemPriority` | System job priority | `3` | Required |

### Timeout Settings

| Setting | Description | Default | Notes |
|---------|-------------|---------|-------|
| `Timeouts.ShutdownWaitMs` | Shutdown grace period | `500` | Optional |
| `Timeouts.JobProcessingTimeoutMs` | Job processing timeout | `1000` | Optional |

### Buffer Settings

| Setting | Description | Default | Notes |
|---------|-------------|---------|-------|
| `Buffers.JobMessageSize` | Job message buffer size | `256` | Optional |
| `Buffers.StatusMessageSize` | Status message buffer size | `256` | Optional |

## Priority System

The Print Queue uses a priority-based system where lower numbers indicate higher priority:

1. **Emergency Priority (0)**:
   - Highest priority
   - Used for critical system operations
   - Interrupts current jobs

2. **Default Priority (1)**:
   - Standard print jobs
   - Most user-submitted jobs

3. **Maintenance Priority (2)**:
   - Maintenance operations
   - Calibration routines

4. **System Priority (3)**:
   - Background tasks
   - Non-critical operations

## Common Configurations

### Basic Development Setup

```json
{
    "PrintQueue": {
        "Enabled": true,
        "LogLevel": "DEBUG",
        "QueueSettings": {
            "DefaultPriority": 1,
            "EmergencyPriority": 0,
            "MaintenancePriority": 2,
            "SystemPriority": 3
        },
        "Timeouts": {
            "ShutdownWaitMs": 1000,
            "JobProcessingTimeoutMs": 2000
        }
    }
}
```

### Production Setup

```json
{
    "PrintQueue": {
        "Enabled": true,
        "LogLevel": "WARN",
        "QueueSettings": {
            "DefaultPriority": 1,
            "EmergencyPriority": 0,
            "MaintenancePriority": 2,
            "SystemPriority": 3
        },
        "Timeouts": {
            "ShutdownWaitMs": 500,
            "JobProcessingTimeoutMs": 1000
        },
        "Buffers": {
            "JobMessageSize": 256,
            "StatusMessageSize": 256
        }
    }
}
```

### High-Performance Setup

```json
{
    "PrintQueue": {
        "Enabled": true,
        "LogLevel": "ERROR",
        "QueueSettings": {
            "DefaultPriority": 1,
            "EmergencyPriority": 0,
            "MaintenancePriority": 2,
            "SystemPriority": 3
        },
        "Timeouts": {
            "ShutdownWaitMs": 250,
            "JobProcessingTimeoutMs": 500
        },
        "Buffers": {
            "JobMessageSize": 512,
            "StatusMessageSize": 512
        }
    }
}
```

## Environment Variable Support

You can use environment variables for any setting:

```json
{
    "PrintQueue": {
        "Enabled": "${env.HYDROGEN_QUEUE_ENABLED}",
        "QueueSettings": {
            "DefaultPriority": "${env.HYDROGEN_QUEUE_DEFAULT_PRIORITY}"
        }
    }
}
```

Common environment variable configurations:

```bash
# Development
export HYDROGEN_QUEUE_ENABLED=true
export HYDROGEN_QUEUE_DEFAULT_PRIORITY=1

# Production
export HYDROGEN_QUEUE_ENABLED=true
export HYDROGEN_QUEUE_DEFAULT_PRIORITY=2
```

## Queue Management

The Print Queue system provides several management features:

1. **Job States**:
   - Queued
   - Processing
   - Completed
   - Failed
   - Cancelled

2. **Queue Operations**:
   - Add Job
   - Cancel Job
   - Pause Queue
   - Resume Queue
   - Clear Queue

3. **Job Information**:
   - Job ID
   - Priority
   - Status
   - Progress
   - Estimated Time

## Security Considerations

1. **Queue Protection**:
   - Set appropriate priorities for different job types
   - Configure reasonable timeouts
   - Monitor queue length

2. **Resource Management**:
   - Set appropriate buffer sizes
   - Configure job processing timeouts
   - Monitor system resources

3. **Access Control**:
   - Implement job submission restrictions
   - Control queue management access
   - Log queue operations

## Troubleshooting

### Common Issues

1. **Queue Not Processing**:
   - Check if queue is enabled
   - Verify job processing timeout
   - Check system resources

2. **Job Priority Issues**:
   - Verify priority settings
   - Check job submission process
   - Monitor queue order

3. **Performance Problems**:
   - Review buffer sizes
   - Check timeout settings
   - Monitor system load

### Diagnostic Steps

#### Enable debug logging

```json
{
    "PrintQueue": {
        "LogLevel": "DEBUG"
    }
}
```

#### Monitor queue status

```bash
curl http://your-printer:5000/api/queue/status
```

#### Check queue metrics

```bash
curl http://your-printer:5000/api/queue/metrics
```

## Related Documentation

- [Web Server Configuration](/docs/H/core/reference/webserver_configuration.md) - HTTP API settings
- [WebSocket Configuration](/docs/H/core/reference/websocket_configuration.md) - Real-time updates
- [Logging Configuration](/docs/H/core/reference/logging_configuration.md) - Detailed logging setup
- [System Resources](/docs/H/core/reference/resources_configuration.md) - Resource management
