# Hydrogen

## Overview

Hydrogen is a high-performance, multithreaded server framework that began as an idea for a modern replacement for Mainsail and Moonraker in the 3D printing ecosystem. It has since evolved into a versatile platform capable of serving many roles beyond 3D printing.

At its heart, Hydrogen features a robust queue system and a sophisticated service management architecture. Core services include:

- Web Server: Delivers static content and exposes a comprehensive REST API
- WebSocket Server: Enables real-time bidirectional communication with guaranteed message delivery
- mDNS Client/Server: Handles service discovery and auto-configuration
- Queue System: Provides thread-safe data management across all components
- Print Server: Offers complete 3D printer control and monitoring capabilities
- OIDC Provider: Implements OpenID Connect protocol for secure authentication and authorization

Whether you're managing a single 3D printer, orchestrating a large print farm, or building a high-performance web application, Hydrogen provides the foundation you need. Its modular architecture and emphasis on performance make it an ideal elemental core for both Philement projects and broader applications.

## Intended Audience & Requirements

Hydrogen is currently designed for technical users who:

- Are comfortable working with Linux-based systems
- Have experience with command-line interfaces and system configuration
- Understand basic networking concepts and server administration
- Are familiar with development tools and building software from source

**Platform Support:**

- Primary Platform: Linux-based systems - Fedora and Ubuntu to start with.

## Table of Contents

### Getting Started

- [**AI Recipe**](RECIPE.md) - Development guide optimized for AI assistance
- [**Project Structure**](STRUCTURE.md) - Complete file organization and architecture overview
- [**Build Environment**](SETUP.md) - Build and runtime requirements, environment setup
- [**Secrets Management**](SECRETS.md) - Environment variables and security configuration
- [**Release Notes**](RELEASES.md) - Detailed version history and changes
- [**Markdown Site Map**](SITEMAP.md) - Index to all Markdown files in this repository

### Core Documentation

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

Generated on: Wed Jul  2 09:42:28 PDT 2025

### Summary

| Metric | Value |
| ------ | ----- |
| Total Tests | 17 |
| Passed | 17 |
| Failed | 0 |
| Skipped | 0 |
| Total Subtests | 132 |
| Passed Subtests | 132 |
| Failed Subtests | 0 |
| Runtime | 00:01:56.322 |
| Execution | 00:01:54.948 |

### Individual Test Results

| Status | Time | Test | Subs | Pass | Summary |
| ------ | ---- | ---- | ---- | ---- | ------- |
| ✅ | 00:00:09.350 | 10_compilation | 13 | 13 | Test completed without errors |
| ✅ | 00:00:00.887 | 12_env_payload | 2 | 2 | Test completed without errors |
| ✅ | 00:00:01.433 | 15_startup_shutdown | 9 | 9 | Test completed without errors |
| ✅ | 00:00:01.325 | 20_shutdown | 5 | 5 | Test completed without errors |
| ✅ | 00:00:03.707 | 25_library_dependencies | 16 | 16 | Test completed without errors |
| ✅ | 00:00:00.183 | 30_unity_tests | 3 | 3 | Test completed without errors |
| ✅ | 00:00:00.073 | 35_env_variables | 1 | 1 | Test completed without errors |
| ✅ | 00:00:00.090 | 40_json_error_handling | 2 | 2 | Test completed without errors |
| ✅ | 00:00:11.439 | 45_signals | 5 | 5 | Test completed without errors |
| ✅ | 00:00:01.817 | 50_crash_handler | 20 | 20 | Test completed without errors |
| ✅ | 00:00:00.085 | 55_socket_rebind | 1 | 1 | Test completed without errors |
| ✅ | 00:00:05.357 | 60_api_prefixes | 10 | 10 | Test completed without errors |
| ✅ | 00:00:06.001 | 65_system_endpoints | 17 | 17 | Test completed without errors |
| ✅ | 00:00:06.517 | 70_swagger | 10 | 10 | Test completed without errors |
| ✅ | 00:00:02.904 | 95_leaks_like_a_sieve | 2 | 2 | Test completed without errors |
| ✅ | 00:00:21.711 | 98_check_links | 4 | 4 | Test completed without errors |
| ✅ | 00:00:42.061 | 99_codebase | 12 | 12 | Test completed without errors |

## Repository Information

Generated via cloc: Wed Jul  2 09:42:28 PDT 2025

```cloc
github.com/AlDanial/cloc v 2.02  T=0.36 s (1395.7 files/s, 236075.6 lines/s)
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C                              129           4459           5851          19980
Markdown                       196           5665             48          18329
Bourne Shell                    50           2297           2614          11434
JSON                            17              2              0           4701
C/C++ Header                   105           1098           4275           2678
HTML                             1             74              0            493
CMake                            2             84            139            397
Text                             1             26              0             97
-------------------------------------------------------------------------------
SUM:                           501          13705          12927          58109
-------------------------------------------------------------------------------
```
