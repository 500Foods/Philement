# Database Configuration Guide

## Overview

The Database component manages connections to various databases used by Hydrogen's subsystems. This guide explains how to configure database connections through the `Databases` section of your `hydrogen.json` configuration file.

## Basic Configuration

Minimal configuration example:

```json
{
    "Databases": {
        "DefaultWorkers": 1,
        "Connections": {
            "Acuranzo": {
                "Enabled": true,
                "Type": "${env.ACURANZO_DB_TYPE}",
                "Database": "${env.ACURANZO_DB_DATABASE}",
                "Host": "${env.ACURANZO_DB_HOST}",
                "Port": "${env.ACURANZO_DB_PORT}",
                "User": "${env.ACURANZO_DB_USER}",
                "Pass": "${env.ACURANZO_DB_PASS}",
                "Workers": 1
            },
            "OIDC": {
                "Enabled": false
            },
            "Log": {
                "Enabled": false
            },
            "Canvas": {
                "Enabled": false
            },
            "Helium": {
                "Enabled": false
            }
        }
    }
}
```

## Available Settings

### Core Settings

| Setting | Description | Default | Notes |
|---------|-------------|---------|-------|
| `DefaultWorkers` | Default worker threads for all databases | `1` | Required |

### Connection Settings

Each connection in `Connections` supports:

| Setting | Description | Default | Required |
|---------|-------------|---------|----------|
| `Enabled` | Enable/disable this database | Varies* | Yes |
| `Type` | Database type ("postgres") | "postgres" | When enabled |
| `Database` | Database name | - | When enabled |
| `Host` | Database hostname | - | When enabled |
| `Port` | Database port | - | When enabled |
| `User` | Authentication username | - | When enabled |
| `Pass` | Authentication password | - | When enabled |
| `Workers` | Worker threads for this database | DefaultWorkers | When enabled |

\* Default enabled state:

- Acuranzo: true
- OIDC: false
- Log: false
- Canvas: false
- Helium: false

## Environment Variables

The Acuranzo database uses environment variables by default:

```bash
# Acuranzo Database Configuration
export ACURANZO_DB_TYPE="postgres"
export ACURANZO_DB_DATABASE="Acuranzo"
export ACURANZO_DB_HOST="carnival.500foods.com"
export ACURANZO_DB_PORT="55433"
export ACURANZO_DB_USER="postgres"
export ACURANZO_DB_PASS="ForEnglandJames"
```

## Configuration Output

The configuration system logs each setting with clear attribution:

```log
Databases
― DefaultWorkers: 1 *
― Connections
  ― Acuranzo
    ― Type: ${env.ACURANZO_DB_TYPE} *
    ― Database: ${env.ACURANZO_DB_DATABASE} *
    ― Host: ${env.ACURANZO_DB_HOST} *
    ― Port: ${env.ACURANZO_DB_PORT} *
    ― User: configured *
    ― Enabled: true *
    ― Workers: 1 *
  ― OIDC
    ― Enabled: false *
  ― Log
    ― Enabled: false *
  ― Canvas
    ― Enabled: false *
  ― Helium
    ― Enabled: false *
```

An asterisk (*) indicates a default value is being used.

## Database Types

### Acuranzo Database (Primary)

The main application database, enabled by default with environment variable configuration.

### OIDC Database

Stores OpenID Connect data when enabled:

- Client registrations
- Access tokens
- Refresh tokens
- User information

### Log Database

Stores application logs when enabled:

- System events
- Error logs
- Audit trails
- Performance metrics

### Canvas Database

Stores canvas-related data when enabled:

- Drawing data
- Canvas states
- User preferences

### Helium Database

Stores Helium-specific data when enabled:

- Integration data
- Cross-system mappings
- Helium states

## Security Considerations

1. **Authentication**:
   - Use strong passwords
   - Store credentials in environment variables
   - Rotate credentials regularly

2. **Network Security**:
   - Use SSL/TLS connections
   - Configure firewall rules
   - Restrict network access

3. **Access Control**:
   - Use least privilege accounts
   - Implement role-based access
   - Regular permission review

## Best Practices

1. **Connection Management**:
   - Set appropriate worker counts
   - Configure timeouts
   - Handle connection errors

2. **Performance**:
   - Optimize worker count
   - Monitor query performance
   - Use connection pooling

3. **Maintenance**:
   - Regular backups
   - Monitor disk space
   - Update statistics

## Troubleshooting

### Common Issues

1. **Connection Problems**:
   - Check credentials
   - Verify network access
   - Test database status

2. **Performance Issues**:
   - Review worker settings
   - Check query performance
   - Monitor system resources

3. **Configuration Problems**:
   - Verify environment variables
   - Check connection strings
   - Review worker settings

### Diagnostic Steps

Test database connection:

```bash
psql -h $ACURANZO_DB_HOST -U $ACURANZO_DB_USER -d $ACURANZO_DATABASE
```

Check connection status:

```bash
curl http://localhost:5000/api/system/db/status
```

## Related Documentation

- [OIDC Configuration](/docs/H/core/reference/oidc_configuration.md)
- [Logging Configuration](/docs/H/core/reference/logging_configuration.md)
- [System Architecture](/docs/H/core/reference/system_architecture.md)
