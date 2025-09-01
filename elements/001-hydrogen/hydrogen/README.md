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

Generated on: 2025-Aug-31 (Sun) 17:33:06 PDT

### Summary

| Metric | Value |
| ------ | ----- |
| Total Tests | 29 |
| Passed | 26 |
| Failed | 0 |
| Total Subtests | 389 |
| Passed Subtests | 385 |
| Failed Subtests | 0 |
| Elapsed Time | 00:00:54.238 |
| Cumulative Time | 00:01:09.167 |

[Test Suite Results](COMPLETE.svg) | [Test Suite Coverage](COVERAGE.svg)

### Test Coverage

| Test Type | Files Cover | Files Instr | Lines Cover | Lines Instr | Coverage |
| --------- | ----------- | ----------- | ----------- | ----------- | -------- |
| Unity Tests | 97 | 131 | 5,068 | 13,517 | 37.494% |
| Blackbox Tests | 116 | 131 | 8,206 | 13,517 | 60.709% |
| Combined Tests | 116 | 131 | 9,223 | 13,517 | 68.233% |

### Individual Test Results

| Status | Time | Test | Test Name | Tests | Pass | Fail |
| ------ | ---- | -- | ---- | ----- | ---- | ---- |
| ✅ | 00:00:34.159 | 01-CMP | Compilation (source code: 396 files) | 16 | 16 | 0 |
| ✅ | 00:00:00.344 | 02-ENV | Environment Variables (secrets: 3) | 5 | 5 | 0 |
| ✅ | 00:00:02.504 | 04-LNK | Markdown Links Check (github-sitemap) | 3 | 3 | 0 |
| ✅ | 00:00:07.551 | 10-UNT | Unity (1,325 / 1,325 unit tests passed) | 171 | 171 | 0 |
| ✅ | 00:00:00.590 | 11-SIV | Memory Leak Detection | 4 | 4 | 0 |
| ✅ | 00:00:01.163 | 12-VAR | Env Var Substitution | 15 | 15 | 0 |
| ✅ | 00:00:01.335 | 13-BUG | Crash Handler | 37 | 37 | 0 |
| ✅ | 00:00:00.751 | 14-DEP | Library Dependencies | 13 | 13 | 0 |
| ✅ | 00:00:00.281 | 15-JSN | JSON Error Handling | 4 | 4 | 0 |
| ✅ | 00:00:00.504 | 16-SHD | Shutdown | 5 | 5 | 0 |
| ✅ | 00:00:01.025 | 17-UPD | Startup/Shutdown | 9 | 9 | 0 |
| ✅ | 00:00:00.604 | 18-SIG | Signal Handling | 9 | 9 | 0 |
| ✅ | 00:00:01.062 | 19-SCK | Socket Rebinding | 8 | 8 | 0 |
| ✅ | 00:00:00.628 | 20-PRE | API Prefix | 11 | 11 | 0 |
| ✅ | 00:00:00.846 | 21-SYS | System API | 11 | 11 | 0 |
| ❌ | 00:00:00.836 | 22-SWG | Swagger | 7 | 5 | 2 |
| ✅ | 00:00:01.331 | 23-WSS | WebSockets | 13 | 13 | 0 |
| ✅ | 00:00:01.038 | 24-UPL | Uploads | 13 | 13 | 0 |
| ✅ | 00:00:02.435 | 25-DNS | mDNS | 7 | 7 | 0 |
| ✅ | 00:00:00.676 | 26-TRM | Terminal | 12 | 12 | 0 |
| ❌ | 00:00:01.548 | 90-MKD | Markdown Lint (markdownlint: 253 files) | 1 | 0 | 1 |
| ✅ | 00:00:01.441 | 91-GCC | C Lint (cppcheck: 397 files) | 1 | 1 | 0 |
| ❌ | 00:00:01.028 | 92-BSH | Bash Lint (shellheck: 55 files) | 2 | 1 | 1 |
| ✅ | 00:00:00.585 | 93-JSN | JSON Lint (jsonlint: 30 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.130 | 94-JAV | JavaScript Lint (eslint: 0 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.201 | 95-CSS | CSS Lint (stylelint: 0 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.508 | 96-HTM | HTML Lint (htmlhint: 4 files) | 1 | 1 | 0 |
| ✅ | 00:00:02.156 | 98-SIZ | Code Size Analysis (lines: 85,683) | 4 | 4 | 0 |
| ✅ | 00:00:01.907 | 99-COV | Test Suite Coverage (coverage_table) | 4 | 4 | 0 |

## Repository Information

Generated via cloc: 2025-Aug-31 (Sun) 17:33:06 PDT

```cloc
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C                              309          11386          13065          40863
Markdown                       253           7750             56          23148
Bourne Shell                    55           2599           2955          11549
JSON                            30              0              0           3055
C/C++ Header                    88           1091           3517           2953
SVG                              2              0              0           2881
CMake                            1            139            186            719
HTML                             3             74              0            515
-------------------------------------------------------------------------------
SUM:                           741          23039          19779          85683
-------------------------------------------------------------------------------

Code/Docs: 2.4   Code/Comments: 2.8   Instrumented Code:  13,517   Ratio: 35.98
Docs/Code: 0.4   Comments/Code: 0.4   Instrumented Test:  15,769   Unity: 116.66
```
