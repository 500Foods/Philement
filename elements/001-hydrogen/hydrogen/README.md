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

Generated on: 2025-Aug-27 (Wed) 02:09:13 PDT

### Summary

| Metric | Value |
| ------ | ----- |
| Total Tests | 27 |
| Passed | 27 |
| Failed | 0 |
| Total Subtests | 369 |
| Passed Subtests | 369 |
| Failed Subtests | 0 |
| Elapsed Time | 00:00:53.344 |
| Cumulative Time | 00:01:07.542 |

[Test Suite Results](COMPLETE.svg) | [Test Suite Coverage](COVERAGE.svg)

### Test Coverage

| Test Type | Files Cover | Files Instr | Lines Cover | Lines Instr | Coverage |
| --------- | ----------- | ----------- | ----------- | ----------- | -------- |
| Unity Tests | 96 | 127 | 4,664 | 12,807 | 36.418% |
| Blackbox Tests | 110 | 127 | 7,283 | 12,807 | 56.867% |
| Combined Tests | 112 | 127 | 8,490 | 12,807 | 66.292% |

### Individual Test Results

| Status | Time | Test | Test Name | Tests | Pass | Fail |
| ------ | ---- | -- | ---- | ----- | ---- | ---- |
| ✅ | 00:00:34.640 | 01-CMP | Compilation (source code: 381 files) | 16 | 16 | 0 |
| ✅ | 00:00:00.367 | 02-ENV | Environment Variables (secrets: 3) | 5 | 5 | 0 |
| ✅ | 00:00:02.514 | 04-LNK | Markdown Links Check (github-sitemap) | 3 | 3 | 0 |
| ✅ | 00:00:08.873 | 10-UNT | Unity (1,244 / 1,244 unit tests passed) | 163 | 163 | 0 |
| ✅ | 00:00:00.645 | 11-SIV | Memory Leak Detection | 4 | 4 | 0 |
| ✅ | 00:00:01.415 | 12-VAR | Env Var Substitution | 15 | 15 | 0 |
| ✅ | 00:00:01.585 | 13-BUG | Crash Handler | 37 | 37 | 0 |
| ✅ | 00:00:00.984 | 14-DEP | Library Dependencies | 13 | 13 | 0 |
| ✅ | 00:00:00.279 | 15-JSN | JSON Error Handling | 4 | 4 | 0 |
| ✅ | 00:00:00.540 | 16-SHD | Shutdown | 5 | 5 | 0 |
| ✅ | 00:00:00.969 | 17-UPD | Startup/Shutdown | 9 | 9 | 0 |
| ✅ | 00:00:00.874 | 18-SIG | Signal Handling | 9 | 9 | 0 |
| ✅ | 00:00:01.296 | 19-SCK | Socket Rebinding | 8 | 8 | 0 |
| ✅ | 00:00:00.624 | 20-PRE | API Prefix | 11 | 11 | 0 |
| ✅ | 00:00:00.750 | 21-SYS | System API | 10 | 10 | 0 |
| ✅ | 00:00:00.632 | 22-SWG | Swagger | 15 | 15 | 0 |
| ✅ | 00:00:01.086 | 23-WSS | WebSockets | 13 | 13 | 0 |
| ✅ | 00:00:00.979 | 24-UPL | Uploads | 13 | 13 | 0 |
| ✅ | 00:00:01.484 | 90-MKD | Markdown Lint (markdownlint: 251 files) | 1 | 1 | 0 |
| ✅ | 00:00:01.326 | 91-GCC | C Lint (cppcheck: 382 files) | 1 | 1 | 0 |
| ✅ | 00:00:01.104 | 92-BSH | Bash Lint (shellheck: 52 files) | 2 | 2 | 0 |
| ✅ | 00:00:00.315 | 93-JSN | JSON Lint (jsonlint: 26 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.045 | 94-JAV | JavaScript Lint (eslint: 0 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.045 | 95-CSS | CSS Lint (stylelint: 0 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.447 | 96-HTM | HTML Lint (htmlhint: 1 files) | 1 | 1 | 0 |
| ✅ | 00:00:01.976 | 98-SIZ | Code Size Analysis (lines: 80,470) | 4 | 4 | 0 |
| ✅ | 00:00:01.748 | 99-COV | Test Suite Coverage (coverage_table) | 4 | 4 | 0 |

## Repository Information

Generated via cloc: 2025-Aug-27 (Wed) 02:09:13 PDT

```cloc
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C                              297          10582          12401          38396
Markdown                       251           7388             55          22093
Bourne Shell                    52           2326           2736          10251
JSON                            26              0              0           2877
C/C++ Header                    85           1056           3422           2857
SVG                              2              0              0           2784
CMake                            1            139            186            719
HTML                             1             74              0            493
-------------------------------------------------------------------------------
SUM:                           715          21565          18800          80470
-------------------------------------------------------------------------------

Code/Docs: 2.4   Code/Comments: 2.8   Instrumented Code:  12,807   Ratio: 35.46
Docs/Code: 0.4   Comments/Code: 0.4   Instrumented Test:  14,629   Unity: 114.22
```
