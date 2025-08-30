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

Generated on: 2025-Aug-30 (Sat) 09:03:00 PDT

### Summary

| Metric | Value |
| ------ | ----- |
| Total Tests | 28 |
| Passed | 28 |
| Failed | 0 |
| Total Subtests | 387 |
| Passed Subtests | 387 |
| Failed Subtests | 0 |
| Elapsed Time | 00:01:42.715 |
| Cumulative Time | 00:02:06.096 |

[Test Suite Results](COMPLETE.svg) | [Test Suite Coverage](COVERAGE.svg)

### Test Coverage

| Test Type | Files Cover | Files Instr | Lines Cover | Lines Instr | Coverage |
| --------- | ----------- | ----------- | ----------- | ----------- | -------- |
| Unity Tests | 97 | 129 | 5,116 | 13,053 | 39.194% |
| Blackbox Tests | 114 | 129 | 7,948 | 13,053 | 60.890% |
| Combined Tests | 114 | 129 | 9,000 | 13,053 | 68.950% |

### Individual Test Results

| Status | Time | Test | Test Name | Tests | Pass | Fail |
| ------ | ---- | -- | ---- | ----- | ---- | ---- |
| ✅ | 00:01:01.289 | 01-CMP | Compilation (source code: 394 files) | 16 | 16 | 0 |
| ✅ | 00:00:00.401 | 02-ENV | Environment Variables (secrets: 3) | 5 | 5 | 0 |
| ✅ | 00:00:02.596 | 04-LNK | Markdown Links Check (github-sitemap) | 3 | 3 | 0 |
| ✅ | 00:00:08.071 | 10-UNT | Unity (1,345 / 1,345 unit tests passed) | 173 | 173 | 0 |
| ✅ | 00:00:00.628 | 11-SIV | Memory Leak Detection | 4 | 4 | 0 |
| ✅ | 00:00:01.252 | 12-VAR | Env Var Substitution | 15 | 15 | 0 |
| ✅ | 00:00:01.461 | 13-BUG | Crash Handler | 37 | 37 | 0 |
| ✅ | 00:00:00.799 | 14-DEP | Library Dependencies | 13 | 13 | 0 |
| ✅ | 00:00:00.256 | 15-JSN | JSON Error Handling | 4 | 4 | 0 |
| ✅ | 00:00:00.457 | 16-SHD | Shutdown | 5 | 5 | 0 |
| ✅ | 00:00:00.878 | 17-UPD | Startup/Shutdown | 9 | 9 | 0 |
| ✅ | 00:00:00.610 | 18-SIG | Signal Handling | 9 | 9 | 0 |
| ✅ | 00:00:01.041 | 19-SCK | Socket Rebinding | 8 | 8 | 0 |
| ✅ | 00:00:00.598 | 20-PRE | API Prefix | 11 | 11 | 0 |
| ✅ | 00:00:00.744 | 21-SYS | System API | 11 | 11 | 0 |
| ✅ | 00:00:00.604 | 22-SWG | Swagger | 15 | 15 | 0 |
| ✅ | 00:00:01.425 | 23-WSS | WebSockets | 13 | 13 | 0 |
| ✅ | 00:00:01.022 | 24-UPL | Uploads | 13 | 13 | 0 |
| ✅ | 00:00:02.442 | 25-DNS | mDNS | 7 | 7 | 0 |
| ✅ | 00:00:03.584 | 90-MKD | Markdown Lint (markdownlint: 252 files) | 1 | 1 | 0 |
| ✅ | 00:00:22.713 | 91-GCC | C Lint (cppcheck: 395 files) | 1 | 1 | 0 |
| ✅ | 00:00:01.475 | 92-BSH | Bash Lint (shellheck: 53 files) | 2 | 2 | 0 |
| ✅ | 00:00:00.450 | 93-JSN | JSON Lint (jsonlint: 27 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.195 | 94-JAV | JavaScript Lint (eslint: 0 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.175 | 95-CSS | CSS Lint (stylelint: 0 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.439 | 96-HTM | HTML Lint (htmlhint: 1 files) | 1 | 1 | 0 |
| ✅ | 00:00:05.813 | 98-SIZ | Code Size Analysis (lines: 83,775) | 4 | 4 | 0 |
| ✅ | 00:00:04.678 | 99-COV | Test Suite Coverage (coverage_table) | 4 | 4 | 0 |

## Repository Information

Generated via cloc: 2025-Aug-30 (Sat) 09:03:00 PDT

```cloc
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C                              309          11344          13021          40676
Markdown                       252           7509             55          22445
Bourne Shell                    53           2438           2844          10728
JSON                            27              0              0           2977
C/C++ Header                    86           1071           3464           2908
SVG                              2              0              0           2829
CMake                            1            139            186            719
HTML                             1             74              0            493
-------------------------------------------------------------------------------
SUM:                           731          22575          19570          83775
-------------------------------------------------------------------------------

Code/Docs: 2.5   Code/Comments: 2.8   Instrumented Code:  13,053   Ratio: 36.90
Docs/Code: 0.4   Comments/Code: 0.4   Instrumented Test:  16,084   Unity: 123.22
```
