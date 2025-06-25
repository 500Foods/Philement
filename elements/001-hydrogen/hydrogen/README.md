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

Generated on: Wed Jun 25 07:24:09 PDT 2025

### Summary

| Metric | Value |
| ------ | ----- |
| Total Tests | 17 |
| Passed | 17 |
| Failed | 0 |
| Skipped | 0 |
| Total Subtests | 118 |
| Passed Subtests | 118 |
| Failed Subtests | 0 |
| Runtime | 00:01:52.754 |

### Individual Test Results

| Status | Time | Test | Subs | Pass | Summary |
| ------ | ---- | ---- | ---- | ---- | ------- |
| ✅ | 00:00:11.672 | 10_compilation | 1 | 1 | Test completed without errors |
| ✅ | 00:00:00.123 | 12_env_payload | 2 | 2 | Test completed without errors |
| ✅ | 00:00:02.665 | 15_startup_shutdown | 6 | 6 | Test completed without errors |
| ✅ | 00:00:02.297 | 20_shutdown | 5 | 5 | Test completed without errors |
| ✅ | 00:00:05.856 | 25_library_dependencies | 16 | 16 | Test completed without errors |
| ✅ | 00:00:00.178 | 30_unity_tests | 3 | 3 | Test completed without errors |
| ✅ | 00:00:00.088 | 35_env_variables | 1 | 1 | Test completed without errors |
| ✅ | 00:00:00.092 | 40_json_error_handling | 2 | 2 | Test completed without errors |
| ✅ | 00:00:16.267 | 45_signals | 5 | 5 | Test completed without errors |
| ✅ | 00:00:01.946 | 50_crash_handler | 20 | 20 | Test completed without errors |
| ✅ | 00:00:00.090 | 55_socket_rebind | 1 | 1 | Test completed without errors |
| ✅ | 00:00:08.482 | 60_api_prefixes | 10 | 10 | Test completed without errors |
| ✅ | 00:00:06.016 | 65_system_endpoints | 17 | 17 | Test completed without errors |
| ✅ | 00:00:03.953 | 70_swagger | 10 | 10 | Test completed without errors |
| ✅ | 00:00:03.110 | 95_leaks_like_a_sieve | 2 | 2 | Test completed without errors |
| ✅ | 00:00:18.208 | 98_check_links | 4 | 4 | Test completed without errors |
| ✅ | 00:00:29.704 | 99_codebase | 13 | 13 | Test completed without errors |

## Repository Information

Generated via cloc: Wed Jun 25 07:24:09 PDT 2025

```cloc
github.com/AlDanial/cloc v 2.02  T=0.34 s (1320.1 files/s, 233472.2 lines/s)
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C                              129           4452           5849          19953
Markdown                       157           4812             48          16659
Bourne Shell                    39           2044           2228           9606
JSON                            17              2              0           4701
C/C++ Header                   105           1098           4275           2678
HTML                             1             74              0            493
CMake                            2             65             95            340
make                             2             58            124            285
-------------------------------------------------------------------------------
SUM:                           452          12605          12619          54715
-------------------------------------------------------------------------------
```
