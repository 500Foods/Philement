# SMTP Relay Configuration Guide

## Overview

The SMTP Relay component enables Hydrogen to send email notifications and alerts. It supports multiple outbound servers, retry mechanisms, and secure email delivery. This guide explains how to configure the SMTP Relay through the `SMTPRelay` section of your `hydrogen.json` configuration file.

## Basic Configuration

Minimal configuration to enable SMTP relay:

```json
{
    "SMTPRelay": {
        "Enabled": true,
        "ListenPort": 587,
        "Workers": 2,
        "OutboundServers": [
            {
                "Host": "smtp.example.com",
                "Port": 587,
                "Username": "user@example.com",
                "Password": "${env.SMTP_PASSWORD}",
                "UseTLS": true
            }
        ]
    }
}
```

## Available Settings

### Core Settings

| Setting | Description | Default | Notes |
|---------|-------------|---------|-------|
| `Enabled` | Turns SMTP relay on/off | `true` | Required |
| `ListenPort` | Local SMTP port | `587` | Required |
| `Workers` | Number of worker threads | `2` | Required |
| `LogLevel` | Logging detail level | `"WARN"` | Optional |

### Queue Settings

| Setting | Description | Default | Notes |
|---------|-------------|---------|-------|
| `QueueSettings.MaxQueueSize` | Maximum queued messages | `1000` | Optional |
| `QueueSettings.RetryAttempts` | Retry count for failed sends | `3` | Optional |
| `QueueSettings.RetryDelaySeconds` | Delay between retries | `300` | Optional |

### Server Configuration

Each server in `OutboundServers` supports:

| Setting | Description | Default | Notes |
|---------|-------------|---------|-------|
| `Host` | SMTP server hostname | - | Required |
| `Port` | SMTP server port | `587` | Required |
| `Username` | Authentication username | - | Required for auth |
| `Password` | Authentication password | - | Required for auth |
| `UseTLS` | Enable TLS encryption | `true` | Recommended |

## Common Configurations

### Basic Development Setup

```json
{
    "SMTPRelay": {
        "Enabled": true,
        "ListenPort": 2525,
        "Workers": 1,
        "QueueSettings": {
            "MaxQueueSize": 100,
            "RetryAttempts": 2,
            "RetryDelaySeconds": 60
        },
        "OutboundServers": [
            {
                "Host": "localhost",
                "Port": 25,
                "UseTLS": false
            }
        ]
    }
}
```

### Production Setup

```json
{
    "SMTPRelay": {
        "Enabled": true,
        "ListenPort": 587,
        "Workers": 4,
        "QueueSettings": {
            "MaxQueueSize": 1000,
            "RetryAttempts": 3,
            "RetryDelaySeconds": 300
        },
        "OutboundServers": [
            {
                "Host": "${env.SMTP_SERVER1_HOST}",
                "Port": "${env.SMTP_SERVER1_PORT}",
                "Username": "${env.SMTP_SERVER1_USER}",
                "Password": "${env.SMTP_SERVER1_PASS}",
                "UseTLS": true
            },
            {
                "Host": "${env.SMTP_SERVER2_HOST}",
                "Port": "${env.SMTP_SERVER2_PORT}",
                "Username": "${env.SMTP_SERVER2_USER}",
                "Password": "${env.SMTP_SERVER2_PASS}",
                "UseTLS": true
            }
        ]
    }
}
```

### High-Availability Setup

```json
{
    "SMTPRelay": {
        "Enabled": true,
        "ListenPort": 587,
        "Workers": 8,
        "QueueSettings": {
            "MaxQueueSize": 5000,
            "RetryAttempts": 5,
            "RetryDelaySeconds": 180
        },
        "OutboundServers": [
            {
                "Host": "smtp1.example.com",
                "Port": 587,
                "Username": "${env.SMTP1_USER}",
                "Password": "${env.SMTP1_PASS}",
                "UseTLS": true
            },
            {
                "Host": "smtp2.example.com",
                "Port": 587,
                "Username": "${env.SMTP2_USER}",
                "Password": "${env.SMTP2_PASS}",
                "UseTLS": true
            }
        ]
    }
}
```

## Environment Variable Support

You can use environment variables for sensitive settings:

```json
{
    "SMTPRelay": {
        "OutboundServers": [
            {
                "Host": "${env.SMTP_HOST}",
                "Port": "${env.SMTP_PORT}",
                "Username": "${env.SMTP_USER}",
                "Password": "${env.SMTP_PASS}"
            }
        ]
    }
}
```

Common environment variable configurations:

```bash
# Development
export SMTP_HOST="localhost"
export SMTP_PORT=25
export SMTP_USER=""
export SMTP_PASS=""

# Production
export SMTP_HOST="smtp.gmail.com"
export SMTP_PORT=587
export SMTP_USER="your-email@gmail.com"
export SMTP_PASS="your-app-specific-password"
```

## Email Templates

The SMTP Relay supports HTML email templates for:

1. **Print Notifications**:
   - Job Started
   - Job Completed
   - Job Failed
   - Print Progress

2. **System Alerts**:
   - Error Conditions
   - Resource Warnings
   - System Updates

3. **Maintenance Notifications**:
   - Scheduled Maintenance
   - Required Updates
   - System Health

## Security Considerations

1. **Authentication**:
   - Use strong passwords
   - Store credentials securely
   - Use environment variables
   - Rotate credentials regularly

2. **Encryption**:
   - Enable TLS when possible
   - Verify certificate validity
   - Use secure ports (587/465)
   - Avoid plain text (port 25)

3. **Access Control**:
   - Restrict relay access
   - Monitor usage patterns
   - Implement rate limiting
   - Log all activity

## Best Practices

1. **Queue Management**:
   - Set appropriate queue sizes
   - Configure retry policies
   - Monitor queue length
   - Handle failed messages

2. **Server Configuration**:
   - Use multiple servers
   - Enable failover
   - Monitor server health
   - Test connectivity

3. **Performance**:
   - Adjust worker count
   - Monitor throughput
   - Configure timeouts
   - Optimize retry delays

## Troubleshooting

### Common Issues

1. **Connection Problems**:
   - Check server settings
   - Verify credentials
   - Test network connectivity
   - Check TLS settings

2. **Queue Issues**:
   - Monitor queue length
   - Check retry settings
   - Verify disk space
   - Review failed messages

3. **Performance Problems**:
   - Adjust worker count
   - Check server capacity
   - Monitor system resources
   - Review queue settings

### Diagnostic Steps

#### Test SMTP connection

```bash
telnet smtp.example.com 587
```

#### Check relay status

```bash
curl http://your-printer:5000/api/smtp/status
```

#### Monitor queue

```bash
curl http://your-printer:5000/api/smtp/queue
```

## Related Documentation

- [Logging Configuration](logging_configuration.md) - Logging setup
- [Security Configuration](security_configuration.md) - Security settings
- [Network Configuration](network_configuration.md) - Network settings
- [Notification Guide](/docs/reference/notifications.md) - Email notification setup
