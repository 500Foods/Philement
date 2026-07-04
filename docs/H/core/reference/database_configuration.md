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

### Query Watchdog Settings

These settings configure the client-side query watchdog and the engine-agnostic retry layer that wraps every `database_engine_execute` call. They live at the top level of the `Databases` object, alongside `DefaultWorkers`. The full architecture is documented under [Query Watchdog and Transient Failure Recovery](/docs/H/core/subsystems/database/database.md#query-watchdog-and-transient-failure-recovery); this section is the config-only reference.

| Setting | Description | Default | Range | Notes |
|---------|-------------|---------|-------|-------|
| `BootstrapTimeoutSeconds` | Per-query timeout (seconds) for the bootstrap query and the orphan-DROP. Also used as the watchdog's default when a request's `timeout_seconds` is 0 or negative. | `30` | Clamped to `[WatchdogMinSeconds, WatchdogMaxSeconds]` | A 30s default is the original hard-coded value; production deployments against slow databases should raise this |
| `BootstrapRetries` | Number of additional attempts the engine abstraction makes on `DB_ERR_TRANSPORT` or `DB_ERR_TIMEOUT` before giving up. Each retry uses the full `BootstrapTimeoutSeconds` as its own budget; backoff between attempts is 1s, 2s, 4s, ... (capped at 30s). | `3` | `0` (no retry) or positive | 3 retries + 7s of backoff is the original motivation: survive a single transient hiccup without making the bootstrap take an unbounded amount of time |
| `WatchdogMinSeconds` | Lower bound (seconds) for any `QueryRequest.timeout_seconds` the watchdog enforces. Values below this are clamped up. | `30` | positive integer | A misconfigured 1s timeout would cause the watchdog to log ALERTs on every legitimate query; this floor prevents that |
| `WatchdogMaxSeconds` | Upper bound (seconds) for any `QueryRequest.timeout_seconds`. Values above this are clamped down. | `3600` | positive integer, must be `>= WatchdogMinSeconds` | An hour is the practical ceiling - a hung query held in the registry longer than this is taking up operator attention without anyone being able to act on the ALERTs |

The watchdog's startup log line reports the effective values that will be used for every request, so a misconfiguration in `hydrogen.json` is immediately visible:

```
[SR-DATABASE] Database query watchdog initialized (min=30s, max=3600s, default=30s, heartbeat=30s)
```

#### Example: tuning for a slow production database

```json
{
    "Databases": {
        "DefaultWorkers": 2,
        "BootstrapTimeoutSeconds": 120,
        "BootstrapRetries": 5,
        "WatchdogMinSeconds": 60,
        "WatchdogMaxSeconds": 1800,
        "Connections": {
            "Production": {
                "Enabled": true,
                "Type": "postgresql",
                "Host": "${env.PG_HOST}",
                "Port": 5432,
                "Database": "${env.PG_DB}",
                "User": "${env.PG_USER}",
                "Pass": "${env.PG_PASS}",
                "Workers": 4
            }
        }
    }
}
```

This raises the per-query budget to two minutes, allows up to 5 retries on transient hiccups, and tightens the watchdog floor to one minute (so a long-running legitimate query doesn't accidentally cross the threshold and trigger a cancel).

#### What these knobs do not control

- **Per-query timeouts for normal Conduit / migration / health-check queries** — those still use the value in `QueryRequest.timeout_seconds` (the watchdog clamps it to the configured `[min, max]`). If you need different timeouts for different query types, set them in the request that submits the query.
- **Server-side timeouts** — engines still issue `SET statement_timeout = N` on the connection, and engines like SQLite honor `sqlite3_busy_timeout`. The watchdog is the *client-side* counterpart; both layers coexist.
- **The cancel mechanism itself** — that is per-engine and is enabled automatically when the underlying client library is loaded successfully. If `dlsym` fails to find `PQcancel` / `mysql_kill` / `sqlite3_interrupt` / `SQLCancel`, the watchdog logs a one-time `ALERT` at startup naming the missing function.

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
