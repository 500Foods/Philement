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

Generated on: 2025-Aug-07 (Thu) 16:37:27 PDT

### Summary

| Metric | Value |
| ------ | ----- |
| Total Tests | 26 |
| Passed | 26 |
| Failed | 0 |
| Total Subtests | 201 |
| Passed Subtests | 201 |
| Failed Subtests | 0 |
| Elapsed Time | 00:00:14.658 |
| Cumulative Time | 00:00:29.957 |

[Test Suite Results](COMPLETE.svg) | [Test Suite Coverage](COVERAGE.svg)

### Test Coverage

| Test Type | Files Cover | Files Instr | Lines Cover | Lines Instr | Coverage | Timestamp |
| --------- | ----------- | ----------- | ----------- | ----------- | -------- | --------- |
| Unity Tests | 12 | 121 | 266 | 11,982 | 2.220% | 20250807_163741 |
| Blackbox Tests | 98 | 121 | 6,254 | 11,983 | 52.191% | 20250807_163741 |
| Combined Tests | 98 | 121 | 6,339 | 11,983 | 52.900% | 20250807_163741 |

### Individual Test Results

| Status | Time | Test | Test Name | Tests | Pass | Fail |
| ------ | ---- | -- | ---- | ----- | ---- | ---- |
| ✅ | 00:00:07.945 | 01-CMP | Compilation (249 source files) | 16 | 16 | 0 |
| ✅ | 00:00:01.899 | 03-SIZ | Code Size Analysis (cloc: 66,055 lines) | 4 | 4 | 0 |
| ✅ | 00:00:03.726 | 06-LNK | Markdown Links Check (github-sitemap) | 3 | 3 | 0 |
| ✅ | 00:00:01.123 | 11-UNT | Unity (187 / 187 unit tests passed) | 16 | 16 | 0 |
| ✅ | 00:00:01.164 | 12-VAR | Env Var Substitution | 15 | 15 | 0 |
| ✅ | 00:00:00.768 | 13-BUG | Crash Handler | 30 | 30 | 0 |
| ✅ | 00:00:00.172 | 14-ENV | Env Vars and Secrets | 5 | 5 | 0 |
| ✅ | 00:00:00.678 | 16-DEP | Library Dependencies | 13 | 13 | 0 |
| ✅ | 00:00:00.189 | 18-JSN | JSON Error Handling | 4 | 4 | 0 |
| ✅ | 00:00:00.358 | 19-SIV | Memory Leak Detection | 4 | 4 | 0 |
| ✅ | 00:00:01.147 | 20-SIG | Signal Handling | 9 | 9 | 0 |
| ✅ | 00:00:00.533 | 22-UPD | Startup/Shutdown | 7 | 7 | 0 |
| ✅ | 00:00:00.256 | 26-SHD | Shutdown | 5 | 5 | 0 |
| ✅ | 00:00:00.714 | 28-SCK | Socket Rebinding | 8 | 8 | 0 |
| ✅ | 00:00:01.350 | 30-PRE | API Prefix | 10 | 10 | 0 |
| ✅ | 00:00:01.088 | 32-SYS | System API | 10 | 10 | 0 |
| ✅ | 00:00:01.352 | 34-SWG | Swagger | 15 | 15 | 0 |
| ✅ | 00:00:01.224 | 36-WSS | WebSockets | 15 | 15 | 0 |
| ✅ | 00:00:00.998 | 90-MKD | Markdown Lint (markdownlint: 235 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.739 | 91-GCC | C Lint (cppcheck: 249 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.608 | 92-BSH | Bash Lint (shellheck: 51 files) | 2 | 2 | 0 |
| ✅ | 00:00:00.237 | 93-JSN | JSON Linting (jsonlint: 23 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.037 | 94-JAV | JavaScript Linting (eslint: 0 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.037 | 95-CSS | CSS Linting (stylelint: 0 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.239 | 96-HTM | HTML Linting (htmlhint: 1 files) | 1 | 1 | 0 |
| ✅ | 00:00:01.376 | 99-COV | Test Suite Coverage (coverage_table) | 4 | 4 | 0 |

## Repository Information

Generated via cloc: 2025-Aug-07 (Thu) 16:37:27 PDT

```cloc
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C                              144           5150           6342          22750
Markdown                       235           7184             56          21942
Bourne Shell                    51           2140           2283           9867
JSON                            22              1              0           5393
C/C++ Header                   105           1099           4276           2688
CMake                            1            139            182            700
HTML                             1             74              0            493
SVG                              1              3              2            490
-------------------------------------------------------------------------------
SUM:                           560          15790          13141          64323
-------------------------------------------------------------------------------

Code/Docs: 1.6    Code/Comments: 2.8
Docs/Code: 0.6    Comments/Code: 0.4
```
