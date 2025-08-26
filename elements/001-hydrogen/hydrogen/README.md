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

Generated on: 2025-Aug-26 (Tue) 00:06:44 PDT

### Summary

| Metric | Value |
| ------ | ----- |
| Total Tests | 27 |
| Passed | 27 |
| Failed | 0 |
| Total Subtests | 367 |
| Passed Subtests | 367 |
| Failed Subtests | 0 |
| Elapsed Time | 00:00:25.027 |
| Cumulative Time | 00:00:34.272 |

[Test Suite Results](COMPLETE.svg) | [Test Suite Coverage](COVERAGE.svg)

### Test Coverage

| Test Type | Files Cover | Files Instr | Lines Cover | Lines Instr | Coverage |
| --------- | ----------- | ----------- | ----------- | ----------- | -------- |
| Unity Tests | 92 | 127 | 5,194 | 12,534 | 41.439% |
| Blackbox Tests | 106 | 127 | 6,824 | 12,534 | 54.444% |
| Combined Tests | 108 | 127 | 8,078 | 12,534 | 64.449% |

### Individual Test Results

| Status | Time | Test | Test Name | Tests | Pass | Fail |
| ------ | ---- | -- | ---- | ----- | ---- | ---- |
| ✅ | 00:00:08.134 | 01-CMP | Compilation (source code: 381 files) | 16 | 16 | 0 |
| ✅ | 00:00:00.261 | 02-ENV | Environment Variables (secrets: 3) | 5 | 5 | 0 |
| ✅ | 00:00:03.167 | 04-LNK | Markdown Links Check (github-sitemap) | 3 | 3 | 0 |
| ✅ | 00:00:05.201 | 10-UNT | Unity (1,245 / 1,245 unit tests passed) | 163 | 163 | 0 |
| ✅ | 00:00:00.592 | 11-SIV | Memory Leak Detection | 4 | 4 | 0 |
| ✅ | 00:00:01.172 | 12-VAR | Env Var Substitution | 15 | 15 | 0 |
| ✅ | 00:00:01.297 | 13-BUG | Crash Handler | 37 | 37 | 0 |
| ✅ | 00:00:00.672 | 14-DEP | Library Dependencies | 13 | 13 | 0 |
| ✅ | 00:00:00.172 | 15-JSN | JSON Error Handling | 4 | 4 | 0 |
| ✅ | 00:00:00.440 | 16-SHD | Shutdown | 5 | 5 | 0 |
| ✅ | 00:00:00.761 | 17-UPD | Startup/Shutdown | 9 | 9 | 0 |
| ✅ | 00:00:00.595 | 18-SIG | Signal Handling | 9 | 9 | 0 |
| ✅ | 00:00:01.031 | 19-SCK | Socket Rebinding | 8 | 8 | 0 |
| ✅ | 00:00:00.649 | 20-PRE | API Prefix | 11 | 11 | 0 |
| ✅ | 00:00:00.851 | 21-SYS | System API | 10 | 10 | 0 |
| ✅ | 00:00:00.708 | 22-SWG | Swagger | 15 | 15 | 0 |
| ✅ | 00:00:01.457 | 23-WSS | WebSockets | 13 | 13 | 0 |
| ✅ | 00:00:01.001 | 24-UPL | Uploads | 11 | 11 | 0 |
| ✅ | 00:00:01.122 | 90-MKD | Markdown Lint (markdownlint: 250 files) | 1 | 1 | 0 |
| ✅ | 00:00:01.028 | 91-GCC | C Lint (cppcheck: 382 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.702 | 92-BSH | Bash Lint (shellheck: 52 files) | 2 | 2 | 0 |
| ✅ | 00:00:00.319 | 93-JSN | JSON Lint (jsonlint: 25 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.127 | 94-JAV | JavaScript Lint (eslint: 0 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.098 | 95-CSS | CSS Lint (stylelint: 0 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.244 | 96-HTM | HTML Lint (htmlhint: 1 files) | 1 | 1 | 0 |
| ✅ | 00:00:02.292 | 98-SIZ | Code Size Analysis (lines: 79,897) | 4 | 4 | 0 |
| ✅ | 00:00:00.179 | 99-COV | Test Suite Coverage (coverage_table) | 4 | 4 | 0 |

## Repository Information

Generated via cloc: 2025-Aug-26 (Tue) 00:06:44 PDT

```cloc
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C                              297          10530          12341          38129
Markdown                       250           7325             55          21933
Bourne Shell                    52           2301           2706          10121
JSON                            25              0              0           2863
C/C++ Header                    85           1057           3396           2841
SVG                              2              0              0           2796
CMake                            1            139            186            721
HTML                             1             74              0            493
-------------------------------------------------------------------------------
SUM:                           713          21426          18684          79897
-------------------------------------------------------------------------------

Code/Docs: 2.4   Code/Comments: 2.8   Instrumented Code:  12,534   Ratio: 35.70
Docs/Code: 0.4   Comments/Code: 0.4   Instrumented Test:  14,628   Unity: 116.70
```
