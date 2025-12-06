# System Resources Configuration Guide

## Overview

The System Resources component manages Hydrogen's resource allocation, monitoring, and thresholds. It controls queue sizes, buffer allocations, and system monitoring parameters. This guide explains how to configure system resources through the `SystemResources` and `SystemMonitoring` sections of your `hydrogen.json` configuration file.

## Basic Configuration

Minimal configuration for system resources:

```json
{
    "SystemResources": {
        "Queues": {
            "MaxQueueBlocks": 128,
            "QueueHashSize": 256,
            "DefaultQueueCapacity": 1024
        },
        "Buffers": {
            "DefaultMessageBuffer": 1024,
            "MaxLogMessageSize": 2048,
            "PathBufferSize": 256,
            "LineBufferSize": 512
        }
    },
    "SystemMonitoring": {
        "Intervals": {
            "StatusUpdateMs": 1000,
            "ResourceCheckMs": 5000,
            "MetricsUpdateMs": 2000
        },
        "Thresholds": {
            "MemoryWarningPercent": 90,
            "DiskSpaceWarningPercent": 85,
            "LoadAverageWarning": 5.0
        }
    }
}
```

## Available Settings

### Queue Settings

| Setting | Description | Default | Notes |
|---------|-------------|---------|-------|
| `MaxQueueBlocks` | Maximum queue memory blocks | `128` | Required |
| `QueueHashSize` | Queue hash table size | `256` | Required |
| `DefaultQueueCapacity` | Default queue capacity | `1024` | Required |

### Buffer Settings

| Setting | Description | Default | Notes |
|---------|-------------|---------|-------|
| `DefaultMessageBuffer` | Default message size | `1024` | Required |
| `MaxLogMessageSize` | Maximum log message | `2048` | Required |
| `PathBufferSize` | Path string buffer size | `256` | Required |
| `LineBufferSize` | Line buffer size | `512` | Required |

### Monitoring Intervals

| Setting | Description | Default | Notes |
|---------|-------------|---------|-------|
| `StatusUpdateMs` | Status check interval | `1000` | Required |
| `ResourceCheckMs` | Resource check interval | `5000` | Required |
| `MetricsUpdateMs` | Metrics update interval | `2000` | Required |

### Warning Thresholds

| Setting | Description | Default | Notes |
|---------|-------------|---------|-------|
| `MemoryWarningPercent` | Memory usage warning | `90` | Required |
| `DiskSpaceWarningPercent` | Disk usage warning | `85` | Required |
| `LoadAverageWarning` | Load average warning | `5.0` | Required |

## Common Configurations

### Development Setup

```json
{
    "SystemResources": {
        "Queues": {
            "MaxQueueBlocks": 64,
            "QueueHashSize": 128,
            "DefaultQueueCapacity": 512
        },
        "Buffers": {
            "DefaultMessageBuffer": 2048,
            "MaxLogMessageSize": 4096,
            "PathBufferSize": 512,
            "LineBufferSize": 1024
        }
    },
    "SystemMonitoring": {
        "Intervals": {
            "StatusUpdateMs": 500,
            "ResourceCheckMs": 2000,
            "MetricsUpdateMs": 1000
        },
        "Thresholds": {
            "MemoryWarningPercent": 80,
            "DiskSpaceWarningPercent": 75,
            "LoadAverageWarning": 3.0
        }
    }
}
```

### Production Setup

```json
{
    "SystemResources": {
        "Queues": {
            "MaxQueueBlocks": 256,
            "QueueHashSize": 512,
            "DefaultQueueCapacity": 2048
        },
        "Buffers": {
            "DefaultMessageBuffer": 1024,
            "MaxLogMessageSize": 2048,
            "PathBufferSize": 256,
            "LineBufferSize": 512
        }
    },
    "SystemMonitoring": {
        "Intervals": {
            "StatusUpdateMs": 1000,
            "ResourceCheckMs": 5000,
            "MetricsUpdateMs": 2000
        },
        "Thresholds": {
            "MemoryWarningPercent": 90,
            "DiskSpaceWarningPercent": 85,
            "LoadAverageWarning": 5.0
        }
    }
}
```

### High-Performance Setup

```json
{
    "SystemResources": {
        "Queues": {
            "MaxQueueBlocks": 512,
            "QueueHashSize": 1024,
            "DefaultQueueCapacity": 4096
        },
        "Buffers": {
            "DefaultMessageBuffer": 4096,
            "MaxLogMessageSize": 8192,
            "PathBufferSize": 1024,
            "LineBufferSize": 2048
        }
    },
    "SystemMonitoring": {
        "Intervals": {
            "StatusUpdateMs": 500,
            "ResourceCheckMs": 2000,
            "MetricsUpdateMs": 1000
        },
        "Thresholds": {
            "MemoryWarningPercent": 95,
            "DiskSpaceWarningPercent": 90,
            "LoadAverageWarning": 8.0
        }
    }
}
```

## Environment Variable Support

You can use environment variables for thresholds and intervals:

```json
{
    "SystemMonitoring": {
        "Thresholds": {
            "MemoryWarningPercent": "${env.MEMORY_WARNING_THRESHOLD}",
            "DiskSpaceWarningPercent": "${env.DISK_WARNING_THRESHOLD}",
            "LoadAverageWarning": "${env.LOAD_WARNING_THRESHOLD}"
        }
    }
}
```

Common environment variable configurations:

```bash
# Development
export MEMORY_WARNING_THRESHOLD=80
export DISK_WARNING_THRESHOLD=75
export LOAD_WARNING_THRESHOLD=3.0

# Production
export MEMORY_WARNING_THRESHOLD=90
export DISK_WARNING_THRESHOLD=85
export LOAD_WARNING_THRESHOLD=5.0
```

## Resource Management

### Queue Management

- Dynamic queue allocation
- Memory block management
- Hash table optimization
- Queue capacity control

### Buffer Management

- Buffer pool allocation
- Size optimization
- Memory efficiency
- Buffer reuse

### System Monitoring

- Resource usage tracking
- Performance metrics
- Warning thresholds
- Alert generation

## Performance Considerations

1. **Queue Configuration**:
   - Set appropriate block sizes
   - Configure hash table size
   - Optimize queue capacity
   - Monitor queue usage

2. **Buffer Sizing**:
   - Match workload requirements
   - Consider memory constraints
   - Balance performance
   - Monitor buffer usage

3. **Monitoring Settings**:
   - Balance update frequency
   - Set appropriate thresholds
   - Consider system impact
   - Monitor overhead

## Security Considerations

1. **Resource Protection**:
   - Prevent resource exhaustion
   - Monitor usage patterns
   - Set resource limits
   - Implement quotas

2. **Memory Management**:
   - Secure buffer handling
   - Prevent overflows
   - Clear sensitive data
   - Monitor allocations

3. **System Protection**:
   - Monitor system load
   - Protect against DoS
   - Set usage limits
   - Track resource usage

## Best Practices

1. **Queue Configuration**:
   - Size based on workload
   - Consider memory limits
   - Monitor performance
   - Adjust as needed

2. **Buffer Management**:
   - Use appropriate sizes
   - Implement pooling
   - Monitor utilization
   - Handle overflow

3. **System Monitoring**:
   - Set realistic thresholds
   - Monitor consistently
   - React to warnings
   - Track trends

## Troubleshooting

### Common Issues

1. **Memory Problems**:
   - Check queue sizes
   - Review buffer allocation
   - Monitor memory usage
   - Check for leaks

2. **Performance Issues**:
   - Review queue settings
   - Check buffer sizes
   - Monitor system load
   - Analyze metrics

3. **Resource Exhaustion**:
   - Check warning thresholds
   - Monitor resource usage
   - Review allocation
   - Analyze patterns

### Diagnostic Steps

#### Check resource usage

```bash
curl http://your-printer:5000/api/system/resources
```

#### Monitor queues

```bash
curl http://your-printer:5000/api/system/queues
```

#### View system metrics

```bash
curl http://your-printer:5000/api/system/metrics
```

## Related Documentation

- [Print Queue Configuration](printqueue_configuration.md) - Queue settings
- [Logging Configuration](logging_configuration.md) - Log management
- [Monitoring Guide](/docs/reference/monitoring.md) - System monitoring
- [Performance Guide](/docs/reference/performance.md) - Performance tuning
