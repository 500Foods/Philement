# Terminal Subsystem Architecture

The Terminal subsystem provides secure, web-based terminal access to the Hydrogen server through a combination of xterm.js frontend and PTY session management.

## System Overview

```diagram
┌───────────────────────────────────────────────────────────────┐
│                     Terminal System                           │
│                                                               │
│   ┌───────────────┐         ┌───────────────┐                 │
│   │   xterm.js    │         │   Terminal    │                 │
│   │   Frontend    │◄────────┤   Server      │                 │
│   └───────────────┘         └───────┬───────┘                 │
│           ▲                         │                         │
│           │                         ▼                         │
│   ┌───────────────┐         ┌───────────────┐                 │
│   │   WebSocket   │         │    PTY        │                 │
│   │   Server      │◄────────┤   Manager     │                 │
│   └───────────────┘         └───────────────┘                 │
│                                                               │
└───────────────────────────────────────────────────────────────┘
```

## Key Components

### Frontend (xterm.js)

- Web-based terminal emulator
- Full terminal feature support
- Configurable appearance and behavior
- WebSocket-based communication

### Terminal Server

- Session management and lifecycle control
- User authentication and authorization
- Resource limits enforcement
- Configuration handling

### PTY Manager

- PTY session creation and management
- Shell process spawning and control
- Input/output stream handling
- Resource cleanup

### WebSocket Integration

- Bidirectional real-time communication
- Binary data support for terminal I/O
- Session persistence
- Error handling

## Configuration

The Terminal subsystem is configured through the following settings in hydrogen.json:

```json
{
    "Terminal": {
        "Enabled": true,
        "WebPath": "/terminal",
        "ShellCommand": "/bin/bash",
        "MaxSessions": 4,
        "IdleTimeoutSeconds": 600,
        "BufferSize": 4096
    }
}
```

- **Enabled**: Enable/disable the terminal subsystem
- **WebPath**: URL path for accessing the terminal interface
- **ShellCommand**: Default shell to spawn for sessions
- **MaxSessions**: Maximum concurrent terminal sessions
- **IdleTimeoutSeconds**: Session timeout for inactive terminals
- **BufferSize**: I/O buffer size for terminal communication

## Security Considerations

1. **Authentication**
   - Required for all terminal sessions
   - Integration with OIDC system (see [oidc_architecture.md](/docs/H/core/reference/oidc_architecture.md))
   - RSA-signed session tokens with configurable expiration
   - Secure token storage (see [SECRETS.md](/docs/H/SECRETS.md))

2. **Authorization**
   - Role-based access control with least privilege principle
   - Command restrictions based on user roles
   - Environment isolation using PTY containment
   - Audit logging of all commands

3. **Resource Protection**
   - Session limits enforced per user and globally
   - Configurable idle timeout with automatic cleanup
   - Memory and CPU usage monitoring
   - Resource quota enforcement

4. **Data Security**
   - WebSocket TLS encryption with modern cipher suites (see [network_architecture.md](/docs/H/core/reference/network_architecture.md))
   - Input sanitization and command filtering
   - Output filtering for sensitive data
   - Environment variable protection (see [SECRETS.md](/docs/H/SECRETS.md))

## Session Lifecycle

```sequence
Client->Web Server: GET /terminal
Web Server->Client: xterm.js interface
Client->Terminal Server: WebSocket connection
Terminal Server->PTY Manager: Create session
PTY Manager->Shell: Spawn process
Shell->PTY Manager: I/O streams
PTY Manager->Terminal Server: Session ready
Terminal Server->Client: Connection established
```

### Session States

1. **Initialization**
   - Client requests terminal
   - Authentication check
   - Resource availability verification
   - PTY allocation

2. **Active**
   - I/O processing
   - Command execution
   - Window size handling
   - Keep-alive monitoring

3. **Termination**
   - Idle timeout
   - Client disconnect
   - Error condition
   - Resource cleanup

## Error Handling

1. **Connection Issues**
   - WebSocket reconnection
   - Session recovery
   - State synchronization

2. **Resource Exhaustion**
   - Graceful session limiting
   - Memory monitoring
   - Process cleanup

3. **Shell Errors**
   - Process exit handling
   - Error reporting
   - Session cleanup

## Integration Points

### Web Server

- Serves xterm.js frontend
- Handles initial HTTP requests
- Manages static assets
- Processes WebSocket upgrades

### WebSocket Server

- Maintains terminal sessions
- Handles binary data transfer
- Manages connection lifecycle
- Provides error handling

### Authentication System

- User verification
- Session validation
- Permission checking
- Access control

## Performance Considerations

1. **Resource Management**
   - Buffer size optimization
   - Session pooling
   - Memory usage monitoring
   - CPU utilization control

2. **Scalability**
   - Session limit enforcement
   - Load distribution
   - Resource allocation
   - Connection management

3. **Reliability**
   - Error recovery
   - Session persistence
   - State management
   - Cleanup procedures

## Testing

1. **Unit Tests**
   - PTY creation
   - Session management
   - I/O handling
   - Error conditions

2. **Integration Tests**
   - WebSocket communication
   - Authentication flow
   - Resource management
   - Error recovery

3. **Load Tests**
   - Multiple sessions
   - Resource utilization
   - Performance metrics
   - Stress conditions

## Future Considerations

1. **Feature Enhancements**
   - Session recording
   - Command restrictions
   - Multi-user sessions
   - Terminal sharing

2. **Security Improvements**
   - Enhanced auditing
   - Command filtering
   - Access controls
   - Session isolation

3. **Performance Optimization**
   - Buffer management
   - Resource utilization
   - Connection handling
   - State synchronization
