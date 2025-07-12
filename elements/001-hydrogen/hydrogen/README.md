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

Generated on: Fri Jul 11 18:48:36 PDT 2025

### Summary

| Metric | Value |
| ------ | ----- |
| Total Tests | 25 |
| Passed | 25 |
| Failed | 0 |
| Skipped | 0 |
| Total Subtests | 179 |
| Passed Subtests | 178 |
| Failed Subtests | 1 |
| Elapsed Time | 00:00:37.857 |
| Running Time | 00:01:36.624 |

### Unit Tests

| Test Type | Files Cover | Files Instr | Lines Cover | Lines Instr | Coverage |
| --------- | ----------- | ----------- | ----------- | ----------- | -------- |
| Unity Tests | 3 | 118 | 31 | 26,625 | 0.116% |
| Blackbox Tests | 88 | 117 | 5,446 | 26,602 | 20.472% |

### Individual Test Results

| Status | Time | Test | Tests | Pass | Fail | Summary |
| ------ | ---- | ---- | ----- | ---- | ---- | ------- |
| ✅ | 00:00:17.846 | 01_compilation | 17 | 17 | 0 | Test completed without errors |
| ✅ | 00:00:14.833 | 03_code_size_analysis | 5 | 5 | 0 | Test completed without errors |
| ✅ | 00:00:06.288 | 04_c/c++_code_analysis_(cppcheck) | 1 | 1 | 0 | Test completed without errors |
| ❌ | 00:00:08.199 | 05_shell_script_analysis_(shellcheck) | 2 | 1 | 1 | Test failed with errors |
| ✅ | 00:00:15.429 | 06_markdown_links_check_(github-sitemap) | 4 | 4 | 0 | Test completed without errors |
| ✅ | 00:00:01.380 | 10_memory_leak_detection | 4 | 4 | 0 | Test completed without errors |
| ✅ | 00:00:01.775 | 11_unity_unit_tests | 6 | 6 | 0 | Test completed without errors |
| ✅ | 00:00:01.582 | 12_env_var_substitution | 16 | 16 | 0 | Test completed without errors |
| ✅ | 00:00:00.156 | 14_payload_env_vars | 2 | 2 | 0 | Test completed without errors |
| ✅ | 00:00:00.679 | 16_library_dependencies | 14 | 14 | 0 | Test completed without errors |
| ✅ | 00:00:00.410 | 18_json_error_handling | 4 | 4 | 0 | Test completed without errors |
| ✅ | 00:00:02.508 | 20_crash_handler | 22 | 22 | 0 | Test completed without errors |
| ✅ | 00:00:00.847 | 22_startup/shutdown | 9 | 9 | 0 | Test completed without errors |
| ✅ | 00:00:02.282 | 24_signal_handling | 9 | 9 | 0 | Test completed without errors |
| ✅ | 00:00:00.483 | 26_shutdown | 6 | 6 | 0 | Test completed without errors |
| ✅ | 00:00:01.027 | 28_socket_rebinding | 7 | 7 | 0 | Test completed without errors |
| ✅ | 00:00:01.642 | 30_api_prefix | 10 | 10 | 0 | Test completed without errors |
| ✅ | 00:00:01.711 | 32_system_api_endpoints | 18 | 18 | 0 | Test completed without errors |
| ✅ | 00:00:01.730 | 34_swagger | 13 | 13 | 0 | Test completed without errors |
| ✅ | 00:00:00.053 | 94_javascript_linting_(eslint) | 1 | 1 | 0 | Test completed without errors |
| ✅ | 00:00:00.054 | 95_css_linting_(stylelint) | 1 | 1 | 0 | Test completed without errors |
| ✅ | 00:00:00.165 | 96_html_linting_(htmlhint) | 1 | 1 | 0 | Test completed without errors |
| ✅ | 00:00:00.151 | 97_json_linting_(jsonlint) | 1 | 1 | 0 | Test completed without errors |
| ✅ | 00:00:01.850 | 98_markdown_linting_(markdownlint) | 1 | 1 | 0 | Test completed without errors |
| ✅ | 00:00:13.544 | 99_build_system_cleanup_and_coverage_collection | 5 | 5 | 0 | Test completed without errors |

## Repository Information

Generated via cloc: Fri Jul 11 18:48:36 PDT 2025

```cloc
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
Markdown                       208           7097             48          22220
C                              131           4671           5901          20924
Bourne Shell                    56           2143           2229          13358
JSON                            17              2              0           4701
C/C++ Header                   105           1098           4275           2678
Text                             9            142              0           1201
CMake                           19            182            236           1107
YAML                             2             30              4            568
HTML                             1             74              0            493
make                             2            123            126            259
TypeScript                       4              0              0              8
-------------------------------------------------------------------------------
SUM:                           554          15562          12819          67517
-------------------------------------------------------------------------------

Code/Docs: 1.7    Code/Comments: 3.0
Docs/Code: 0.6    Comments/Code: 0.3
```
