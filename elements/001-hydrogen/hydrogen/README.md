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

Generated on: Fri Jul 18 19:23:19 PDT 2025

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
| Elapsed Time | 00:00:17.719 |
| Cumulative Time | 00:00:44.587 |

[Test Suite Results](COMPLETE.svg) | [Test Suite Coverage](COVERAGE.svg)

### Test Coverage

| Test Type | Files Cover | Files Instr | Lines Cover | Lines Instr | Coverage |
| --------- | ----------- | ----------- | ----------- | ----------- | -------- |
| Unity Tests | 12 | 121 | 266 | 11,982 | 2.220% |
| Blackbox Tests | 98 | 121 | 6,234 | 11,983 | 52.024% |
| Combined Tests | 98 | 121 | 6,319 | 11,983 | 52.733% |

### Individual Test Results

| Status | Time | Test | Tests | Pass | Fail | Summary |
| ------ | ---- | ---- | ----- | ---- | ---- | ------- |
| ✅ | 00:00:08.158 | 01_compilation_{blue}(243_source_files){reset} | 18 | 18 | 0 | Test completed without errors |
| ✅ | 00:00:02.176 | 03_code_size_analysis | 5 | 5 | 0 | Test completed without errors |
| ✅ | 00:00:06.859 | 06_markdown_links_check_{blue}(github-sitemap){reset} | 4 | 4 | 0 | Test completed without errors |
| ✅ | 00:00:01.675 | 11_unity_{blue}(187_/_187_unit_tests_passed){reset} | 17 | 17 | 0 | Test completed without errors |
| ✅ | 00:00:02.024 | 12_env_var_substitution | 16 | 16 | 0 | Test completed without errors |
| ✅ | 00:00:02.625 | 13_crash_handler | 30 | 30 | 0 | Test completed without errors |
| ✅ | 00:00:00.175 | 14_payload_env_vars | 2 | 2 | 0 | Test completed without errors |
| ✅ | 00:00:00.902 | 16_library_dependencies | 14 | 14 | 0 | Test completed without errors |
| ✅ | 00:00:00.535 | 18_json_error_handling | 4 | 4 | 0 | Test completed without errors |
| ✅ | 00:00:00.478 | 19_memory_leak_detection | 4 | 4 | 0 | Test completed without errors |
| ✅ | 00:00:01.093 | 22_startup/shutdown | 9 | 9 | 0 | Test completed without errors |
| ✅ | 00:00:01.881 | 24_signal_handling | 9 | 9 | 0 | Test completed without errors |
| ✅ | 00:00:00.638 | 26_shutdown | 6 | 6 | 0 | Test completed without errors |
| ✅ | 00:00:01.283 | 28_socket_rebinding | 7 | 7 | 0 | Test completed without errors |
| ✅ | 00:00:01.973 | 29_api_prefix | 10 | 10 | 0 | Test completed without errors |
| ✅ | 00:00:01.651 | 32_system_api | 10 | 10 | 0 | Test completed without errors |
| ✅ | 00:00:02.192 | 34_swagger | 15 | 15 | 0 | Test completed without errors |
| ✅ | 00:00:02.659 | 36_websockets | 16 | 16 | 0 | Test completed without errors |
| ✅ | 00:00:00.916 | 91_c/c++_code_analysis_{blue}(cppcheck){reset} | 1 | 1 | 0 | Test completed without errors |
| ✅ | 00:00:00.665 | 92_shell_script_analysis_{blue}(shellcheck){reset} | 2 | 2 | 0 | Test completed without errors |
| ✅ | 00:00:00.067 | 94_javascript_linting_{blue}(eslint){reset} | 1 | 1 | 0 | Test completed without errors |
| ✅ | 00:00:00.072 | 95_css_linting_{blue}(stylelint){reset} | 1 | 1 | 0 | Test completed without errors |
| ✅ | 00:00:00.213 | 96_html_linting_{blue}(htmlhint){reset} | 1 | 1 | 0 | Test completed without errors |
| ✅ | 00:00:00.218 | 97_json_linting_{blue}(jsonlint){reset} | 1 | 1 | 0 | Test completed without errors |
| ✅ | 00:00:01.748 | 98_markdown_linting_{blue}(markdownlint){reset} | 1 | 1 | 0 | Test completed without errors |
| ✅ | 00:00:01.711 | 99_test_suite_coverage_{blue}(coverage_table){reset} | 4 | 4 | 0 | Test completed without errors |

## Repository Information

Generated via cloc: Fri Jul 18 19:23:19 PDT 2025

```cloc
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C                              144           5150           6342          22750
Markdown                       217           6900             56          20962
Bourne Shell                    52           2534           2777          13268
JSON                            21              1              0           5378
C/C++ Header                   105           1099           4276           2688
CMake                            5            160            215           1000
HTML                             1             74              0            493
Text                             1              0              0              2
-------------------------------------------------------------------------------
SUM:                           546          15918          13666          66541
-------------------------------------------------------------------------------

Code/Docs: 1.9    Code/Comments: 2.9
Docs/Code: 0.5    Comments/Code: 0.3
```
