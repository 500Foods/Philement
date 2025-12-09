# SMTP Relay Subsystem Architecture

The SMTP Relay subsystem provides a sophisticated email processing system with queuing capabilities, content transformation, and multi-server outbound routing.

## System Overview

```diagram
┌───────────────────────────────────────────────────────────────┐
│                     SMTP Relay System                         │
│                                                               │
│   ┌───────────────┐         ┌───────────────┐                 │
│   │   Inbound     │         │   Message     │                 │
│   │   SMTP        │────────►│   Queue       │                 │
│   └───────────────┘         └───────┬───────┘                 │
│                                     │                         │
│                                     ▼                         │
│   ┌───────────────┐         ┌───────────────┐                 │
│   │   Content     │◄────────┤   Worker      │                 │
│   │   Transform   │         │   Threads     │                 │
│   └───────┬───────┘         └───────────────┘                 │
│           │                                                   │
│           ▼                                                   │
│   ┌───────────────┐         ┌───────────────┐                 │
│   │   Outbound    │         │   Outbound    │                 │
│   │   Server 1    │         │   Server 2    │                 │
│   └───────────────┘         └───────────────┘                 │
│                                                               │
└───────────────────────────────────────────────────────────────┘
```

## Key Components

### Inbound SMTP Server

- Message reception
- Protocol compliance
- Authentication handling
- Rate limiting

### Message Queue

- Message storage
- Retry management
- Priority handling
- Load balancing

### Content Transform

- Message modification
- Logo enhancement
- Content filtering
- Format conversion

### Outbound Routing

- Server selection
- Connection management
- Delivery tracking
- Error handling

## Configuration

The SMTP Relay subsystem is configured through the following settings in hydrogen.json:

```json
{
    "SMTPRelay": {
        "Enabled": true,
        "ListenPort": 587,
        "Workers": 2,
        "QueueSettings": {
            "MaxQueueSize": 1000,
            "RetryAttempts": 3,
            "RetryDelaySeconds": 300
        },
        "OutboundServers": [
            {
                "Host": "{$env.SMTP_SERVER1_HOST}",
                "Port": "{$env.SMTP_SERVER1_PORT}",
                "Username": "{$env.SMTP_SERVER1_USER}",
                "Password": "{$env.SMTP_SERVER1_PASS}",
                "UseTLS": true
            },
            {
                "Host": "{$env.SMTP_SERVER2_HOST}",
                "Port": "{$env.SMTP_SERVER2_PORT}",
                "Username": "{$env.SMTP_SERVER2_USER}",
                "Password": "{$env.SMTP_SERVER2_PASS}",
                "UseTLS": true
            }
        ]
    }
}
```

## Security Considerations

1. **Transport Security**
   - TLS encryption with modern cipher suites (see [network_architecture.md](network_architecture.md))
   - Certificate validation using system CA store
   - STARTTLS protocol compliance (RFC 3207)
   - Connection security with configurable timeouts

2. **Authentication**
   - Client authentication with SASL mechanisms
   - Server authentication via TLS certificates
   - Credential management via environment variables (see [SECRETS.md](/docs/H/SECRETS.md))
   - Role-based access control for relay permissions

3. **Content Security**
   - Message scanning for malicious content
   - Attachment filtering with configurable rules
   - Content validation against RFC standards
   - Real-time threat detection and blocking

## Message Flow

```sequence
Client->Inbound SMTP: Submit message
Inbound SMTP->Message Queue: Queue message
Message Queue->Worker Thread: Process message
Worker Thread->Content Transform: Modify content
Content Transform->Outbound Server: Send message
Outbound Server->Destination: Deliver message
```

### Message States

1. **Reception**
   - Connection acceptance
   - Protocol handling
   - Message validation
   - Initial processing

2. **Processing**
   - Queue management
   - Content transformation
   - Routing decisions
   - Retry handling

3. **Delivery**
   - Server selection
   - Connection management
   - Delivery attempt
   - Status tracking

## Error Handling

1. **Reception Errors**
   - Protocol violations
   - Authentication failures
   - Resource exhaustion
   - Invalid content

2. **Processing Errors**
   - Queue overflow
   - Transform failures
   - Resource constraints
   - Internal errors

3. **Delivery Errors**
   - Connection failures
   - Server rejections
   - Timeout handling
   - Retry management

## Integration Points

### Queue System

- Message storage
- Priority handling
- Retry management
- Status tracking

### Worker Threads

- Message processing
- Content transformation
- Delivery management
- Error handling

### Monitoring System

- Queue metrics
- Server status
- Performance monitoring
- Error tracking

## Performance Considerations

1. **Message Processing**
   - Queue optimization
   - Worker allocation
   - Transform efficiency
   - Delivery throughput

2. **Resource Management**
   - Memory utilization
   - Connection pooling
   - Thread allocation
   - Queue sizing

3. **Delivery Optimization**
   - Server selection
   - Connection reuse
   - Batch processing
   - Load balancing

## Testing

1. **Unit Tests**
   - Protocol handling
   - Queue operations
   - Content transformation
   - Error handling

2. **Integration Tests**
   - End-to-end flow
   - Server interaction
   - Error recovery
   - Performance metrics

3. **Load Tests**
   - Queue capacity
   - Processing throughput
   - Resource utilization
   - Error handling

## Future Considerations

1. **Feature Enhancements**
   - Additional transformations
   - Advanced routing
   - Enhanced monitoring
   - Extended protocols

2. **Security Improvements**
   - Enhanced encryption
   - Content scanning
   - Access control
   - Threat detection

3. **Performance Optimization**
   - Queue management
   - Processing efficiency
   - Delivery throughput
   - Resource utilization
