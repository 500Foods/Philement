# Hydrogen Documentation

Welcome to the Hydrogen documentation. This guide provides comprehensive information about configuring, using, and maintaining your Hydrogen 3D printer control server.

## Quick Start

- [Quick Start Guide](/docs/guides/quick-start.md) - Get up and running quickly
- [Configuration Guide](/docs/configuration.md) - Basic configuration concepts
- [API Documentation](/docs/api.md) - REST API reference

## Configuration Guides

The configuration system is divided into subsystems, each with its own detailed guide:

### Core Systems

- [Web Server Configuration](/docs/reference/webserver_configuration.md) - HTTP server settings
- [WebSocket Configuration](/docs/reference/websocket_configuration.md) - Real-time updates
- [Print Queue Configuration](/docs/reference/printqueue_configuration.md) - Job management
- [System Resources](/docs/reference/resources_configuration.md) - Resource management

### Network & Discovery

- [mDNS Configuration](/docs/reference/mdns_configuration.md) - Network discovery
- [Network Architecture](/docs/reference/network_architecture.md) - Network design

### Documentation & API

- [Swagger Configuration](/docs/reference/swagger_configuration.md) - API documentation
- [API Reference](/docs/reference/api.md) - API details
- [OpenAPI Specification](/docs/reference/swagger_architecture.md) - API architecture

### Logging & Monitoring

- [Logging Configuration](/docs/reference/logging_configuration.md) - Log management
- [Terminal Configuration](/docs/reference/terminal_configuration.md) - Web terminal
- [Monitoring Guide](/docs/reference/monitoring.md) - System monitoring

### Communication

- [SMTP Configuration](/docs/reference/smtp_configuration.md) - Email notifications
- [Database Configuration](/docs/reference/database_configuration.md) - Data storage

## Developer Guides

- [Developer Onboarding](/docs/developer_onboarding.md) - Getting started with development
- [Coding Guidelines](/docs/coding_guidelines.md) - Code standards and practices
- [Testing Guide](/docs/testing.md) - Testing procedures
- [Release Process](/RELEASES.md) - Release management

## Architecture Documentation

- [System Architecture](/docs/reference/system_architecture.md) - Overall system design
- [Subsystem Registry Architecture](/docs/reference/subsystem_registry_architecture.md) - Subsystem lifecycle management
- [Launch System Architecture](/docs/reference/launch_system_architecture.md) - Subsystem launch go/no-go process
- [Thread Monitoring](/docs/thread_monitoring.md) - Thread management
- [Shutdown Architecture](/docs/shutdown_architecture.md) - Shutdown process

### Subsystem Documentation

- [WebServer Subsystem](/docs/reference/webserver_subsystem.md) - Web server implementation details
- [WebSocket Subsystem](/docs/reference/websocket_subsystem.md) - WebSocket server implementation details
- [Print Subsystem](/docs/reference/print_subsystem.md) - Print queue implementation details
- [WebSocket Architecture](/docs/web_socket.md) - WebSocket system

## Security Documentation

- [Security Configuration](/docs/reference/security_configuration.md) - Security settings
- [OIDC Integration](/docs/oidc_integration.md) - Authentication
- [Secrets Management](/docs/SECRETS.md) - Managing sensitive data

## Use Cases

- [Home Workshop](/docs/guides/use-cases/home-workshop.md) - Single printer setup
- [Print Farm](/docs/guides/use-cases/print-farm.md) - Multiple printer management

## Additional Resources

- [FAQ](/docs/reference/faq.md) - Common questions
- [Troubleshooting](/docs/reference/troubleshooting.md) - Common issues
- [Glossary](/docs/reference/glossary.md) - Terms and definitions

## Getting Help

If you need assistance:

1. Check the [Troubleshooting Guide](/docs/reference/troubleshooting.md)
2. Search existing [Issues](https://github.com/philement/hydrogen/issues)
3. Join our [Community Forum](https://forum.philement.com)
4. Contact [Support](https://philement.com/support)

## License

Hydrogen is licensed under the MIT License. See the license file at the root of the project for details.
