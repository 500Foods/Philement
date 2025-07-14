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
- [**Build Scripts & Utilities**](extras/README.md) - Build scripts and diagnostic tools
- [**API Implementation**](src/api/README.md) - API implementation details
- [**Payload System**](payloads/README.md) - Payload system with encryption
- [**Testing Suite**](tests/README.md) - Test framework and procedures

## Latest Test Results

Generated on: Mon Jul 14 14:00:12 PDT 2025

### Summary

| Metric | Value |
| ------ | ----- |
| Total Tests | 26 |
| Passed | 26 |
| Failed | 0 |
| Skipped | 0 |
| Total Subtests | 191 |
| Passed Subtests | 191 |
| Failed Subtests | 0 |
| Elapsed Time | 00:00:27.648 |
| Running Time | 00:01:06.095 |

### Test Coverage

| Test Type | Files Cover | Files Instr | Lines Cover | Lines Instr | Coverage |
| --------- | ----------- | ----------- | ----------- | ----------- | -------- |
| Unity Tests | 7 | 120 | 220 | 11,964 | 1.839% |
| Blackbox Tests | 92 | 119 | 5,860 | 12,021 | 48.748% |
| Combined Tests | 92 | 120 | 5,860 | 12,021 | 48.748% |

### Individual Test Results

| Status | Time | Test | Tests | Pass | Fail | Summary |
| ------ | ---- | ---- | ----- | ---- | ---- | ------- |
| ✅ | 00:00:07.330 | 01_compilation | 17 | 17 | 0 | Test completed without errors |
| ✅ | 00:00:04.991 | 03_code_size_analysis | 5 | 5 | 0 | Test completed without errors |
| ✅ | 00:00:06.778 | 06_markdown_links_check_(github-sitemap) | 4 | 4 | 0 | Test completed without errors |
| ✅ | 00:00:00.546 | 10_memory_leak_detection | 4 | 4 | 0 | Test completed without errors |
| ✅ | 00:00:01.123 | 11_unity_unit_tests | 6 | 6 | 0 | Test completed without errors |
| ✅ | 00:00:02.030 | 12_env_var_substitution | 16 | 16 | 0 | Test completed without errors |
| ✅ | 00:00:00.173 | 14_payload_env_vars | 2 | 2 | 0 | Test completed without errors |
| ✅ | 00:00:00.839 | 16_library_dependencies | 14 | 14 | 0 | Test completed without errors |
| ✅ | 00:00:00.424 | 18_json_error_handling | 4 | 4 | 0 | Test completed without errors |
| ✅ | 00:00:02.629 | 20_crash_handler | 30 | 30 | 0 | Test completed without errors |
| ✅ | 00:00:01.075 | 22_startup/shutdown | 9 | 9 | 0 | Test completed without errors |
| ✅ | 00:00:02.447 | 24_signal_handling | 9 | 9 | 0 | Test completed without errors |
| ✅ | 00:00:00.622 | 26_shutdown | 6 | 6 | 0 | Test completed without errors |
| ✅ | 00:00:01.244 | 28_socket_rebinding | 7 | 7 | 0 | Test completed without errors |
| ✅ | 00:00:02.000 | 30_api_prefix | 10 | 10 | 0 | Test completed without errors |
| ✅ | 00:00:01.158 | 32_system_api_endpoints | 7 | 7 | 0 | Test completed without errors |
| ✅ | 00:00:02.148 | 34_swagger | 15 | 15 | 0 | Test completed without errors |
| ✅ | 00:00:01.868 | 36_websockets | 14 | 14 | 0 | Test completed without errors |
| ✅ | 00:00:06.326 | 91_c/c++_code_analysis_(cppcheck) | 1 | 1 | 0 | Test completed without errors |
| ✅ | 00:00:07.856 | 92_shell_script_analysis_(shellcheck) | 2 | 2 | 0 | Test completed without errors |
| ✅ | 00:00:00.066 | 94_javascript_linting_(eslint) | 1 | 1 | 0 | Test completed without errors |
| ✅ | 00:00:00.068 | 95_css_linting_(stylelint) | 1 | 1 | 0 | Test completed without errors |
| ✅ | 00:00:00.227 | 96_html_linting_(htmlhint) | 1 | 1 | 0 | Test completed without errors |
| ✅ | 00:00:00.214 | 97_json_linting_(jsonlint) | 1 | 1 | 0 | Test completed without errors |
| ✅ | 00:00:06.134 | 98_markdown_linting_(markdownlint) | 1 | 1 | 0 | Test completed without errors |
| ✅ | 00:00:05.779 | 99_test_suite_coverage_(coverage_tables) | 4 | 4 | 0 | Test completed without errors |

## Repository Information

Generated via cloc: Mon Jul 14 14:00:12 PDT 2025

```cloc
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C                              132           4759           5976          21441
Markdown                       213           6850             48          21035
Bourne Shell                    51           2295           2628          12199
JSON                            21              1              0           5325
CMake                           50            259            293           3152
C/C++ Header                   105           1099           4276           2686
Text                            17              0              0            919
HTML                             1             74              0            493
make                             1            177            208            431
YAML                             1             13              2            255
TypeScript                      36              0              0             72
-------------------------------------------------------------------------------
SUM:                           628          15527          13431          68008
-------------------------------------------------------------------------------

Code/Docs: 1.9    Code/Comments: 3.0
Docs/Code: 0.5    Comments/Code: 0.3
```
