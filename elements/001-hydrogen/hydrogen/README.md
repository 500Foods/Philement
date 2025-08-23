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

Generated on: 2025-Aug-23 (Sat) 08:51:21 PDT

### Summary

| Metric | Value |
| ------ | ----- |
| Total Tests | 26 |
| Passed | 26 |
| Failed | 0 |
| Total Subtests | 276 |
| Passed Subtests | 276 |
| Failed Subtests | 0 |
| Elapsed Time | 00:00:15.608 |
| Cumulative Time | 00:00:27.807 |

[Test Suite Results](COMPLETE.svg) | [Test Suite Coverage](COVERAGE.svg)

### Test Coverage

| Test Type | Files Cover | Files Instr | Lines Cover | Lines Instr | Coverage |
| --------- | ----------- | ----------- | ----------- | ----------- | -------- |
| Unity Tests | 81 | 123 | 4,164 | 12,045 | 34.570% |
| Blackbox Tests | 100 | 123 | 6,119 | 12,045 | 50.801% |
| Combined Tests | 100 | 123 | 6,824 | 12,045 | 56.654% |

### Individual Test Results

| Status | Time | Test | Test Name | Tests | Pass | Fail |
| ------ | ---- | -- | ---- | ----- | ---- | ---- |
| ✅ | 00:00:05.741 | 01-CMP | Compilation (source code: 297 files) | 16 | 16 | 0 |
| ✅ | 00:00:00.386 | 02-ENV | Environment Variables (secrets: 3) | 5 | 5 | 0 |
| ✅ | 00:00:03.034 | 04-LNK | Markdown Links Check (github-sitemap) | 3 | 3 | 0 |
| ✅ | 00:00:02.269 | 10-UNT | Unity (849 / 849 unit tests passed) | 83 | 83 | 0 |
| ✅ | 00:00:00.626 | 11-SIV | Memory Leak Detection | 4 | 4 | 0 |
| ✅ | 00:00:01.284 | 12-VAR | Env Var Substitution | 15 | 15 | 0 |
| ✅ | 00:00:01.385 | 13-BUG | Crash Handler | 37 | 37 | 0 |
| ✅ | 00:00:00.789 | 14-DEP | Library Dependencies | 13 | 13 | 0 |
| ✅ | 00:00:00.225 | 15-JSN | JSON Error Handling | 4 | 4 | 0 |
| ✅ | 00:00:00.435 | 16-SHD | Shutdown | 5 | 5 | 0 |
| ✅ | 00:00:00.942 | 17-UPD | Startup/Shutdown | 9 | 9 | 0 |
| ✅ | 00:00:00.709 | 18-SIG | Signal Handling | 9 | 9 | 0 |
| ✅ | 00:00:01.160 | 19-SCK | Socket Rebinding | 8 | 8 | 0 |
| ✅ | 00:00:00.572 | 20-PRE | API Prefix | 11 | 11 | 0 |
| ✅ | 00:00:00.681 | 21-SYS | System API | 10 | 10 | 0 |
| ✅ | 00:00:00.518 | 22-SWG | Swagger | 15 | 15 | 0 |
| ✅ | 00:00:01.061 | 23-WSS | WebSockets | 13 | 13 | 0 |
| ✅ | 00:00:01.224 | 90-MKD | Markdown Lint (markdownlint: 249 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.844 | 91-GCC | C Lint (cppcheck: 297 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.833 | 92-BSH | Bash Lint (shellheck: 50 files) | 2 | 2 | 0 |
| ✅ | 00:00:00.201 | 93-JSN | JSON Lint (jsonlint: 21 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.046 | 94-JAV | JavaScript Lint (eslint: 0 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.055 | 95-CSS | CSS Lint (stylelint: 0 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.280 | 96-HTM | HTML Lint (htmlhint: 1 files) | 1 | 1 | 0 |
| ✅ | 00:00:02.182 | 98-SIZ | Code Size Analysis (lines: 71,673) | 4 | 4 | 0 |
| ✅ | 00:00:00.325 | 99-COV | Test Suite Coverage (coverage_table) | 4 | 4 | 0 |

## Repository Information

Generated via cloc: 2025-Aug-23 (Sat) 08:51:21 PDT

```cloc
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C                              213           8172           9448          31295
Markdown                       249           7188             55          21555
Bourne Shell                    50           2187           2612           9618
C/C++ Header                    84           1045           3326           2801
JSON                            21              0              0           2723
SVG                              2              0              0           2473
CMake                            1            139            186            715
HTML                             1             74              0            493
-------------------------------------------------------------------------------
SUM:                           621          18805          15627          71673
-------------------------------------------------------------------------------

Code/Docs: 2.1   Code/Comments: 2.9   Instrumented Code:  12,045   Ratio: 29.60
Docs/Code: 0.5   Comments/Code: 0.4   Instrumented Test:  10,093   Unity: 83.79
```
