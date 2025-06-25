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

Generated on: Wed Jun 25 05:27:36 PDT 2025

### Summary

| Metric | Value |
| ------ | ----- |
| Total Tests | 17 |
| Passed | 16 |
| Failed | 1 |
| Skipped | 0 |
| Total Subtests | 118 |
| Passed Subtests | 117 |
| Failed Subtests | 1 |
| Runtime | 00:01:58.480 |

### Individual Test Results

| Status | Time | Test | Subs | Pass | Summary |
| ------ | ---- | ---- | ---- | ---- | ------- |
| ✅ | 00:00:19.228 | 10_compilation | 1 | 1 | Test completed without errors |
| ✅ | 00:00:00.089 | 12_env_payload | 2 | 2 | Test completed without errors |
| ✅ | 00:00:02.616 | 15_startup_shutdown | 6 | 6 | Test completed without errors |
| ✅ | 00:00:02.295 | 20_shutdown | 5 | 5 | Test completed without errors |
| ✅ | 00:00:05.849 | 25_library_dependencies | 16 | 16 | Test completed without errors |
| ✅ | 00:00:00.057 | 30_unity_tests | 3 | 3 | Test completed without errors |
| ✅ | 00:00:00.084 | 35_env_variables | 1 | 1 | Test completed without errors |
| ✅ | 00:00:00.088 | 40_json_error_handling | 2 | 2 | Test completed without errors |
| ✅ | 00:00:16.209 | 45_signals | 5 | 5 | Test completed without errors |
| ✅ | 00:00:01.902 | 50_crash_handler | 20 | 20 | Test completed without errors |
| ✅ | 00:00:00.092 | 55_socket_rebind | 1 | 1 | Test completed without errors |
| ✅ | 00:00:06.487 | 60_api_prefixes | 10 | 10 | Test completed without errors |
| ✅ | 00:00:06.007 | 65_system_endpoints | 17 | 17 | Test completed without errors |
| ✅ | 00:00:04.440 | 70_swagger | 10 | 10 | Test completed without errors |
| ✅ | 00:00:03.111 | 95_leaks_like_a_sieve | 2 | 2 | Test completed without errors |
| ✅ | 00:00:17.060 | 98_check_links | 4 | 4 | Test completed without errors |
| ❌ | 00:00:30.941 | 99_codebase | 13 | 12 | Test failed with errors |

## Repository Information

Generated via cloc: Wed Jun 25 05:27:36 PDT 2025

```cloc
github.com/AlDanial/cloc v 2.02  T=0.34 s (1441.6 files/s, 268738.1 lines/s)
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C                              130           4606           5910          20641
Markdown                       159           4809             48          16669
Bourne Shell                    39           2033           2221           9478
JSON                            19              2              0           8334
Text                            22            101              0           5319
C/C++ Header                   105           1098           4275           2678
CMake                            7             97            116            658
HTML                             1             74              0            493
make                             2             58            124            285
YAML                             1             15              2            267
-------------------------------------------------------------------------------
SUM:                           485          12893          12696          64822
-------------------------------------------------------------------------------
```
