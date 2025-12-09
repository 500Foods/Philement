# Hydrogen System Architecture

This document provides a high-level overview of the Hydrogen system architecture, illustrating how the various components interact to form a complete system.

## System Overview

Hydrogen is designed as a modular, multi-layered architecture that prioritizes performance, scalability, and reliability. The system follows a service-oriented design where specialized components handle specific responsibilities while communicating through well-defined interfaces.

```diagram
┌───────────────────────────────────────────────────────────────┐
│                     Application Layer                         │
│                                                               │
│   ┌───────────────┐   ┌───────────────┐   ┌───────────────┐   │
│   │   Print Queue │   │  Web Server   │   │   WebSocket   │   │
│   │    Manager    │   │               │   │    Server     │   │
│   └───────┬───────┘   └───────┬───────┘   └───────┬───────┘   │
│                                                               │
│   ┌───────────────┐   ┌───────────────┐   ┌───────────────┐   │
│   │   Terminal    │   │   Database    │   │   SMTPRelay   │   │
│   │    Server     │   │    Manager    │   │    Server     │   │
│   └───────┬───────┘   └───────┬───────┘   └───────┬───────┘   │
│                                                               │
│   ┌───────────────┐                                           │
│   │   Swagger     │                                           │
│   │      UI       │                                           │
│   └───────┬───────┘                                           │
│                                                               │
│           │                   │                   │           │
│           │                   │                   │           │
│           ▼                   ▼                   ▼           │
│   ┌─────────────────────────────────────────────────────┐     │
│   │                   Core Services                     │     │
│   │                                                     │     │
│   │   ┌───────────┐   ┌───────────┐   ┌───────────┐     │     │
│   │   │  Queue    │   │  mDNS     │   │  System   │     │     │
│   │   │  System   │   │  Server   │   │  Service  │     │     │
│   │   └───────────┘   └─────┬─────┘   └───────────┘     │     │
│   │                         │                           │     │
│   │                   ┌─────┴─────┐                     │     │
│   │                   │   mDNS    │                     │     │
│   │                   │  Client   │                     │     │
│   │                   └───────────┘                     │     │
│   │                                                     │     │
│   └─────────────────────────────────────────────────────┘     │
│                             │                                 │
│                             │                                 │
│                             ▼                                 │
│   ┌─────────────────────────────────────────────────────┐     │
│   │                  System Foundation                  │     │
│   │                                                     │     │
│   │   ┌───────────┐   ┌───────────┐   ┌───────────┐     │     │
│   │   │ Logging   │   │  State    │   │  Network  │     │     │
│   │   │ System    │   │ Management│   │ Interface │     │     │
│   │   └───────────┘   └───────────┘   └───────────┘     │     │
│   │                                                     │     │
│   └─────────────────────────────────────────────────────┘     │
│                                                               │
└───────────────────────────────────────────────────────────────┘
```

## Architectural Layers

Hydrogen's architecture is organized into three primary layers, each with distinct responsibilities:

### 1. System Foundation Layer

The foundation layer provides core infrastructure and utilities used by all other components:

- **Logging System**: Centralized logging with configurable output targets and severity levels
- **State Management**: Global state tracking and lifecycle coordination
- **Network Interface**: Platform-abstracted network connectivity and interface management
- **Configuration System**: JSON-based configuration with validation and defaults
- **Utilities**: Common functions for time, threads, queue management, etc.

This layer establishes the groundwork for all higher-level services, providing essential capabilities while abstracting platform-specific details.

### 2. Core Services Layer

The core services layer implements fundamental system capabilities:

- **Queue System**: Generic, thread-safe queue implementation for message passing
- **mDNS Server**: Service advertising and discovery on local networks
- **System Service**: System information and health monitoring

These services act as building blocks for application-level features, providing reusable functionality with clean APIs.

### 3. Application Layer

The application layer delivers end-user functionality through specialized components:

- **Print Queue Manager**: 3D print job scheduling and management
- **Web Server**: HTTP REST API and static content delivery
- **WebSocket Server**: Real-time bidirectional communication

These components implement the primary user-facing features of Hydrogen, leveraging the lower layers to deliver their functionality.

## Key System Components

### Print Queue System

```diagram
┌───────────────┐         ┌───────────────┐         ┌───────────────┐
│  Web Server   │         │  Print Queue  │         │  Printer      │
│  (REST API)   │────────►│   Manager     │────────►│  Controller   │
└───────────────┘         └───────┬───────┘         └───────────────┘
                                  │
                                  │
                                  ▼
                          ┌───────────────┐
                          │   Beryllium   │
                          │  (G-code)     │
                          └───────────────┘
```

The [Print Queue Architecture](/docs/H/core/reference/print_queue_architecture.md) document provides detailed information about this component.

### WebSocket Communication

```diagram
┌───────────────┐         ┌───────────────┐         ┌───────────────┐
│    Client     │◄────┬───┤   WebSocket   │─────────┤ System Events │
│  Applications │     │   │    Server     │         └───────────────┘
└───────────────┘     │   └───────┬───────┘
                      │           │
                      │           ▼
                      │   ┌───────────────┐
                      └───┤  Message      │
                          │  Dispatcher   │
                          └───────────────┘
```

The [WebSocket Server Architecture](/docs/H/core/reference/websocket_architecture.md) document provides detailed information about this component.

### Network Management

```diagram
┌───────────────┐         ┌───────────────┐         ┌───────────────┐
│ mDNS Server   │         │   Network     │         │  WebSocket/   │
│               │◄────────┤   Interface   │────────►│  HTTP Servers │
└───────────────┘         └───────┬───────┘         └───────────────┘
                                  │
                                  ▼
                          ┌───────────────┐
                          │ Platform      │
                          │ Specific Impl │
                          └───────────────┘
```

The [Network Interface Architecture](/docs/H/core/reference/network_architecture.md) document provides detailed information about this component.

### Terminal System

```diagram
┌───────────────┐         ┌───────────────┐         ┌───────────────┐
│  Web Server   │         │   Terminal    │         │    PTY        │
│  (xterm.js)   │────────►│   Server      │────────►│   Session     │
└───────────────┘         └───────┬───────┘         └───────────────┘
                                  │
                                  │
                                  ▼
                          ┌───────────────┐
                          │   WebSocket   │
                          │   Server      │
                          └───────────────┘
```

The Terminal subsystem provides web-based terminal access through:

- xterm.js frontend served by the web server
- WebSocket-based bidirectional communication
- PTY session management for shell access
- Configurable session limits and timeouts

### Database Management

```diagram
┌───────────────┐         ┌───────────────┐         ┌───────────────┐
│   Service     │         │   Database    │         │   Postgres    │
│   Components  │────────►│   Manager     │────────►│   Databases   │
└───────────────┘         └───────┬───────┘         └───────────────┘
                                  │
                                  │
                                  ▼
                          ┌───────────────┐
                          │   Worker      │
                          │   Threads     │
                          └───────────────┘
```

The Database subsystem provides:

- Connection pooling for multiple databases
- Worker thread management for query processing
- Environment variable-based configuration
- Support for OIDC, Acuranzo, Helium, and Canvas databases

### SMTP Relay System

```diagram
┌───────────────┐         ┌───────────────┐         ┌───────────────┐
│   Inbound     │         │   SMTPRelay   │         │   Outbound    │
│   SMTP        │────────►│   Server      │────────►│   SMTP        │
└───────────────┘         └───────┬───────┘         └───────────────┘
                                  │
                                  │
                                  ▼
                          ┌───────────────┐
                          │   Message     │
                          │   Queue       │
                          └───────────────┘
```

The SMTP Relay subsystem provides:

- Inbound SMTP server on configurable port
- Message queuing with retry capabilities
- Multiple outbound SMTP server support
- Message content transformation

### Swagger Documentation

```diagram
┌───────────────┐         ┌───────────────┐         ┌───────────────┐
│   Web Server  │         │   Swagger     │         │   OpenAPI     │
│   (UI)        │────────►│   UI          │────────►│   Spec        │
└───────────────┘         └───────┬───────┘         └───────────────┘
                                  │
                                  │
                                  ▼
                          ┌───────────────┐
                          │   API         │
                          │   Explorer    │
                          └───────────────┘
```

The Swagger subsystem provides:

- OpenAPI 3.1 specification support
- Interactive API documentation
- Built-in API testing capabilities
- Configurable UI options and URL prefix

## Data Flow Patterns

Hydrogen implements several consistent data flow patterns across the system:

### 1. Command and Control Flow

```diagram
┌───────────┐     ┌───────────┐     ┌───────────┐     ┌───────────┐
│  Client   │────►│ Web/WS    │────►│ Service   │────►│ System    │
│ Request   │     │ Server    │     │ Component │     │ Resource  │
└───────────┘     └───────────┘     └───────────┘     └───────────┘
      ▲                                                     │
      │                                                     │
      │                                                     │
      └─────────────────────────────────────────────────────┘
                          Response
```

This pattern is used for handling user commands to control system behavior, such as adding print jobs, configuring services, or retrieving status information.

### 2. Event Notification Flow

```diagram
┌───────────┐     ┌───────────┐     ┌───────────┐     ┌───────────┐
│ System    │────►│ Event     │────►│ WebSocket │────►│ Client    │
│ Component │     │ Generator │     │ Server    │     │ Subscriber│
└───────────┘     └───────────┘     └───────────┘     └───────────┘
```

This pattern is used for real-time notifications of system events, such as print progress updates, status changes, or error conditions.

### 3. Service Discovery Flow

```diagram
┌───────────┐     ┌───────────┐     ┌───────────┐
│ System    │────►│  mDNS     │────►│ Network   │
│ Services  │     │ Publisher │     │           │
└───────────┘     └───────────┘     └───────────┘
                                          │
                                          │
                                          ▼
┌───────────┐     ┌───────────┐     ┌───────────┐
│ Client    │◄────┤ Service   │◄────┤ mDNS      │
│ Device    │     │ Discovery │     │ Responder │
└───────────┘     └───────────┘     └───────────┘
```

This pattern enables automatic discovery of Hydrogen instances on the local network, allowing clients to connect without manual configuration.

## Thread Model

Hydrogen uses a multi-threaded architecture with the following key thread groups:

```diagram
┌─────────────────────────────────────────────────────┐
│ Main Thread                                         │
│ ┌───────────────────────────────────────────────┐   │
│ │ Initialization / Shutdown Coordination        │   │
│ └───────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────┘

┌─────────────────────┐ ┌─────────────────────┐ ┌─────────────────────┐
│ Web Server Thread   │ │ WebSocket Thread    │ │ mDNS Server Thread  │
│ ┌─────────────────┐ │ │ ┌─────────────────┐ │ │ ┌─────────────────┐ │
│ │ HTTP Request    │ │ │ │ WebSocket       │ │ │ │ mDNS Service    │ │
│ │ Processing      │ │ │ │ Communication   │ │ │ │ Advertisement   │ │
│ └─────────────────┘ │ │ └─────────────────┘ │ │ └─────────────────┘ │
└─────────────────────┘ └─────────────────────┘ └─────────────────────┘

┌─────────────────────┐ ┌─────────────────────┐ ┌─────────────────────┐
│ Print Queue Thread  │ │ Logging Thread      │ │ Worker Threads      │
│ ┌─────────────────┐ │ │ ┌─────────────────┐ │ │ ┌─────────────────┐ │
│ │ Job Scheduling  │ │ │ │ Log Message     │ │ │ │ Task-specific   │ │
│ │ & Management    │ │ │ │ Processing      │ │ │ │ Processing      │ │
│ └─────────────────┘ │ │ └─────────────────┘ │ │ └─────────────────┘ │
└─────────────────────┘ └─────────────────────┘ └─────────────────────┘
```

Each major component runs in its own thread, with the main thread responsible for initialization, coordination, and shutdown sequencing. This approach provides:

1. **Isolation**: Component failures don't directly impact other parts of the system
2. **Parallelism**: System resources are utilized efficiently
3. **Responsiveness**: Long-running operations don't block the entire system
4. **Scheduling**: Components can operate at different priorities and frequencies

See the [Thread Monitoring](/docs/H/core/reference/thread_monitoring.md) document for details on monitoring thread health.

## Startup Sequence

The Hydrogen startup sequence follows a careful, dependency-aware ordering:

```diagram
┌─────────────┐
│  Start Main │
└──────┬──────┘
       │
       ▼
┌─────────────┐      ┌─────────────┐
│  Parse      │─────►│  Load       │
│  Arguments  │      │  Config     │
└──────┬──────┘      └──────┬──────┘
       │                    │
       ▼                    ▼
┌─────────────┐      ┌─────────────┐
│  Initialize │◄─────┤  Configure  │
│  Logging    │      │  Components │
└──────┬──────┘      └──────┬──────┘
       │                    │
       ▼                    │
┌─────────────┐             │
│  Initialize │◄────────────┘
│  State      │
└──────┬──────┘
       │
       ▼
┌─────────────┐      ┌─────────────┐      ┌─────────────┐
│  Start Log  │─────►│ Start Core  │─────►│ Start mDNS  │
│  Thread     │      │ Services    │      │ Server      │
└──────┬──────┘      └──────┬──────┘      └──────┬──────┘
       │                    │                    │
       │                    ▼                    │
       │            ┌─────────────┐              │
       └────────────┤ Start Print │◄─────────────┘
                    │ Queue       │
                    └──────┬──────┘
                           │
                           ▼
                    ┌─────────────┐      ┌─────────────┐
                    │ Start Web   │─────►│ Start       │
                    │ Server      │      │ WebSocket   │
                    └──────┬──────┘      └──────┬──────┘
                           │                    │
                           ▼                    ▼
                    ┌─────────────────────────────┐
                    │      Enter Main Loop        │
                    └─────────────────────────────┘
```

This sequence ensures that each component has its dependencies available before initialization. For example:

- The logging system must be available before any component that produces logs
- Core services must be running before components that depend on them
- The WebSocket server depends on the web server for its HTTP upgrade mechanism

## Shutdown Architecture

Hydrogen implements a carefully orchestrated shutdown sequence to ensure resources are released properly:

```diagram
┌─────────────┐
│  Signal     │
│  Handler    │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  Set        │
│  Shutdown   │
│  Flags      │
└──────┬──────┘
       │
       ▼
┌─────────────┐      ┌─────────────┐
│  Signal     │─────►│  Wait for   │
│  Threads    │      │  Thread     │
└──────┬──────┘      │  Completion │
       │             └──────┬──────┘
       │                    │
       ▼                    ▼
┌─────────────┐      ┌─────────────┐      ┌─────────────┐
│  Close      │─────►│  Close      │─────►│  Free       │
│  Connections│      │  Files      │      │  Resources  │
└─────────────┘      └─────────────┘      └─────────────┘
```

The shutdown sequence follows the reverse of the startup sequence, ensuring dependencies are respected. For example:

- Application services are stopped before core services
- Network connections are closed before network resources are freed
- Log system is the last to shut down

See the [Shutdown Architecture](/docs/H/core/shutdown_architecture.md) document for detailed information about this process.

## Key Architectural Patterns

Hydrogen employs several consistent architectural patterns across components:

### 1. Subsystem Registry

The Subsystem Registry provides a centralized mechanism for managing the lifecycle and dependencies of all components:

```diagram
┌───────────────────────────────────────────────────────────────┐
│                   Subsystem Registry                          │
│                                                               │
│   ┌───────────────┐   ┌───────────────┐   ┌───────────────┐   │
│   │   Subsystem   │   │   Subsystem   │   │   Subsystem   │   │
│   │   Info #1     │   │   Info #2     │   │   Info #3     │   │
│   └───────────────┘   └───────────────┘   └───────────────┘   │
│                                                               │
│   ┌─────────────────────────────────────────────────────┐     │
│   │              Registry Operations                     │     │
│   │  • Register Subsystem                               │     │
│   │  • Start/Stop Subsystem                             │     │
│   │  • Manage Dependencies                              │     │
│   │  • Track State Changes                              │     │
│   └─────────────────────────────────────────────────────┘     │
└───────────────────────────────────────────────────────────────┘
```

This pattern enables:

- Centralized subsystem lifecycle management
- Dependency-aware startup and shutdown sequences
- Runtime monitoring of subsystem health
- Structured status reporting

See the [Subsystem Registry Architecture](/docs/H/core/reference/subsystem_registry_architecture.md) document for detailed information.

### 2. Thread-Safe Queues

Queue-based communication is used extensively to decouple components and provide thread safety:

```diagram
┌───────────┐      ┌───────────┐      ┌───────────┐
│ Producer  │─────►│ Thread-   │─────►│ Consumer  │
│ Thread    │      │ Safe Queue│      │ Thread    │
└───────────┘      └───────────┘      └───────────┘
```

This pattern is implemented in:

- Log message processing
- Print job management
- WebSocket message delivery

### 2. Service Manager Pattern

Components follow a consistent service manager pattern:

```diagram
┌─────────────────────────────────────────────────┐
│ Service Manager                                 │
│                                                 │
│  ┌────────────┐     ┌────────────┐              │
│  │ Initialize │     │ Shutdown   │              │
│  └────────────┘     └────────────┘              │
│                                                 │
│  ┌────────────┐     ┌────────────┐              │
│  │ Worker     │     │ Resource   │              │
│  │ Thread     │     │ Management │              │
│  └────────────┘     └────────────┘              │
│                                                 │
└─────────────────────────────────────────────────┘
```

This pattern provides consistent:

- Initialization/shutdown sequences
- Thread management
- Resource tracking
- Error handling

### 3. Configuration-Driven Behavior

Components are configured through a consistent JSON configuration structure:

```diagram
┌───────────┐      ┌───────────┐      ┌───────────┐
│ JSON      │─────►│ Config    │─────►│ Component │
│ Config    │      │ Validator │      │ Behavior  │
└───────────┘      └───────────┘      └───────────┘
```

This pattern enables:

- Runtime behavior customization
- Consistent validation
- Default values
- Per-component configuration sections

### 4. Platform Abstraction

Platform-specific code is isolated behind abstraction layers:

```diagram
┌───────────┐      ┌─────────────┐      ┌───────────┐
│ Component │─────►│ Abstraction │─────►│ Platform  │
│ Logic     │      │ Layer       │      │ Specific  │
└───────────┘      └─────────────┘      └───────────┘
```

This pattern facilitates:

- Cross-platform compatibility
- Testability through mock implementations
- Clear separation of concerns

## Service Dependencies

Hydrogen services have the following dependency relationships:

```diagram
                      ┌───────────┐
                      │ Logging   │
                      │ System    │
                      └─────┬─────┘
                            │
                            ▼
┌───────────┐      ┌───────────┐      ┌───────────┐
│ State     │◄─────┤ Core      │─────►│ Queue     │
│ Management│      │ Services  │      │ System    │
└───────────┘      └─────┬─────┘      └───────────┘
                         │
                         ▼
            ┌─────────────────────────┐
            │                         │
            ▼                         ▼
┌───────────┐               ┌───────────┐
│ Network   │──────────────►│ mDNS      │
│ Interface │               │ Server    │
└─────┬─────┘               └───────────┘
      │
      │
      ▼
┌───────────┐      ┌───────────┐
│ Web       │─────►│ WebSocket │
│ Server    │      │ Server    │
└─────┬─────┘      └───────────┘
      │
      ▼
┌───────────┐
│ Print     │
│ Queue     │
└───────────┘
```

This dependency graph guides:

- Initialization order during startup
- Shutdown sequence
- Error propagation
- Resource allocation

## Security Model

Hydrogen implements a comprehensive security model with multiple layers of protection. For detailed implementation of encryption and secrets management, see [SECRETS.md](/docs/H/SECRETS.md).

```diagram
┌───────────────────────────────────────────────────────────────┐
│ Transport Security                                            │
│ - TLS encryption with modern cipher suites                    │
│ - Certificate validation with system CA store                 │
│ - Secure WebSocket protocol (wss://)                          │
└───────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌───────────────────────────────────────────────────────────────┐
│ Encryption Layer                                              │
│ - RSA+AES hybrid encryption for payloads                      │
│ - Environment-based key management                            │
│ - Secure credential storage                                   │
└───────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌───────────────────────────────────────────────────────────────┐
│ Authentication & Authorization                                │
│ - OpenID Connect integration                                  │
│ - RSA-signed JWT tokens                                       │
│ - Role-based access control                                   │
└───────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌───────────────────────────────────────────────────────────────┐
│ Data Protection                                               │
│ - Input validation and sanitization                           │
│ - AES-256 encryption for sensitive data                       │
│ - Secure configuration management                             │
└───────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌───────────────────────────────────────────────────────────────┐
│ Resource Protection                                           │
│ - Rate limiting and abuse prevention                          │
│ - Session management                                          │
│ - Audit logging                                               │
└───────────────────────────────────────────────────────────────┘
```

Each security layer is documented in detail:

- [Network Security](/docs/H/core/reference/network_architecture.md)
- [Payload Encryption](/elements/001-hydrogen/hydrogen/payloads/README.md)
- [OIDC Security](/docs/H/core/reference/oidc_architecture.md)
- [Database Security](/docs/H/core/reference/database_architecture.md)
- [SMTP Security](/docs/H/core/reference/smtp_relay_architecture.md)

## AI Integration

AI capabilities are integrated throughout the Hydrogen architecture:

```diagram
┌───────────────────────────────────────────────────────────────┐
│                     Application Layer                         │
│                                                               │
│   ┌───────────┐   ┌───────────┐   ┌───────────┐   ┌────────┐  │
│   │ Print     │   │ Web       │   │ WebSocket │   │ AI     │  │
│   │ Queue     │   │ Server    │   │ Server    │   │ Engine │  │
│   └───────────┘   └───────────┘   └───────────┘   └────────┘  │
│         │               │               │              │      │
│         └───────────────┼───────────────┼──────────────┘      │
│                         │               │                     │
└─────────────────────────┼───────────────┼─────────────────────┘
                          │               │
                ┌─────────┴───────────────┴─────────┐
                │         AI Services Layer         │
                │  ┌─────────────────────────────┐  │
                │  │ - Print Analysis            │  │
                │  │ - Prediction Models         │  │
                │  │ - Optimization Algorithms   │  │
                │  │ - Natural Language          │  │
                │  └─────────────────────────────┘  │
                └───────────────────────────────────┘
```

See the [AI Integration](/docs/H/core/ai_integration.md) document for detailed information about AI capabilities.

## Component Interaction Examples

### Print Job Submission and Processing

```sequence
Client->Web Server: POST /api/print/queue
Web Server->Print Queue: addJob(file, params)
Print Queue->Beryllium: analyzeFile(file)
Beryllium->Print Queue: analysisResult
Print Queue->WebSocket Server: job_added event
WebSocket Server->Client: job_added notification
Print Queue->Printer: startPrint(job)
```

### Service Discovery

```sequence
mDNS Server->Network: advertiseService("hydrogen")
Client->Network: browseLAN()
Network->Client: discovered("hydrogen")
Client->Web Server: HTTP connection
Client->WebSocket Server: WebSocket connection
```

### Real-time Status Updates

```sequence
System->WebSocket Server: statusChanged event
WebSocket Server->Client: status update
Client->WebSocket Server: acknowledgment
```

## Source Code Organization

The Hydrogen source code is organized in a modular structure that reflects the architecture:

```files
src/
├── api/             # API implementation
│   └── system/      # System API endpoints
├── config/          # Configuration management
├── logging/         # Logging system
├── mdns/            # mDNS server implementation
├── network/         # Network interface abstraction
├── print/           # Print queue management
├── queue/           # Generic queue implementation
├── state/           # State management
├── utils/           # Utility functions
├── webserver/       # Web server implementation
├── websocket/       # WebSocket server implementation
└── hydrogen.c       # Main entry point
```

## Related Documentation

For more detailed information about specific components, refer to these documents:

- [Print Queue Architecture](/docs/H/core/reference/print_queue_architecture.md) - Detailed print queue design
- [WebSocket Server Architecture](/docs/H/core/reference/websocket_architecture.md) - WebSocket implementation details
- [Network Interface Architecture](/docs/H/core/reference/network_architecture.md) - Network abstraction design
- [Shutdown Architecture](/docs/H/core/shutdown_architecture.md) - Shutdown process details
- [Data Structures](/docs/H/core/reference/data_structures.md) - Core data structures
- [Thread Monitoring](/docs/H/core/reference/thread_monitoring.md) - Thread health monitoring
- [AI Integration](/docs/H/core/ai_integration.md) - AI capabilities
