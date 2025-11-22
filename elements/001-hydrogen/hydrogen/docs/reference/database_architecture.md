# Database Subsystem Architecture

The Database subsystem provides a robust, multi-database management system with connection pooling, worker thread management, and environment-based configuration for Hydrogen's various database needs.

## System Overview

```diagram
┌───────────────────────────────────────────────────────────────┐
│                     Database System                           │
│                                                               │
│   ┌───────────────┐         ┌───────────────┐                 │
│   │   Service     │         │   Database    │                 │
│   │   Components  │◄────────┤   Manager     │                 │
│   └───────────────┘         └───────┬───────┘                 │
│                                     │                         │
│                                     ▼                         │
│   ┌───────────────┐         ┌───────────────┐                 │
│   │   Connection  │         │    Worker     │                 │
│   │     Pool      │◄────────┤    Threads    │                 │
│   └───────────────┘         └───────┬───────┘                 │
│           │                         │                         │
│           ▼                         ▼                         │
│   ┌───────────────┐         ┌───────────────┐                 │
│   │    OIDC DB    │         │   Acuranzo    │                 │
│   │   (Postgres)  │         │      DB       │                 │
│   └───────────────┘         └───────────────┘                 │
│                                                               │
│   ┌───────────────┐         ┌───────────────┐                 │
│   │   Helium DB   │         │   Canvas DB   │                 │
│   │   (Postgres)  │         │   (Postgres)  │                 │
│   └───────────────┘         └───────────────┘                 │
│                                                               │
└───────────────────────────────────────────────────────────────┘
```

## Key Components

### Database Manager

- Connection pool management
- Worker thread coordination
- Configuration handling
- Error management

### Connection Pool

- Connection lifecycle management
- Connection reuse optimization
- Pool size management
- Health monitoring

### Worker Threads

- Query execution
- Transaction management
- Resource cleanup
- Error handling

### Database Instances

- OIDC: Authentication and authorization data
- Acuranzo: Application-specific data
- Helium: System management data
- Canvas: User interface data

## Configuration

The Database subsystem is configured through the following settings in hydrogen.json:

```json
{
    "Databases": {
        "Workers": 1,
        "Connections": {
            "OIDC": {
                "Type": "postgres",
                "Host": "{$env.OIDC_DB_HOST}",
                "Port": "{$env.OIDC_DB_PORT}",
                "Database": "{$env.OIDC_DB_NAME}",
                "Username": "{$env.OIDC_DB_USER}",
                "Password": "{$env.OIDC_DB_PASS}",
                "MaxConnections": 10
            },
            "Acuranzo": {
                "Type": "postgres",
                "Host": "{$env.ACURANZO_DB_HOST}",
                "Port": "{$env.ACURANZO_DB_PORT}",
                "Database": "{$env.ACURANZO_DB_NAME}",
                "Username": "{$env.ACURANZO_DB_USER}",
                "Password": "{$env.ACURANZO_DB_PASS}",
                "MaxConnections": 10
            },
            "Helium": {
                "Type": "postgres",
                "Host": "{$env.HELIUM_DB_HOST}",
                "Port": "{$env.HELIUM_DB_PORT}",
                "Database": "{$env.HELIUM_DB_NAME}",
                "Username": "{$env.HELIUM_DB_USER}",
                "Password": "{$env.HELIUM_DB_PASS}",
                "MaxConnections": 10
            },
            "Canvas": {
                "Type": "postgres",
                "Host": "{$env.CANVAS_DB_HOST}",
                "Port": "{$env.CANVAS_DB_PORT}",
                "Database": "{$env.CANVAS_DB_NAME}",
                "Username": "{$env.CANVAS_DB_USER}",
                "Password": "{$env.CANVAS_DB_PASS}",
                "MaxConnections": 10
            }
        }
    }
}
```

## Security Considerations

1. **Connection Security**
   - TLS encryption for all connections (see [network_architecture.md](./network_architecture.md))
   - Certificate validation using system CA store
   - Connection timeouts with configurable thresholds
   - Secure credential handling as documented in [SECRETS.md](../SECRETS.md)

2. **Access Control**
   - Role-based database access with least privilege principle
   - Schema-level permissions for service isolation
   - Connection pooling limits to prevent resource exhaustion
   - Query restrictions based on service roles

3. **Data Protection**
   - Environment variable secrets management (see [SECRETS.md](../SECRETS.md))
   - AES-256 encrypted credentials in transit
   - Secure connection strings using environment variables
   - Sensitive data handling with encryption at rest

## Connection Lifecycle

```sequence
Service->Database Manager: Request connection
Database Manager->Connection Pool: Get connection
Connection Pool->Database: Create/reuse connection
Database->Connection Pool: Connection ready
Connection Pool->Database Manager: Connection available
Database Manager->Service: Connection provided
Service->Database Manager: Release connection
Database Manager->Connection Pool: Return connection
```

### Connection States

1. **Initialization**
   - Pool creation
   - Initial connections
   - Configuration loading
   - Health checks

2. **Active**
   - Query execution
   - Transaction management
   - Connection monitoring
   - Pool maintenance

3. **Termination**
   - Transaction cleanup
   - Connection closure
   - Resource release
   - Pool shutdown

## Error Handling

1. **Connection Errors**
   - Automatic retry
   - Connection recovery
   - Pool rebalancing
   - Error reporting

2. **Query Errors**
   - Transaction rollback
   - Error propagation
   - State recovery
   - Client notification

3. **Resource Errors**
   - Pool resizing
   - Connection cleanup
   - Worker management
   - Resource monitoring

## Integration Points

### Service Components

- Query requests
- Transaction management
- Error handling
- Connection lifecycle

### Worker Thread Handling

- Query execution
- Connection management
- Resource allocation
- Error handling

### Monitoring System

- Connection metrics
- Performance monitoring
- Resource utilization
- Error tracking

## Performance Considerations

1. **Connection Management**
   - Pool size optimization
   - Connection reuse
   - Query caching
   - Load balancing

2. **Resource Utilization**
   - Worker thread allocation
   - Memory management
   - CPU utilization
   - I/O optimization

3. **Query Optimization**
   - Query planning
   - Index utilization
   - Transaction management
   - Cache efficiency

## Testing

1. **Unit Tests**
   - Connection management
   - Pool operations
   - Query execution
   - Error handling

2. **Integration Tests**
   - Multi-database operations
   - Transaction management
   - Error recovery
   - Resource cleanup

3. **Performance Tests**
   - Connection pooling
   - Query throughput
   - Resource utilization
   - Stress testing

## Future Considerations

1. **Feature Enhancements**
   - Additional database types
   - Advanced pooling strategies
   - Query optimization
   - Monitoring improvements

2. **Security Improvements**
   - Enhanced encryption
   - Access control
   - Audit logging
   - Threat detection

3. **Performance Optimization**
   - Connection management
   - Query execution
   - Resource utilization
   - Caching strategies
