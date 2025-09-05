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

Generated on: 2025-Sep-05 (Fri) 13:25:04 PDT

### Summary

| Metric | Value |
| ------ | ----- |
| Total Tests | 30 |
| Passed | 30 |
| Failed | 0 |
| Total Subtests | 422 |
| Passed Subtests | 422 |
| Failed Subtests | 0 |
| Elapsed Time | 00:00:28.644 |
| Cumulative Time | 00:00:50.717 |

[Test Suite Results](COMPLETE.svg) | [Test Suite Coverage](COVERAGE.svg)

### Test Coverage

| Test Type | Files Cover | Files Instr | Lines Cover | Lines Instr | Coverage |
| --------- | ----------- | ----------- | ----------- | ----------- | -------- |
| Unity Tests | 99 | 137 | 4,994 | 15,298 | 32.645% |
| Blackbox Tests | 118 | 137 | 8,704 | 15,298 | 56.896% |
| Combined Tests | 119 | 137 | 9,746 | 15,298 | 63.708% |

### Individual Test Results

| Status | Time | Test | Test Name | Tests | Pass | Fail |
| ------ | ---- | -- | ---- | ----- | ---- | ---- |
| ✅ | 00:00:08.231 | 01-CMP | Compilation (source code: 408 files) | 17 | 17 | 0 |
| ✅ | 00:00:00.422 | 02-ENV | Environment Variables (secrets: 3) | 5 | 5 | 0 |
| ✅ | 00:00:03.481 | 04-LNK | Markdown Links Check (github-sitemap) | 3 | 3 | 0 |
| ✅ | 00:00:05.966 | 10-UNT | Unity (1,338 / 1,338 unit tests passed) | 173 | 173 | 0 |
| ✅ | 00:00:00.705 | 11-SIV | Memory Leak Detection | 4 | 4 | 0 |
| ✅ | 00:00:01.572 | 12-VAR | Env Var Substitution | 15 | 15 | 0 |
| ✅ | 00:00:01.515 | 13-BUG | Crash Handler | 37 | 37 | 0 |
| ✅ | 00:00:00.925 | 14-DEP | Library Dependencies | 17 | 17 | 0 |
| ✅ | 00:00:00.396 | 15-JSN | JSON Error Handling | 4 | 4 | 0 |
| ✅ | 00:00:00.595 | 16-SHD | Shutdown | 5 | 5 | 0 |
| ✅ | 00:00:01.290 | 17-UPD | Startup/Shutdown | 9 | 9 | 0 |
| ✅ | 00:00:04.394 | 18-SIG | Signal Handling | 9 | 9 | 0 |
| ✅ | 00:00:01.263 | 19-SCK | Socket Rebinding | 8 | 8 | 0 |
| ✅ | 00:00:00.886 | 20-PRE | Prefix | 11 | 11 | 0 |
| ✅ | 00:00:01.123 | 21-SYS | Endpoints | 11 | 11 | 0 |
| ✅ | 00:00:01.099 | 22-SWG | Swagger | 15 | 15 | 0 |
| ✅ | 00:00:01.377 | 23-WSS | WebSockets | 13 | 13 | 0 |
| ✅ | 00:00:01.291 | 24-UPL | Uploads | 13 | 13 | 0 |
| ✅ | 00:00:02.674 | 25-DNS | mDNS | 7 | 7 | 0 |
| ✅ | 00:00:01.089 | 26-TRM | Terminal | 17 | 17 | 0 |
| ✅ | 00:00:01.200 | 27-DBS | Databases | 13 | 13 | 0 |
| ✅ | 00:00:01.134 | 90-MKD | Markdown Lint (markdownlint: 254 files) | 1 | 1 | 0 |
| ✅ | 00:00:01.188 | 91-GCC | C Lint (cppcheck: 410 files) | 1 | 1 | 0 |
| ✅ | 00:00:01.061 | 92-BSH | Bash Lint (shellheck: 60 files) | 2 | 2 | 0 |
| ✅ | 00:00:00.280 | 93-JSN | JSON Lint (jsonlint: 35 files) | 1 | 1 | 0 |
| ✅ | 00:00:01.601 | 94-JAV | JavaScript Lint (eslint: 3 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.604 | 95-CSS | CSS Lint (stylelint: 3 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.263 | 96-HTM | HTML Lint (htmlhint: 6 files) | 1 | 1 | 0 |
| ✅ | 00:00:02.890 | 98-SIZ | Code Size Analysis (lines: 91,224) | 4 | 4 | 0 |
| ✅ | 00:00:00.202 | 99-COV | Test Suite Coverage (coverage_table) | 4 | 4 | 0 |

## Repository Information

Generated via cloc: 2025-Sep-05 (Fri) 13:25:04 PDT

```cloc
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C                              317          12095          13975          43737
Markdown                       254           7973             56          23810
Bourne Shell                    60           2786           3148          12476
C/C++ Header                    93           1248           3940           3326
JSON                            35              0              0           3210
SVG                              2              0              0           3015
HTML                             4             98              0            725
CMake                            1            139            186            708
CSS                              2             37             52            178
SQL                              4              4              0             33
JavaScript                       3              0              3              3
Text                             1              0              0              3
-------------------------------------------------------------------------------
SUM:                           776          24380          21360          91224
-------------------------------------------------------------------------------

Code/Docs: 2.5   Code/Comments: 2.8   Instrumented Code:  15,298   Ratio: 33.89
Docs/Code: 0.4   Comments/Code: 0.4   Instrumented Test:  15,950   Unity: 104.26
```
