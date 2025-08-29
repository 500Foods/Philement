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

Generated on: 2025-Aug-29 (Fri) 14:12:50 PDT

### Summary

| Metric | Value |
| ------ | ----- |
| Total Tests | 28 |
| Passed | 28 |
| Failed | 0 |
| Total Subtests | 387 |
| Passed Subtests | 387 |
| Failed Subtests | 0 |
| Elapsed Time | 00:00:22.950 |
| Cumulative Time | 00:00:41.608 |

[Test Suite Results](COMPLETE.svg) | [Test Suite Coverage](COVERAGE.svg)

### Test Coverage

| Test Type | Files Cover | Files Instr | Lines Cover | Lines Instr | Coverage |
| --------- | ----------- | ----------- | ----------- | ----------- | -------- |
| Unity Tests | 97 | 129 | 5,118 | 13,073 | 39.149% |
| Blackbox Tests | 113 | 129 | 7,951 | 13,073 | 60.820% |
| Combined Tests | 113 | 129 | 9,003 | 13,073 | 68.867% |

### Individual Test Results

| Status | Time | Test | Test Name | Tests | Pass | Fail |
| ------ | ---- | -- | ---- | ----- | ---- | ---- |
| ✅ | 00:00:05.993 | 01-CMP | Compilation (source code: 394 files) | 16 | 16 | 0 |
| ✅ | 00:00:00.421 | 02-ENV | Environment Variables (secrets: 3) | 5 | 5 | 0 |
| ✅ | 00:00:02.901 | 04-LNK | Markdown Links Check (github-sitemap) | 3 | 3 | 0 |
| ✅ | 00:00:07.712 | 10-UNT | Unity (1,346 / 1,346 unit tests passed) | 174 | 174 | 0 |
| ✅ | 00:00:00.469 | 11-SIV | Memory Leak Detection | 4 | 4 | 0 |
| ✅ | 00:00:01.182 | 12-VAR | Env Var Substitution | 15 | 15 | 0 |
| ✅ | 00:00:01.269 | 13-BUG | Crash Handler | 37 | 37 | 0 |
| ✅ | 00:00:00.766 | 14-DEP | Library Dependencies | 13 | 13 | 0 |
| ✅ | 00:00:00.159 | 15-JSN | JSON Error Handling | 4 | 4 | 0 |
| ✅ | 00:00:00.413 | 16-SHD | Shutdown | 5 | 5 | 0 |
| ✅ | 00:00:01.010 | 17-UPD | Startup/Shutdown | 9 | 9 | 0 |
| ✅ | 00:00:00.579 | 18-SIG | Signal Handling | 9 | 9 | 0 |
| ✅ | 00:00:01.131 | 19-SCK | Socket Rebinding | 8 | 8 | 0 |
| ✅ | 00:00:00.590 | 20-PRE | API Prefix | 11 | 11 | 0 |
| ✅ | 00:00:00.755 | 21-SYS | System API | 10 | 10 | 0 |
| ✅ | 00:00:00.599 | 22-SWG | Swagger | 15 | 15 | 0 |
| ✅ | 00:00:01.080 | 23-WSS | WebSockets | 13 | 13 | 0 |
| ✅ | 00:00:00.909 | 24-UPL | Uploads | 13 | 13 | 0 |
| ✅ | 00:00:03.326 | 25-DNS | mDNS | 7 | 7 | 0 |
| ✅ | 00:00:01.733 | 90-MKD | Markdown Lint (markdownlint: 252 files) | 1 | 1 | 0 |
| ✅ | 00:00:02.390 | 91-GCC | C Lint (cppcheck: 395 files) | 1 | 1 | 0 |
| ✅ | 00:00:01.001 | 92-BSH | Bash Lint (shellheck: 53 files) | 2 | 2 | 0 |
| ✅ | 00:00:00.397 | 93-JSN | JSON Lint (jsonlint: 27 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.132 | 94-JAV | JavaScript Lint (eslint: 0 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.105 | 95-CSS | CSS Lint (stylelint: 0 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.290 | 96-HTM | HTML Lint (htmlhint: 1 files) | 1 | 1 | 0 |
| ✅ | 00:00:02.284 | 98-SIZ | Code Size Analysis (lines: 83,752) | 4 | 4 | 0 |
| ✅ | 00:00:02.012 | 99-COV | Test Suite Coverage (coverage_table) | 4 | 4 | 0 |

## Repository Information

Generated via cloc: 2025-Aug-29 (Fri) 14:12:50 PDT

```cloc
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C                              309          11342          13021          40683
Markdown                       252           7509             55          22445
Bourne Shell                    53           2438           2843          10727
JSON                            27              0              0           2977
C/C++ Header                    86           1068           3461           2877
SVG                              2              0              0           2831
CMake                            1            139            186            719
HTML                             1             74              0            493
-------------------------------------------------------------------------------
SUM:                           731          22570          19566          83752
-------------------------------------------------------------------------------

Code/Docs: 2.5   Code/Comments: 2.8   Instrumented Code:  13,073   Ratio: 37.09
Docs/Code: 0.4   Comments/Code: 0.4   Instrumented Test:  16,160   Unity: 123.61
```
