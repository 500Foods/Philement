# Hydrogen Configuration Guide

## Introduction

Hydrogen uses a JSON-based configuration system that allows you to customize all aspects of the server. This guide explains the core concepts of the configuration system. For detailed settings of specific subsystems, please refer to their dedicated configuration guides.

## Configuration File Basics

### Default Configuration

By default, Hydrogen looks for its settings in `hydrogen.json` in the same directory as the executable. You can specify a different configuration file when starting Hydrogen:

```bash
./hydrogen /path/to/your/config.json
```

### File Structure

The configuration file is organized into logical sections, each controlling a different subsystem:

```json
{
    "ServerName": "Philement/hydrogen",
    "LogFile": "/var/log/hydrogen.log",
    "WebServer": { ... },
    "WebSocket": { ... },
    "PrintQueue": { ... },
    "mDNSServer": { ... },
    "Swagger": { ... },
    "Terminal": { ... },
    "Logging": { ... },
    "SMTPRelay": { ... },
    "Databases": { ... }
}
```

## Subsystem Configuration Guides

Each major subsystem has its own detailed configuration guide:

- [Web Server Configuration](reference/webserver_configuration.md) - HTTP server settings
- [WebSocket Configuration](reference/websocket_configuration.md) - Real-time updates
- [Print Queue Configuration](reference/printqueue_configuration.md) - Job management
- [mDNS Configuration](reference/mdns_configuration.md) - Network discovery
- [Swagger Configuration](reference/swagger_configuration.md) - API documentation
- [Terminal Configuration](reference/terminal_configuration.md) - Web terminal
- [Logging Configuration](reference/logging_configuration.md) - Log management
- [SMTP Relay Configuration](reference/smtp_configuration.md) - Email handling
- [Database Configuration](reference/database_configuration.md) - Database connections
- [System Resources](reference/resources_configuration.md) - Resource management

## Using Environment Variables

Hydrogen supports using environment variables in your configuration file, providing flexibility and security for sensitive settings.

### Basic Syntax

Use the `${env.VARIABLE}` syntax to reference environment variables:

```json
{
    "ServerName": "${env.HYDROGEN_SERVER_NAME}",
    "WebServer": {
        "Port": "${env.HYDROGEN_PORT}"
    }
}
```

### Value Type Conversion

Environment variables are automatically converted to appropriate JSON types:

1. **Null Values**: Empty environment variables become JSON `null`
2. **Boolean Values**: "true"/"false" (case-insensitive) become JSON booleans
3. **Number Values**: Numeric strings become JSON numbers
4. **String Values**: All other values remain strings

### Examples

#### Boolean Settings
```json
{
    "WebServer": {
        "Enabled": "${env.HYDROGEN_WEB_ENABLED}"
    }
}
```
Set `HYDROGEN_WEB_ENABLED=true` to enable the web server.

#### Numeric Settings
```json
{
    "WebServer": {
        "Port": "${env.HYDROGEN_PORT}"
    }
}
```
Set `HYDROGEN_PORT=8080` to change the port number.

#### Path Settings
```json
{
    "LogFile": "${env.HYDROGEN_LOG_PATH}"
}
```
Set `HYDROGEN_LOG_PATH=/var/log/hydrogen.log` to specify the log file location.

### Common Use Cases

1. **Development vs. Production**:
   ```bash
   # Development
   export HYDROGEN_PORT=8080
   export HYDROGEN_LOG_PATH="/tmp/hydrogen.log"
   
   # Production
   export HYDROGEN_PORT=5000
   export HYDROGEN_LOG_PATH="/var/log/hydrogen.log"
   ```

2. **Sensitive Information**:
   ```json
   {
     "Databases": {
       "Password": "${env.HYDROGEN_DB_PASSWORD}"
     }
   }
   ```

3. **Host-Specific Settings**:
   ```json
   {
     "ServerName": "${env.HOSTNAME}-hydrogen"
   }
   ```

### Fallback Behavior

If an environment variable is not found, Hydrogen uses the default value for that setting. This ensures your configuration remains robust even when some variables are undefined.

## Security Best Practices

1. **File Permissions**:
   - Restrict configuration file access to necessary users
   - Use appropriate file permissions (e.g., 600)
   - Keep sensitive settings in environment variables

2. **Environment Variables**:
   - Use environment variables for secrets
   - Consider using a secrets management system
   - Rotate sensitive values periodically

3. **Validation**:
   - Validate configuration files before deployment
   - Test with different environment configurations
   - Monitor for configuration-related errors

## Troubleshooting

1. **Configuration Not Loading**:
   - Check file permissions
   - Verify JSON syntax
   - Ensure environment variables are set

2. **Environment Variables Not Working**:
   - Check variable names and syntax
   - Verify variable values are properly set
   - Look for typos in ${env.VARIABLE} references

3. **Invalid Settings**:
   - Check the specific subsystem's configuration guide
   - Verify value types match expectations
   - Look for missing required settings

## Getting Help

If you encounter issues:

1. Check the relevant subsystem configuration guide
2. Enable debug logging for the affected component
3. Review the log files for error messages
4. Contact support with configuration and logs

Remember to remove sensitive information before sharing configurations.