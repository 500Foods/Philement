# Hydrogen

## Overview

Hydrogen is a high-performance, multithreaded server.

- Web Server: Delivers static content and exposes a comprehensive REST API
- WebSocket Server: Enables real-time bidirectional communication with guaranteed message delivery
- mDNS Client/Server: Handles service discovery and auto-configuration
- Queue System: Provides thread-safe data management across all components
- Print Server: Offers complete 3D printer control and monitoring capabilities
- OIDC Provider: Implements OpenID Connect protocol for secure authentication and authorization

Its modular architecture and emphasis on performance make it an ideal elemental core for both Philement projects and broader applications.

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

Generated on: Thu Jul 17 18:30:42 PDT 2025

### Summary

| Metric | Value |
| ------ | ----- |
| Total Tests | 26 |
| Passed | 26 |
| Failed | 0 |
| Skipped | 0 |
| Total Subtests | 208 |
| Passed Subtests | 208 |
| Failed Subtests | 0 |
| Elapsed Time | 00:00:29.161 |
| Running Time | 00:01:05.612 |

[Test Suite Results](COMPLETE.svg) | [Test Suite Coverage](COVERAGE.svg)

### Test Coverage

| Test Type | Files Cover | Files Instr | Lines Cover | Lines Instr | Coverage |
| --------- | ----------- | ----------- | ----------- | ----------- | -------- |
| Unity Tests | 12 | 121 | 266 | 11,982 | 2.220% |
| Blackbox Tests | 98 | 121 | 6,229 | 11,983 | 51.982% |
| Combined Tests | 98 | 121 | 6,314 | 11,983 | 52.691% |

### Individual Test Results

| Status | Time | Test | Tests | Pass | Fail | Summary |
| ------ | ---- | ---- | ----- | ---- | ---- | ------- |
| ✅ | 00:00:08.608 | 01_compilation | 18 | 18 | 0 | Test completed without errors |
| ✅ | 00:00:06.372 | 03_code_size_analysis | 5 | 5 | 0 | Test completed without errors |
| ✅ | 00:00:08.011 | 06_markdown_links_check_(github-sitemap) | 4 | 4 | 0 | Test completed without errors |
| ✅ | 00:00:00.392 | 10_memory_leak_detection | 4 | 4 | 0 | Test completed without errors |
| ✅ | 00:00:01.469 | 11_unity_unit_tests | 17 | 17 | 0 | Test completed without errors |
| ✅ | 00:00:01.984 | 12_env_var_substitution | 16 | 16 | 0 | Test completed without errors |
| ✅ | 00:00:00.168 | 14_payload_env_vars | 2 | 2 | 0 | Test completed without errors |
| ✅ | 00:00:00.831 | 16_library_dependencies | 14 | 14 | 0 | Test completed without errors |
| ✅ | 00:00:00.432 | 18_json_error_handling | 4 | 4 | 0 | Test completed without errors |
| ✅ | 00:00:02.558 | 20_crash_handler | 30 | 30 | 0 | Test completed without errors |
| ✅ | 00:00:01.060 | 22_startup/shutdown | 9 | 9 | 0 | Test completed without errors |
| ✅ | 00:00:01.753 | 24_signal_handling | 9 | 9 | 0 | Test completed without errors |
| ✅ | 00:00:00.602 | 26_shutdown | 6 | 6 | 0 | Test completed without errors |
| ✅ | 00:00:01.250 | 28_socket_rebinding | 7 | 7 | 0 | Test completed without errors |
| ✅ | 00:00:01.969 | 30_api_prefix | 10 | 10 | 0 | Test completed without errors |
| ✅ | 00:00:01.680 | 32_system_api_endpoints | 10 | 10 | 0 | Test completed without errors |
| ✅ | 00:00:02.168 | 34_swagger | 15 | 15 | 0 | Test completed without errors |
| ✅ | 00:00:02.843 | 36_websockets | 16 | 16 | 0 | Test completed without errors |
| ✅ | 00:00:05.281 | 91_c/c++_code_analysis_(cppcheck) | 1 | 1 | 0 | Test completed without errors |
| ✅ | 00:00:06.177 | 92_shell_script_analysis_(shellcheck) | 2 | 2 | 0 | Test completed without errors |
| ✅ | 00:00:00.070 | 94_javascript_linting_(eslint) | 1 | 1 | 0 | Test completed without errors |
| ✅ | 00:00:00.068 | 95_css_linting_(stylelint) | 1 | 1 | 0 | Test completed without errors |
| ✅ | 00:00:00.223 | 96_html_linting_(htmlhint) | 1 | 1 | 0 | Test completed without errors |
| ✅ | 00:00:00.225 | 97_json_linting_(jsonlint) | 1 | 1 | 0 | Test completed without errors |
| ✅ | 00:00:04.796 | 98_markdown_linting_(markdownlint) | 1 | 1 | 0 | Test completed without errors |
| ✅ | 00:00:04.622 | 99_test_suite_coverage_(coverage_table) | 4 | 4 | 0 | Test completed without errors |

## Repository Information

Generated via cloc: Thu Jul 17 18:30:42 PDT 2025

```cloc
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C                              145           5304           6403          23439
Markdown                       218           6993             55          21442
Bourne Shell                    52           2489           2714          13078
JSON                            21              1              0           5330
CMake                           76            293            330           5030
C/C++ Header                   105           1099           4276           2688
SVG                              2              6              4           2126
Text                            30            105              0           1673
YAML                             1             43              6            828
make                             1            243            296            585
HTML                             1             74              0            493
TypeScript                      60              0              0            120
-------------------------------------------------------------------------------
SUM:                           712          16650          14084          76832
-------------------------------------------------------------------------------

Code/Docs: 2.1    Code/Comments: 3.2
Docs/Code: 0.5    Comments/Code: 0.3
```
