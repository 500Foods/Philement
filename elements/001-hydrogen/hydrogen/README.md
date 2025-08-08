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

Generated on: 2025-Aug-08 (Fri) 14:17:53 PDT

### Summary

| Metric | Value |
| ------ | ----- |
| Total Tests | 26 |
| Passed | 26 |
| Failed | 0 |
| Total Subtests | 208 |
| Passed Subtests | 208 |
| Failed Subtests | 0 |
| Elapsed Time | 00:00:14.627 |
| Cumulative Time | 00:00:28.893 |

[Test Suite Results](COMPLETE.svg) | [Test Suite Coverage](COVERAGE.svg)

### Test Coverage

| Test Type | Files Cover | Files Instr | Lines Cover | Lines Instr | Coverage | Timestamp |
| --------- | ----------- | ----------- | ----------- | ----------- | -------- | --------- |
| Unity Tests | 12 | 121 | 266 | 12,014 | 2.214% | 20250808_141807 |
| Blackbox Tests | 98 | 121 | 6,300 | 12,015 | 52.434% | 20250808_141807 |
| Combined Tests | 98 | 121 | 6,385 | 12,015 | 53.142% | 20250808_141806 |

### Individual Test Results

| Status | Time | Test | Test Name | Tests | Pass | Fail |
| ------ | ---- | -- | ---- | ----- | ---- | ---- |
| ✅ | 00:00:07.834 | 01-CMP | Compilation (249 source files) | 16 | 16 | 0 |
| ✅ | 00:00:01.936 | 03-SIZ | Code Size Analysis (cloc: 66,050 lines) | 4 | 4 | 0 |
| ✅ | 00:00:02.138 | 06-LNK | Markdown Links Check (github-sitemap) | 3 | 3 | 0 |
| ✅ | 00:00:01.258 | 11-UNT | Unity (187 / 187 unit tests passed) | 16 | 16 | 0 |
| ✅ | 00:00:01.289 | 12-VAR | Env Var Substitution | 15 | 15 | 0 |
| ✅ | 00:00:01.310 | 13-BUG | Crash Handler | 37 | 37 | 0 |
| ✅ | 00:00:00.160 | 14-ENV | Env Vars and Secrets | 5 | 5 | 0 |
| ✅ | 00:00:00.660 | 16-DEP | Library Dependencies | 13 | 13 | 0 |
| ✅ | 00:00:00.141 | 18-JSN | JSON Error Handling | 4 | 4 | 0 |
| ✅ | 00:00:00.393 | 19-SIV | Memory Leak Detection | 4 | 4 | 0 |
| ✅ | 00:00:01.128 | 20-SIG | Signal Handling | 9 | 9 | 0 |
| ✅ | 00:00:00.512 | 22-UPD | Startup/Shutdown | 7 | 7 | 0 |
| ✅ | 00:00:00.238 | 26-SHD | Shutdown | 5 | 5 | 0 |
| ✅ | 00:00:00.686 | 28-SCK | Socket Rebinding | 8 | 8 | 0 |
| ✅ | 00:00:01.312 | 30-PRE | API Prefix | 10 | 10 | 0 |
| ✅ | 00:00:01.112 | 32-SYS | System API | 10 | 10 | 0 |
| ✅ | 00:00:01.297 | 34-SWG | Swagger | 15 | 15 | 0 |
| ✅ | 00:00:01.219 | 36-WSS | WebSockets | 15 | 15 | 0 |
| ✅ | 00:00:00.981 | 90-MKD | Markdown Lint (markdownlint: 236 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.757 | 91-GCC | C Lint (cppcheck: 249 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.573 | 92-BSH | Bash Lint (shellheck: 51 files) | 2 | 2 | 0 |
| ✅ | 00:00:00.269 | 93-JSN | JSON Linting (jsonlint: 25 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.036 | 94-JAV | JavaScript Linting (eslint: 0 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.038 | 95-CSS | CSS Linting (stylelint: 0 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.282 | 96-HTM | HTML Linting (htmlhint: 1 files) | 1 | 1 | 0 |
| ✅ | 00:00:01.334 | 99-COV | Test Suite Coverage (coverage_table) | 4 | 4 | 0 |

## Repository Information

Generated via cloc: 2025-Aug-08 (Fri) 14:17:53 PDT

```cloc
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C                              144           5161           6354          22798
Markdown                       236           7191             57          21955
Bourne Shell                    51           2133           2294           9795
JSON                            22              1              0           5393
C/C++ Header                   105           1100           4277           2692
CMake                            1            139            182            700
HTML                             1             74              0            493
SVG                              1              3              2            490
-------------------------------------------------------------------------------
SUM:                           561          15802          13166          64316
-------------------------------------------------------------------------------

Code/Docs: 1.6    Code/Comments: 2.7
Docs/Code: 0.6    Comments/Code: 0.4
```
