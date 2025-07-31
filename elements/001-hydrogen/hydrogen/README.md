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

Generated on: 2025-Jul-31 (Thu) 15:34:55 PDT

### Summary

| Metric | Value |
| ------ | ----- |
| Total Tests | 26 |
| Passed | 25 |
| Failed | 0 |
| Total Subtests | 208 |
| Passed Subtests | 207 |
| Failed Subtests | 0 |
| Elapsed Time | 00:00:18.963 |
| Cumulative Time | 00:00:33.234 |

[Test Suite Results](COMPLETE.svg) | [Test Suite Coverage](COVERAGE.svg)

### Test Coverage

| Test Type | Files Cover | Files Instr | Lines Cover | Lines Instr | Coverage | Timestamp |
| --------- | ----------- | ----------- | ----------- | ----------- | -------- | --------- |
| Unity Tests | 0 | 0 | 0 | 0 | 0.000% | 0 |
| Blackbox Tests | 0 | 0 | 0 | 0 | 0.000% | 0 |

### Individual Test Results

| Status | Time | Test | Tests | Pass | Fail | Summary |
| ------ | ---- | ---- | ----- | ---- | ---- | ------- |
| ✅ | 00:00:07.666 | 01_compilation_{blue}(249_source_files){reset} | 17 | 17 | 0 | Test completed without errors |
| ✅ | 00:00:02.133 | 03_code_size_analysis_{blue}(cloc:_549_files){reset} | 5 | 5 | 0 | Test completed without errors |
| ✅ | 00:00:03.318 | 06_markdown_links_check_{blue}(github-sitemap){reset} | 4 | 4 | 0 | Test completed without errors |
| ✅ | 00:00:01.362 | 11_unity_{blue}(187_/_187_unit_tests_passed){reset} | 16 | 16 | 0 | Test completed without errors |
| ✅ | 00:00:01.064 | 12_env_var_substitution | 16 | 16 | 0 | Test completed without errors |
| ✅ | 00:00:02.334 | 13_crash_handler | 30 | 30 | 0 | Test completed without errors |
| ✅ | 00:00:00.153 | 14_env_vars_and_secrets | 5 | 5 | 0 | Test completed without errors |
| ✅ | 00:00:00.564 | 16_library_dependencies | 14 | 14 | 0 | Test completed without errors |
| ✅ | 00:00:00.166 | 18_json_error_handling | 4 | 4 | 0 | Test completed without errors |
| ✅ | 00:00:00.314 | 19_memory_leak_detection | 4 | 4 | 0 | Test completed without errors |
| ✅ | 00:00:01.566 | 20_signal_handling | 9 | 9 | 0 | Test completed without errors |
| ✅ | 00:00:00.479 | 22_startup/shutdown | 8 | 8 | 0 | Test completed without errors |
| ✅ | 00:00:00.267 | 26_shutdown | 6 | 6 | 0 | Test completed without errors |
| ✅ | 00:00:00.685 | 28_socket_rebinding | 7 | 7 | 0 | Test completed without errors |
| ✅ | 00:00:01.302 | 30_api_prefix | 10 | 10 | 0 | Test completed without errors |
| ✅ | 00:00:01.124 | 32_system_api | 10 | 10 | 0 | Test completed without errors |
| ✅ | 00:00:01.243 | 34_swagger | 15 | 15 | 0 | Test completed without errors |
| ✅ | 00:00:02.060 | 36_websockets | 16 | 16 | 0 | Test completed without errors |
| ✅ | 00:00:01.834 | 90_markdown_lint_{blue}(markdownlint:_218_files){reset} | 1 | 1 | 0 | Test completed without errors |
| ✅ | 00:00:00.933 | 91_c_lint_{blue}(cppcheck:_249_files){reset} | 1 | 1 | 0 | Test completed without errors |
| ❌ | 00:00:00.607 | 92_bash_lint_{blue}(shellheck:_53_files){reset} | 2 | 1 | 1 | Test failed with errors |
| ✅ | 00:00:00.195 | 93_json_linting_{blue}(jsonlint:_23_files){reset} | 1 | 1 | 0 | Test completed without errors |
| ✅ | 00:00:00.038 | 94_javascript_linting_{blue}(eslint:_0_files){reset} | 1 | 1 | 0 | Test completed without errors |
| ✅ | 00:00:00.039 | 95_css_linting_{blue}(stylelint:_0_files){reset} | 1 | 1 | 0 | Test completed without errors |
| ✅ | 00:00:00.197 | 96_html_linting_{blue}(htmlhint:_1_files){reset} | 1 | 1 | 0 | Test completed without errors |
| ✅ | 00:00:01.591 | 99_test_suite_coverage_{blue}(coverage_table){reset} | 4 | 4 | 0 | Test completed without errors |

## Repository Information

Generated via cloc: 2025-Jul-31 (Thu) 15:34:55 PDT

```cloc
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
Text                             3              0              0         163787
C                              144           5150           6342          22750
Markdown                       218           7115             56          21766
Bourne Shell                    53           2297           2662          11431
JSON                            22              1              0           5392
C/C++ Header                   105           1099           4276           2688
CMake                            1            139            182            700
HTML                             1             74              0            493
-------------------------------------------------------------------------------
SUM:                           547          15875          13518         229007
-------------------------------------------------------------------------------

Code/Docs: 1.7    Code/Comments: 2.8
Docs/Code: 0.6    Comments/Code: 0.4
```
