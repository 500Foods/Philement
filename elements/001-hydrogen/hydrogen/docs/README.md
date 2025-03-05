# Hydrogen Documentation

Welcome to the Hydrogen documentation. This guide provides comprehensive information about configuring, using, and maintaining your Hydrogen 3D printer control server.

## Quick Start

- [Quick Start Guide](guides/quick-start.md) - Get up and running quickly
- [Configuration Guide](configuration.md) - Basic configuration concepts
- [API Documentation](api.md) - REST API reference

## Configuration Guides

The configuration system is divided into subsystems, each with its own detailed guide:

### Core Systems
- [Web Server Configuration](reference/webserver_configuration.md) - HTTP server settings
- [WebSocket Configuration](reference/websocket_configuration.md) - Real-time updates
- [Print Queue Configuration](reference/printqueue_configuration.md) - Job management
- [System Resources](reference/resources_configuration.md) - Resource management

### Network & Discovery
- [mDNS Configuration](reference/mdns_configuration.md) - Network discovery
- [Network Architecture](reference/network_architecture.md) - Network design

### Documentation & API
- [Swagger Configuration](reference/swagger_configuration.md) - API documentation
- [API Reference](reference/api.md) - API details
- [OpenAPI Specification](reference/swagger_architecture.md) - API architecture

### Logging & Monitoring
- [Logging Configuration](reference/logging_configuration.md) - Log management
- [Terminal Configuration](reference/terminal_configuration.md) - Web terminal
- [Monitoring Guide](reference/monitoring.md) - System monitoring

### Communication
- [SMTP Configuration](reference/smtp_configuration.md) - Email notifications
- [Database Configuration](reference/database_configuration.md) - Data storage

## Developer Guides

- [Developer Onboarding](developer_onboarding.md) - Getting started with development
- [Coding Guidelines](coding_guidelines.md) - Code standards and practices
- [Testing Guide](testing.md) - Testing procedures
- [Release Process](../RELEASES.md) - Release management

## Architecture Documentation

- [System Architecture](reference/system_architecture.md) - Overall system design
- [Thread Monitoring](thread_monitoring.md) - Thread management
- [Shutdown Architecture](shutdown_architecture.md) - Shutdown process
- [WebSocket Architecture](websocket_architecture.md) - WebSocket system

## Security Documentation

- [Security Configuration](reference/security_configuration.md) - Security settings
- [OIDC Integration](oidc_integration.md) - Authentication
- [Secrets Management](../SECRETS.md) - Managing sensitive data

## Use Cases

- [Home Workshop](guides/use-cases/home-workshop.md) - Single printer setup
- [Print Farm](guides/use-cases/print-farm.md) - Multiple printer management

## Additional Resources

- [FAQ](reference/faq.md) - Common questions
- [Troubleshooting](reference/troubleshooting.md) - Common issues
- [Glossary](reference/glossary.md) - Terms and definitions
- [Contributing](../CONTRIBUTING.md) - How to contribute

## Getting Help

If you need assistance:

1. Check the [Troubleshooting Guide](reference/troubleshooting.md)
2. Search existing [Issues](https://github.com/philement/hydrogen/issues)
3. Join our [Community Forum](https://forum.philement.com)
4. Contact [Support](https://philement.com/support)

## License

Hydrogen is licensed under the MIT License. See [LICENSE](../LICENSE) for details.