# Logging Configuration Guide

## Overview

Hydrogen's logging system provides comprehensive logging capabilities with support for multiple output destinations, configurable log levels per subsystem, and structured logging. This guide explains how to configure the logging system through the `Logging` section of your `hydrogen.json` configuration file.

## Basic Configuration

Minimal configuration for logging:

```json
{
    "Logging": {
        "Levels": [
            [0, "TRACE"],
            [1, "DEBUG"],
            [2, "STATE"],
            [3, "ALERT"],
            [4, "ERROR"],
            [5, "FATAL"],
            [6, "QUIET"]
        ],
        "Console": {
            "Enabled": true,
            "DefaultLevel": 1
        },
        "File": {
            "Enabled": true,
            "Path": "/var/log/hydrogen.log",
            "DefaultLevel": 1
        }
    }
}
```

## Log Levels

| Level | Value | Description | Use Case |
|-------|-------|-------------|----------|
| `TRACE` | 0 | All messages | Development debugging |
| `DEBUG` | 1 | Detailed information | Troubleshooting |
| `STATE` | 2 | Normal operations | General status |
| `ALERT` | 3 | Potential issues | Production monitoring |
| `ERROR` | 4 | Error conditions | Error tracking |
| `FATAL` | 5 | Serious problems | Critical issues |
| `QUIET` | 6 | No logging | Disable specific components |

## Output Destinations

### Console Output

```json
{
    "Console": {
        "Enabled": true,
        "DefaultLevel": 1,
        "Subsystems": {
            "ThreadMgmt": 0,
            "Shutdown": 0,
            "mDNSServer": 6,
            "WebServer": 0,
            "WebSocket": 6,
            "PrintQueue": 0,
            "LogQueueManager": 0,
            "Main": 0,
            "QueueSystem": 0,
            "SystemService": 0,
            "Utils": 0,
            "MemoryMetrics": 0,
            "Beryllium": 0,
            "Configuration": 0,
            "API": 0,
            "Initialization": 0,
            "Network": 0
        }
    }
}
```

### File Output

```json
{
    "File": {
        "Enabled": true,
        "DefaultLevel": 1,
        "Path": "/var/log/hydrogen.log",
        "Subsystems": {
            "ThreadMgmt": 1,
            "Shutdown": 1,
            "mDNSServer": 1,
            "WebServer": 1,
            "WebSocket": 1,
            "PrintQueue": 1,
            "LogQueueManager": 1
        }
    }
}
```

### Database Output

```json
{
    "Database": {
        "Enabled": true,
        "DefaultLevel": 4,
        "ConnectionString": "sqlite:///var/lib/hydrogen/logs.db",
        "Subsystems": {
            "ThreadMgmt": 4,
            "Shutdown": 4,
            "WebServer": 4
        }
    }
}
```

## Subsystem Configuration

Each subsystem can have its own log level:

```json
{
    "Subsystems": {
        "ThreadMgmt": 1,    // Normal operation logging
        "Shutdown": 0,      // Detailed shutdown logging
        "mDNSServer": 6,    // No mDNS logging
        "WebServer": 3,     // Debug web server logging
        "WebSocket": 2,     // Warning level for WebSocket
        "PrintQueue": 1     // Normal print queue logging
    }
}
```

## Common Configurations

### Development Setup

```json
{
    "Logging": {
        "Console": {
            "Enabled": true,
            "DefaultLevel": 0,
            "Subsystems": {
                "WebServer": 0,
                "PrintQueue": 0,
                "API": 0
            }
        },
        "File": {
            "Enabled": true,
            "Path": "./hydrogen.log",
            "DefaultLevel": 0
        }
    }
}
```

### Production Setup

```json
{
    "Logging": {
        "Console": {
            "Enabled": true,
            "DefaultLevel": 2,
            "Subsystems": {
                "WebServer": 2,
                "PrintQueue": 2,
                "API": 2
            }
        },
        "File": {
            "Enabled": true,
            "Path": "/var/log/hydrogen.log",
            "DefaultLevel": 1
        },
        "Database": {
            "Enabled": true,
            "DefaultLevel": 4,
            "ConnectionString": "sqlite:///var/lib/hydrogen/logs.db"
        }
    }
}
```

### Minimal Logging Setup

```json
{
    "Logging": {
        "Console": {
            "Enabled": true,
            "DefaultLevel": 4
        },
        "File": {
            "Enabled": true,
            "Path": "/var/log/hydrogen.log",
            "DefaultLevel": 4
        }
    }
}
```

## Environment Variable Support

You can use environment variables for log paths and levels:

```json
{
    "Logging": {
        "File": {
            "Path": "${env.HYDROGEN_LOG_PATH}",
            "DefaultLevel": "${env.HYDROGEN_LOG_LEVEL}"
        }
    }
}
```

Common environment variable configurations:

```bash
# Development
export HYDROGEN_LOG_PATH="./hydrogen.log"
export HYDROGEN_LOG_LEVEL=0

# Production
export HYDROGEN_LOG_PATH="/var/log/hydrogen.log"
export HYDROGEN_LOG_LEVEL=2
```

## Log File Management

### Rotation

Configure your system's logrotate:

```logrotate
/var/log/hydrogen.log {
    daily
    rotate 7
    compress
    delaycompress
    missingok
    notifempty
    create 640 hydrogen hydrogen
}
```

### Permissions

Set appropriate file permissions:

```bash
# Create log directory
sudo mkdir -p /var/log/hydrogen

# Set ownership
sudo chown hydrogen:hydrogen /var/log/hydrogen

# Set permissions
sudo chmod 750 /var/log/hydrogen
```

## Security Considerations

1. **File Security**:
   - Set appropriate file permissions
   - Use secure log directories
   - Implement log rotation

2. **Data Protection**:
   - Avoid logging sensitive data
   - Sanitize log messages
   - Control log access

3. **Resource Management**:
   - Monitor log file sizes
   - Configure appropriate log levels
   - Implement log rotation

## Troubleshooting

### Common Issues

1. **Missing Logs**:
   - Check if logging is enabled
   - Verify log levels
   - Check file permissions

2. **Performance Issues**:
   - Review log levels
   - Check disk space
   - Monitor log file sizes

3. **Access Problems**:
   - Verify file permissions
   - Check directory permissions
   - Validate user access

### Diagnostic Steps

#### Check log file permissions

```bash
ls -la /var/log/hydrogen.log
```

#### Monitor log file growth

```bash
watch -n1 "du -h /var/log/hydrogen.log"
```

#### View real-time logs

```bash
tail -f /var/log/hydrogen.log
```

## Related Documentation

- [System Resources](resources_configuration.md) - Resource management
- [Security Configuration](security_configuration.md) - Security settings
- [Database Configuration](database_configuration.md) - Database logging setup
- [Monitoring Guide](../monitoring.md) - System monitoring
