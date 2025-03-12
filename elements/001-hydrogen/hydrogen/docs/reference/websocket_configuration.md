# WebSocket Configuration Guide

## Overview

The WebSocket server provides real-time updates and bidirectional communication for your Hydrogen printer. This guide explains how to configure the WebSocket server through the `WebSocket` section of your `hydrogen.json` configuration file.

## Basic Configuration

Minimal configuration to enable WebSocket support:

```json
{
    "WebSocket": {
        "Enabled": true,
        "Port": 5001,
        "MaxMessageSize": 2048
    }
}
```

## Available Settings

### Core Settings

| Setting | Description | Default | Notes |
|---------|-------------|---------|-------|
| `Enabled` | Turns WebSocket server on/off | `true` | Required |
| `EnableIPv6` | Enables IPv6 support | `false` | Optional |
| `Port` | Network port for WebSocket | `5001` | Required |
| `MaxMessageSize` | Maximum message size in bytes | `2048` | Optional |
| `LogLevel` | Logging detail level | `"WARN"` | Optional |

### Connection Settings

| Setting | Description | Default | Notes |
|---------|-------------|---------|-------|
| `ConnectionTimeouts.ShutdownWaitSeconds` | Graceful shutdown wait time | `2` | Optional |
| `ConnectionTimeouts.ServiceLoopDelayMs` | Service loop delay | `50` | Optional |
| `ConnectionTimeouts.ConnectionCleanupMs` | Connection cleanup interval | `500` | Optional |
| `ConnectionTimeouts.ExitWaitSeconds` | Thread exit timeout during shutdown | `10` | Optional |

## Common Configurations

### Basic Development Setup

```json
{
    "WebSocket": {
        "Enabled": true,
        "Port": 8081,
        "MaxMessageSize": 4096,
        "LogLevel": "DEBUG"
    }
}
```

### Production Setup

```json
{
    "WebSocket": {
        "Enabled": true,
        "EnableIPv6": true,
        "Port": 5001,
        "MaxMessageSize": 2048,
        "LogLevel": "WARN",
        "ConnectionTimeouts": {
            "ShutdownWaitSeconds": 5,
            "ServiceLoopDelayMs": 50,
            "ConnectionCleanupMs": 1000
        }
    }
}
```

### High-Performance Setup

```json
{
    "WebSocket": {
        "Enabled": true,
        "Port": 5001,
        "MaxMessageSize": 8192,
        "ConnectionTimeouts": {
            "ServiceLoopDelayMs": 25,
            "ConnectionCleanupMs": 250
        }
    }
}
```

## Environment Variable Support

You can use environment variables for any setting:

```json
{
    "WebSocket": {
        "Port": "${env.HYDROGEN_WS_PORT}",
        "MaxMessageSize": "${env.HYDROGEN_WS_MAX_SIZE}"
    }
}
```

Common environment variable configurations:

```bash
# Development
export HYDROGEN_WS_PORT=8081
export HYDROGEN_WS_MAX_SIZE=4096

# Production
export HYDROGEN_WS_PORT=5001
export HYDROGEN_WS_MAX_SIZE=2048
```

## Message Types

The WebSocket server handles several types of messages:

1. **Status Updates**: Printer status changes
2. **Print Progress**: Real-time print progress
3. **Temperature Updates**: Current temperatures
4. **Error Notifications**: Error conditions
5. **Command Responses**: Results of commands

## Security Considerations

1. **Connection Management**:
   - Set appropriate `MaxMessageSize` to prevent memory issues
   - Configure reasonable timeouts for cleanup
   - Monitor connection counts

2. **Network Security**:
   - Use a firewall to control WebSocket port access
   - Consider WebSocket secure (WSS) via reverse proxy
   - Validate all incoming messages

3. **Resource Protection**:
   - Monitor memory usage with large messages
   - Configure cleanup intervals appropriately
   - Set proper shutdown wait times

## Troubleshooting

### Common Issues

1. **Connection Problems**:
   - Verify port is not blocked by firewall
   - Check if port is already in use
   - Ensure IPv6 is available if enabled

2. **Performance Issues**:
   - Monitor message sizes
   - Check connection cleanup settings
   - Verify system resources

3. **Memory Usage**:
   - Review `MaxMessageSize` setting
   - Check connection cleanup frequency
   - Monitor system memory

### Diagnostic Steps

#### Enable debug logging

```json
{
    "WebSocket": {
        "LogLevel": "DEBUG"
    }
}
```

#### Check port availability

```bash
netstat -tuln | grep 5001
```

#### Monitor connections

```bash
netstat -an | grep 5001 | wc -l
```

## Client Integration

Example JavaScript client connection:

```javascript
const ws = new WebSocket('ws://your-printer:5001/websocket');

ws.onmessage = function(event) {
    const data = JSON.parse(event.data);
    // Handle different message types
    switch(data.type) {
        case 'status':
            updateStatus(data);
            break;
        case 'progress':
            updateProgress(data);
            break;
        // ... handle other types
    }
};
```

## Related Documentation

- [Web Server Configuration](webserver_configuration.md) - HTTP server settings
- [Print Queue Configuration](printqueue_configuration.md) - Job management
- [Logging Configuration](logging_configuration.md) - Detailed logging setup
- [Network Configuration](network_configuration.md) - Network settings
