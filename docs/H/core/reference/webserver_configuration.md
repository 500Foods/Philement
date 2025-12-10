# Web Server Configuration Guide

## Overview

The web server component of Hydrogen provides the HTTP interface for accessing your printer's API and web-based tools. This guide explains how to configure the web server through the `WebServer` section of your `hydrogen.json` configuration file.

## Basic Configuration

Minimal configuration to start the web server:

```json
{
    "WebServer": {
        "Enabled": true,
        "Port": 5000,
        "WebRoot": "/var/www/html"
    }
}
```

## Available Settings

### Core Settings

| Setting | Description | Default | Notes |
|---------|-------------|---------|-------|
| `Enabled` | Turns the web server on/off | `true` | Required |
| `EnableIPv6` | Enables IPv6 support | `false` | Optional |
| `Port` | Network port for the server | `5000` | Required |
| `WebRoot` | Directory containing web files | `/var/www/html` | Required |
| `ApiPrefix` | Base path for API endpoints | `/api` | Optional |
| `LogLevel` | Logging detail level | `"WARN"` | Optional |

### File Upload Settings

| Setting | Description | Default | Notes |
|---------|-------------|---------|-------|
| `UploadPath` | URL path for file uploads | `/api/upload` | Optional |
| `UploadDir` | Directory for uploaded files | `/tmp/hydrogen_uploads` | Required if uploads enabled |
| `MaxUploadSize` | Maximum file size in bytes | `2147483648` (2GB) | Optional |

## Common Configurations

### Basic Development Setup

```json
{
    "WebServer": {
        "Enabled": true,
        "Port": 8080,
        "WebRoot": "./web",
        "UploadDir": "./uploads",
        "LogLevel": "DEBUG"
    }
}
```

### Production Setup

```json
{
    "WebServer": {
        "Enabled": true,
        "EnableIPv6": true,
        "Port": 5000,
        "WebRoot": "/var/www/hydrogen",
        "UploadDir": "/var/lib/hydrogen/uploads",
        "MaxUploadSize": 1073741824,
        "LogLevel": "WARN"
    }
}
```

### Minimal Memory Setup

```json
{
    "WebServer": {
        "Enabled": true,
        "Port": 5000,
        "WebRoot": "/var/www/hydrogen",
        "MaxUploadSize": 104857600,
        "LogLevel": "ERROR"
    }
}
```

## Environment Variable Support

You can use environment variables for any setting:

```json
{
    "WebServer": {
        "Port": "${env.HYDROGEN_PORT}",
        "WebRoot": "${env.HYDROGEN_WEB_ROOT}",
        "UploadDir": "${env.HYDROGEN_UPLOAD_DIR}"
    }
}
```

Common environment variable configurations:

```bash
# Development
export HYDROGEN_PORT=8080
export HYDROGEN_WEB_ROOT="./web"
export HYDROGEN_UPLOAD_DIR="./uploads"

# Production
export HYDROGEN_PORT=5000
export HYDROGEN_WEB_ROOT="/var/www/hydrogen"
export HYDROGEN_UPLOAD_DIR="/var/lib/hydrogen/uploads"
```

## Security Considerations

1. **File System Security**:
   - Ensure `WebRoot` and `UploadDir` have appropriate permissions
   - Consider placing `UploadDir` on a separate partition
   - Regularly clean up old uploads

2. **Network Security**:
   - Use a firewall to control access to the configured port
   - Consider running behind a reverse proxy for SSL/TLS
   - Set appropriate `MaxUploadSize` to prevent DoS attacks

3. **Logging**:
   - Use `WARN` or higher in production for manageable log sizes
   - Enable `DEBUG` temporarily for troubleshooting
   - Ensure log rotation is configured

## Troubleshooting

### Common Issues

1. **Server Won't Start**:
   - Check if port is already in use
   - Verify directory permissions
   - Ensure IPv6 is available if enabled

2. **Upload Issues**:
   - Verify `UploadDir` exists and is writable
   - Check available disk space
   - Confirm file size is under `MaxUploadSize`

3. **Performance Problems**:
   - Monitor memory usage with large uploads
   - Check log levels aren't too verbose
   - Verify system has enough resources

### Diagnostic Steps

#### Enable debug logging

```json
{
    "WebServer": {
        "LogLevel": "DEBUG"
    }
}
```

#### Check port availability

```bash
netstat -tuln | grep 5000
```

#### Verify directory permissions

```bash
ls -la /var/www/html
ls -la /tmp/hydrogen_uploads
```

## Related Documentation

- [API Configuration](/docs/H/core/reference/api_configuration.md) - Configure API endpoints
- [Swagger Configuration](/docs/H/core/reference/swagger_configuration.md) - API documentation settings
- [Logging Configuration](/docs/H/core/reference/logging_configuration.md) - Detailed logging setup
- [Security Configuration](/docs/H/core/reference/security_configuration.md) - Security best practices
