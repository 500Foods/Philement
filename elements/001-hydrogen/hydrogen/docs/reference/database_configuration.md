# Database Configuration Guide

## Overview

The Database component manages connections to various databases used by Hydrogen's subsystems, including OIDC authentication, logging, and application data. This guide explains how to configure database connections through the `Databases` section of your `hydrogen.json` configuration file.

## Basic Configuration

Minimal configuration for database support:

```json
{
    "Databases": {
        "Workers": 1,
        "Connections": {
            "OIDC": {
                "Type": "postgres",
                "Host": "localhost",
                "Port": 5432,
                "Database": "hydrogen_oidc",
                "Username": "hydrogen",
                "Password": "${env.OIDC_DB_PASS}",
                "MaxConnections": 10
            }
        }
    }
}
```

## Available Settings

### Core Settings

| Setting | Description | Default | Notes |
|---------|-------------|---------|-------|
| `Workers` | Database worker threads | `1` | Required |
| `LogLevel` | Logging detail level | `"WARN"` | Optional |

### Connection Settings

Each connection in `Connections` supports:

| Setting | Description | Default | Notes |
|---------|-------------|---------|-------|
| `Type` | Database type | - | Required ("postgres") |
| `Host` | Database hostname | - | Required |
| `Port` | Database port | - | Required |
| `Database` | Database name | - | Required |
| `Username` | Authentication username | - | Required |
| `Password` | Authentication password | - | Required |
| `MaxConnections` | Connection pool size | `10` | Optional |

## Database Types

### OIDC Database
Stores OpenID Connect data:
- Client registrations
- Access tokens
- Refresh tokens
- User information

### Logging Database
Stores application logs:
- System events
- Error logs
- Audit trails
- Performance metrics

### Application Database
Stores application-specific data:
- Print job history
- System settings
- User preferences
- Statistics

## Common Configurations

### Development Setup
```json
{
    "Databases": {
        "Workers": 1,
        "Connections": {
            "OIDC": {
                "Type": "postgres",
                "Host": "localhost",
                "Port": 5432,
                "Database": "hydrogen_oidc_dev",
                "Username": "hydrogen_dev",
                "Password": "${env.OIDC_DB_PASS}",
                "MaxConnections": 5
            },
            "Logging": {
                "Type": "postgres",
                "Host": "localhost",
                "Port": 5432,
                "Database": "hydrogen_logs_dev",
                "Username": "hydrogen_dev",
                "Password": "${env.LOGS_DB_PASS}",
                "MaxConnections": 5
            }
        }
    }
}
```

### Production Setup
```json
{
    "Databases": {
        "Workers": 4,
        "Connections": {
            "OIDC": {
                "Type": "postgres",
                "Host": "${env.OIDC_DB_HOST}",
                "Port": "${env.OIDC_DB_PORT}",
                "Database": "${env.OIDC_DB_NAME}",
                "Username": "${env.OIDC_DB_USER}",
                "Password": "${env.OIDC_DB_PASS}",
                "MaxConnections": 20
            },
            "Logging": {
                "Type": "postgres",
                "Host": "${env.LOGS_DB_HOST}",
                "Port": "${env.LOGS_DB_PORT}",
                "Database": "${env.LOGS_DB_NAME}",
                "Username": "${env.LOGS_DB_USER}",
                "Password": "${env.LOGS_DB_PASS}",
                "MaxConnections": 10
            }
        }
    }
}
```

### High-Availability Setup
```json
{
    "Databases": {
        "Workers": 8,
        "Connections": {
            "OIDC": {
                "Type": "postgres",
                "Host": "${env.OIDC_DB_HOST}",
                "Port": "${env.OIDC_DB_PORT}",
                "Database": "${env.OIDC_DB_NAME}",
                "Username": "${env.OIDC_DB_USER}",
                "Password": "${env.OIDC_DB_PASS}",
                "MaxConnections": 50,
                "ReadReplicas": [
                    {
                        "Host": "${env.OIDC_DB_REPLICA1_HOST}",
                        "Port": "${env.OIDC_DB_REPLICA1_PORT}"
                    }
                ]
            }
        }
    }
}
```

## Environment Variable Support

You can use environment variables for sensitive settings:

```json
{
    "Databases": {
        "Connections": {
            "OIDC": {
                "Host": "${env.OIDC_DB_HOST}",
                "Port": "${env.OIDC_DB_PORT}",
                "Database": "${env.OIDC_DB_NAME}",
                "Username": "${env.OIDC_DB_USER}",
                "Password": "${env.OIDC_DB_PASS}"
            }
        }
    }
}
```

Common environment variable configurations:
```bash
# Development
export OIDC_DB_HOST="localhost"
export OIDC_DB_PORT=5432
export OIDC_DB_NAME="hydrogen_oidc_dev"
export OIDC_DB_USER="hydrogen_dev"
export OIDC_DB_PASS="dev_password"

# Production
export OIDC_DB_HOST="db.example.com"
export OIDC_DB_PORT=5432
export OIDC_DB_NAME="hydrogen_oidc"
export OIDC_DB_USER="hydrogen"
export OIDC_DB_PASS="strong_password"
```

## Connection Management

### Connection Pooling
- Maintains connection pools
- Handles connection lifecycle
- Manages pool size
- Implements connection timeouts

### Health Checks
- Monitors connection status
- Performs periodic checks
- Handles reconnection
- Reports connection health

### Load Balancing
- Distributes database load
- Manages read replicas
- Handles failover
- Optimizes query routing

## Security Considerations

1. **Authentication**:
   - Use strong passwords
   - Store credentials securely
   - Use environment variables
   - Rotate credentials regularly

2. **Network Security**:
   - Use SSL/TLS connections
   - Configure firewall rules
   - Restrict network access
   - Monitor connections

3. **Access Control**:
   - Use least privilege accounts
   - Implement role-based access
   - Audit database access
   - Regular permission review

## Best Practices

1. **Connection Management**:
   - Set appropriate pool sizes
   - Configure timeouts
   - Monitor connection usage
   - Handle connection errors

2. **Performance**:
   - Optimize worker count
   - Monitor query performance
   - Configure statement cache
   - Use connection pooling

3. **Maintenance**:
   - Regular backups
   - Monitor disk space
   - Update statistics
   - Schedule maintenance

## Troubleshooting

### Common Issues

1. **Connection Problems**:
   - Check credentials
   - Verify network access
   - Test database status
   - Check SSL settings

2. **Performance Issues**:
   - Review pool settings
   - Check query performance
   - Monitor system resources
   - Analyze connection usage

3. **Configuration Problems**:
   - Verify environment variables
   - Check connection strings
   - Validate SSL certificates
   - Review worker settings

### Diagnostic Steps

1. Test database connection:
```bash
psql -h $OIDC_DB_HOST -U $OIDC_DB_USER -d $OIDC_DB_NAME
```

2. Check connection status:
```bash
curl http://your-printer:5000/api/system/db/status
```

3. Monitor connections:
```bash
curl http://your-printer:5000/api/system/db/connections
```

## Related Documentation

- [OIDC Configuration](oidc_configuration.md) - OIDC setup
- [Logging Configuration](logging_configuration.md) - Logging setup
- [Security Configuration](security_configuration.md) - Security settings
- [Backup Guide](../backup.md) - Database backup procedures