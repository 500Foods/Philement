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

Generated on: 2025-Sep-03 (Wed) 16:41:01 PDT

### Summary

| Metric | Value |
| ------ | ----- |
| Total Tests | 29 |
| Passed | 28 |
| Failed | 0 |
| Total Subtests | 408 |
| Passed Subtests | 407 |
| Failed Subtests | 0 |
| Elapsed Time | 00:00:28.846 |
| Cumulative Time | 00:00:50.862 |

[Test Suite Results](COMPLETE.svg) | [Test Suite Coverage](COVERAGE.svg)

### Test Coverage

| Test Type | Files Cover | Files Instr | Lines Cover | Lines Instr | Coverage |
| --------- | ----------- | ----------- | ----------- | ----------- | -------- |
| Unity Tests | 99 | 135 | 4,991 | 14,390 | 34.684% |
| Blackbox Tests | 118 | 135 | 8,380 | 14,390 | 58.235% |
| Combined Tests | 119 | 135 | 9,422 | 14,390 | 65.476% |

### Individual Test Results

| Status | Time | Test | Test Name | Tests | Pass | Fail |
| ------ | ---- | -- | ---- | ----- | ---- | ---- |
| ✅ | 00:00:06.904 | 01-CMP | Compilation (source code: 404 files) | 16 | 16 | 0 |
| ✅ | 00:00:00.401 | 02-ENV | Environment Variables (secrets: 3) | 5 | 5 | 0 |
| ✅ | 00:00:04.858 | 04-LNK | Markdown Links Check (github-sitemap) | 3 | 3 | 0 |
| ✅ | 00:00:06.186 | 10-UNT | Unity (1,338 / 1,338 unit tests passed) | 173 | 173 | 0 |
| ✅ | 00:00:00.699 | 11-SIV | Memory Leak Detection | 4 | 4 | 0 |
| ✅ | 00:00:01.535 | 12-VAR | Env Var Substitution | 15 | 15 | 0 |
| ✅ | 00:00:01.478 | 13-BUG | Crash Handler | 37 | 37 | 0 |
| ✅ | 00:00:00.877 | 14-DEP | Library Dependencies | 17 | 17 | 0 |
| ✅ | 00:00:00.369 | 15-JSN | JSON Error Handling | 4 | 4 | 0 |
| ✅ | 00:00:00.581 | 16-SHD | Shutdown | 5 | 5 | 0 |
| ✅ | 00:00:01.146 | 17-UPD | Startup/Shutdown | 9 | 9 | 0 |
| ✅ | 00:00:04.251 | 18-SIG | Signal Handling | 9 | 9 | 0 |
| ✅ | 00:00:01.212 | 19-SCK | Socket Rebinding | 8 | 8 | 0 |
| ✅ | 00:00:00.739 | 20-PRE | Prefix | 11 | 11 | 0 |
| ✅ | 00:00:00.930 | 21-SYS | Endpoints | 11 | 11 | 0 |
| ✅ | 00:00:00.925 | 22-SWG | Swagger | 15 | 15 | 0 |
| ✅ | 00:00:01.625 | 23-WSS | WebSockets | 13 | 13 | 0 |
| ✅ | 00:00:01.140 | 24-UPL | Uploads | 13 | 13 | 0 |
| ✅ | 00:00:03.899 | 25-DNS | mDNS | 7 | 7 | 0 |
| ✅ | 00:00:00.887 | 26-TRM | Terminal | 17 | 17 | 0 |
| ✅ | 00:00:01.347 | 90-MKD | Markdown Lint (markdownlint: 254 files) | 1 | 1 | 0 |
| ✅ | 00:00:01.029 | 91-GCC | C Lint (cppcheck: 405 files) | 1 | 1 | 0 |
| ❌ | 00:00:01.127 | 92-BSH | Bash Lint (shellheck: 59 files) | 2 | 1 | 1 |
| ✅ | 00:00:00.695 | 93-JSN | JSON Lint (jsonlint: 30 files) | 1 | 1 | 0 |
| ✅ | 00:00:01.469 | 94-JAV | JavaScript Lint (eslint: 3 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.574 | 95-CSS | CSS Lint (stylelint: 3 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.249 | 96-HTM | HTML Lint (htmlhint: 6 files) | 1 | 1 | 0 |
| ✅ | 00:00:03.531 | 98-SIZ | Code Size Analysis (lines: 89,222) | 4 | 4 | 0 |
| ✅ | 00:00:00.199 | 99-COV | Test Suite Coverage (coverage_table) | 4 | 4 | 0 |

## Repository Information

Generated via cloc: 2025-Sep-03 (Wed) 16:41:01 PDT

```cloc
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C                              314          11792          13727          42340
Markdown                       254           7984             56          23905
Bourne Shell                    59           2720           3062          12240
C/C++ Header                    91           1154           3822           3071
JSON                            30              0              0           3055
SVG                              2              0              0           2961
HTML                             4             98              0            725
CMake                            1            139            186            708
CSS                              2             37             52            178
SQL                              4              4              0             33
JavaScript                       3              0              3              3
Text                             1              0              0              3
-------------------------------------------------------------------------------
SUM:                           765          23928          20908          89222
-------------------------------------------------------------------------------

Code/Docs: 2.4   Code/Comments: 2.8   Instrumented Code:  14,390   Ratio: 35.12
Docs/Code: 0.4   Comments/Code: 0.4   Instrumented Test:  15,950   Unity: 110.84
```
