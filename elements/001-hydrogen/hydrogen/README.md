# Hydrogen

## Overview

Hydrogen is a high-performance, multithreaded server framework that began as a modern replacement for Mainsail and Moonraker in the 3D printing ecosystem. While it excels at providing a powerful front-end for Klipper-based 3D printers, it has evolved into a versatile platform capable of serving many roles beyond 3D printing.

At its heart, Hydrogen features a robust queue system and a sophisticated service management architecture. Core services include:

- Web Server: Delivers static content and exposes a comprehensive REST API
- WebSocket Server: Enables real-time bidirectional communication with guaranteed message delivery
- mDNS Client/Server: Handles service discovery and auto-configuration
- Queue System: Provides thread-safe data management across all components
- Print Server: Offers complete 3D printer control and monitoring capabilities
- OIDC Provider: Implements OpenID Connect protocol for secure authentication and authorization

Whether you're managing a single 3D printer, orchestrating a large print farm, or building a high-performance web application, Hydrogen provides the foundation you need. Its modular architecture and emphasis on performance make it a versatile tool for both Philement projects and broader applications.

## Intended Audience & Requirements

Hydrogen is currently designed for technical users who:

- Are comfortable working with Linux-based systems
- Have experience with command-line interfaces and system configuration
- Understand basic networking concepts and server administration
- Are familiar with development tools and building software from source

**Platform Support:**

- Primary Platform: Linux-based systems
- Future Support: While we plan to expand platform support in the future, Hydrogen is currently optimized for and tested primarily on Linux systems

**Technical Prerequisites:**

- Linux operating system
- Familiarity with building and running server applications
- Basic understanding of WebSocket and REST API concepts
- Command-line proficiency for configuration and maintenance

## Table of Contents

### Core Documentation

- [**Project Structure**](STRUCTURE.md) - Complete file organization and architecture overview
- [**Build Environment**](SETUP.md) - Build and runtime requirements, environment setup
- [**Development Guide**](RECIPE.md) - Development guide optimized for AI assistance
- [**Release Notes**](RELEASES.md) - Detailed version history and changes
- [**Secrets Management**](SECRETS.md) - Environment variables and security configuration
- [**Site Map**](SITEMAP.md) - Index to all Markdown files in this repository

### Main Documentation

- [**Documentation Hub**](docs/README.md) - Start here for comprehensive overview
- [**Developer Onboarding**](docs/developer_onboarding.md) - Visual architecture overview and code navigation
- [**Quick Start Guide**](docs/guides/quick-start.md) - Get up and running quickly
- [**AI Integration**](docs/ai_integration.md) - AI capabilities and implementations

### Technical References

- [**API Documentation**](docs/api.md) - REST API reference and implementation details
- [**Configuration Guide**](docs/configuration.md) - System configuration and settings
- [**Data Structures**](docs/data_structures.md) - Core data structures and interfaces
- [**Testing Framework**](docs/testing.md) - Testing documentation and procedures

### Architecture & Design

- [**Service Architecture**](docs/service.md) - Service management and lifecycle
- [**Shutdown Architecture**](docs/shutdown_architecture.md) - Graceful shutdown procedures
- [**Thread Monitoring**](docs/thread_monitoring.md) - Thread management and monitoring
- [**WebSocket Implementation**](docs/web_socket.md) - WebSocket server architecture
- [**mDNS Server**](docs/mdns_server.md) - Service discovery implementation
- [**Print Queue System**](docs/print_queue.md) - 3D printing queue management
- [**OIDC Integration**](docs/oidc_integration.md) - OpenID Connect authentication
- [**System Information**](docs/system_info.md) - System monitoring and reporting

### Examples & Implementation

- [**Examples Overview**](examples/README.md) - Code examples and implementations
- [**API Implementation**](src/api/README.md) - API implementation details
- [**Payload System**](payloads/README.md) - Payload system with encryption
- [**Testing Suite**](tests/README.md) - Test framework and procedures

## Latest Test Results

Generated on: Fri Jun 20 17:14:38 PDT 2025

### Summary

| Metric | Value |
| ------ | ----- |
| Total Tests | 16 |
| Passed | 16 |
| Failed | 0 |
| Skipped | 0 |
| Total Subtests | 115 |
| Passed Subtests | 115 |
| Failed Subtests | 0 |
| Runtime | 00:02:06.251 |

### Individual Test Results

| Status | Time | Test | Subs | Pass | Summary |
| ------ | ---- | ---- | ---- | ---- | ------- |
| ✅ | 00:00:15.126 | 10_compilation | 1 | 1 | Test completed without errors |
| ✅ | 00:00:00.088 | 12_env_payload | 2 | 2 | Test completed without errors |
| ✅ | 00:00:02.669 | 15_startup_shutdown | 6 | 6 | Test completed without errors |
| ✅ | 00:00:02.314 | 20_shutdown | 5 | 5 | Test completed without errors |
| ✅ | 00:00:05.889 | 25_library_dependencies | 16 | 16 | Test completed without errors |
| ✅ | 00:00:00.096 | 35_env_variables | 1 | 1 | Test completed without errors |
| ✅ | 00:00:00.092 | 40_json_error_handling | 2 | 2 | Test completed without errors |
| ✅ | 00:00:24.280 | 45_signals | 5 | 5 | Test completed without errors |
| ✅ | 00:00:01.937 | 50_crash_handler | 20 | 20 | Test completed without errors |
| ✅ | 00:00:00.091 | 55_socket_rebind | 1 | 1 | Test completed without errors |
| ✅ | 00:00:12.015 | 60_api_prefixes | 10 | 10 | Test completed without errors |
| ✅ | 00:00:06.028 | 65_system_endpoints | 17 | 17 | Test completed without errors |
| ✅ | 00:00:03.966 | 70_swagger | 10 | 10 | Test completed without errors |
| ✅ | 00:00:03.127 | 95_leaks_like_a_sieve | 2 | 2 | Test completed without errors |
| ✅ | 00:00:16.355 | 98_check_links | 4 | 4 | Test completed without errors |
| ✅ | 00:00:30.271 | 99_codebase | 13 | 13 | Test completed without errors |

## Repository Information

Generated via cloc: Fri Jun 20 17:14:38 PDT 2025

```cloc
github.com/AlDanial/cloc v 2.02  T=0.28 s (1589.4 files/s, 289811.4 lines/s)
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C                              126           4428           5804          19897
Markdown                       147           4660             48          16309
Bourne Shell                    34           1958           2141           9207
JSON                            17              2              0           4585
Text                            20              7              0           4285
C/C++ Header                   105           1098           4275           2678
HTML                             1             74              0            493
make                             2             58            124            285
-------------------------------------------------------------------------------
SUM:                           452          12285          12392          57739
-------------------------------------------------------------------------------
```
