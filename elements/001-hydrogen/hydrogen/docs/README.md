# Hydrogen Documentation

Welcome to the Hydrogen Project documentation. This guide will help you understand, configure, and use the Hydrogen 3D printer control server.

## Getting Started

- [Quick Start Guide](./guides/quick-start.md) - Get up and running quickly
- [Configuration Guide](./reference/configuration.md) - Essential configuration
- [Thread Monitoring](./thread_monitoring.md) - Performance monitoring

## Core Documentation

### API & Interfaces

- [API Documentation](./api.md) - Complete API reference
- [WebSocket Interface](./web_socket.md) - Real-time communications
- [mDNS Server](./mdns_server.md) - Network service discovery
- [System API](./api/system/system_info.md) - System endpoints

### Features & Components

- [Print Queue Management](./print_queue.md) - Managing print jobs
- [Data Structures](./reference/data_structures.md) - Core data types
- [Service Management](./service.md) - Service configuration
- [Shutdown Architecture](./shutdown_architecture.md) - Safe system termination
- [AI Integration](./ai_integration.md) - AI capabilities and implementations

### Use Cases

- [Home Workshop](./guides/use-cases/home-workshop.md) - Single printer setup
- [Print Farm](./guides/use-cases/print-farm.md) - Multiple printer management

## Development

- [System Architecture](./reference/system_architecture.md) - High-level architecture of the entire system
- [Coding Guidelines](./coding_guidelines.md) - Development standards
- [Testing System](./testing.md) - Test configuration and execution
- [Docker Deployment](./deployment/docker.md) - Container-based deployment
- [AI in Development](./development/ai_development.md) - How AI is used in the project's development

### Component Architecture

- [Print Queue Architecture](./reference/print_queue_architecture.md) - Detailed internal design of the print queue system
- [WebSocket Server Architecture](./reference/websocket_architecture.md) - Implementation details of the WebSocket communication layer
- [Network Interface Architecture](./reference/network_architecture.md) - Network abstraction layer design and implementation

## Contributing

When adding new features to Hydrogen, please:

1. Update relevant guides and reference documentation
2. Add examples for new functionality
3. Include configuration documentation
4. Update API documentation if applicable

## Documentation Updates

This documentation is actively maintained. For the most up-to-date information:

1. Check the [Configuration Guide](./reference/configuration.md)
2. Review the [API Documentation](./api.md)
3. See the [Quick Start Guide](./guides/quick-start.md) for getting started
