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

Generated on: 2025-Aug-31 (Sun) 10:19:03 PDT

### Summary

| Metric | Value |
| ------ | ----- |
| Total Tests | 29 |
| Passed | 24 |
| Failed | 0 |
| Total Subtests | 385 |
| Passed Subtests | 377 |
| Failed Subtests | 0 |
| Elapsed Time | 00:01:05.952 |
| Cumulative Time | 00:01:23.754 |

[Test Suite Results](COMPLETE.svg) | [Test Suite Coverage](COVERAGE.svg)

### Test Coverage

| Test Type | Files Cover | Files Instr | Lines Cover | Lines Instr | Coverage |
| --------- | ----------- | ----------- | ----------- | ----------- | -------- |
| Unity Tests | 97 | 131 | 5,078 | 13,516 | 37.570% |
| Blackbox Tests | 116 | 131 | 7,966 | 13,516 | 58.938% |
| Combined Tests | 116 | 131 | 9,029 | 13,516 | 66.802% |

### Individual Test Results

| Status | Time | Test | Test Name | Tests | Pass | Fail |
| ------ | ---- | -- | ---- | ----- | ---- | ---- |
| ✅ | 00:00:36.269 | 01-CMP | Compilation (source code: 398 files) | 16 | 16 | 0 |
| ✅ | 00:00:00.352 | 02-ENV | Environment Variables (secrets: 3) | 5 | 5 | 0 |
| ✅ | 00:00:02.462 | 04-LNK | Markdown Links Check (github-sitemap) | 3 | 3 | 0 |
| ❌ | 00:00:07.968 | 10-UNT | Unity Unit Tests | 173 | 171 | 2 |
| ✅ | 00:00:00.510 | 11-SIV | Memory Leak Detection | 4 | 4 | 0 |
| ✅ | 00:00:01.122 | 12-VAR | Env Var Substitution | 15 | 15 | 0 |
| ✅ | 00:00:01.312 | 13-BUG | Crash Handler | 37 | 37 | 0 |
| ✅ | 00:00:00.731 | 14-DEP | Library Dependencies | 13 | 13 | 0 |
| ✅ | 00:00:00.263 | 15-JSN | JSON Error Handling | 4 | 4 | 0 |
| ✅ | 00:00:00.513 | 16-SHD | Shutdown | 5 | 5 | 0 |
| ✅ | 00:00:01.074 | 17-UPD | Startup/Shutdown | 9 | 9 | 0 |
| ✅ | 00:00:00.615 | 18-SIG | Signal Handling | 9 | 9 | 0 |
| ✅ | 00:00:01.027 | 19-SCK | Socket Rebinding | 8 | 8 | 0 |
| ✅ | 00:00:00.589 | 20-PRE | API Prefix | 11 | 11 | 0 |
| ✅ | 00:00:00.694 | 21-SYS | System API | 11 | 11 | 0 |
| ❌ | 00:00:01.375 | 22-SWG | Swagger | 7 | 5 | 2 |
| ✅ | 00:00:01.120 | 23-WSS | WebSockets | 13 | 13 | 0 |
| ✅ | 00:00:01.003 | 24-UPL | Uploads | 13 | 13 | 0 |
| ✅ | 00:00:02.367 | 25-DNS | mDNS | 7 | 7 | 0 |
| ❌ | 00:00:01.280 | 26-TRM | Terminal | 6 | 4 | 2 |
| ❌ | 00:00:01.638 | 90-MKD | Markdown Lint (markdownlint: 253 files) | 1 | 0 | 1 |
| ✅ | 00:00:02.393 | 91-GCC | C Lint (cppcheck: 399 files) | 1 | 1 | 0 |
| ❌ | 00:00:12.055 | 92-BSH | Bash Lint (shellheck: 55 files) | 2 | 1 | 1 |
| ✅ | 00:00:00.276 | 93-JSN | JSON Lint (jsonlint: 28 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.053 | 94-JAV | JavaScript Lint (eslint: 0 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.041 | 95-CSS | CSS Lint (stylelint: 0 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.332 | 96-HTM | HTML Lint (htmlhint: 3 files) | 1 | 1 | 0 |
| ✅ | 00:00:02.277 | 98-SIZ | Code Size Analysis (lines: 85,877) | 4 | 4 | 0 |
| ✅ | 00:00:02.043 | 99-COV | Test Suite Coverage (coverage_table) | 4 | 4 | 0 |

## Repository Information

Generated via cloc: 2025-Aug-31 (Sun) 10:19:03 PDT

```cloc
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C                              311          11507          13210          41295
Markdown                       253           7750             56          23147
Bourne Shell                    55           2565           2930          11379
JSON                            28              0              0           3002
C/C++ Header                    88           1090           3506           2952
SVG                              2              0              0           2868
CMake                            1            139            186            719
HTML                             3             74              0            515
-------------------------------------------------------------------------------
SUM:                           741          23125          19888          85877
-------------------------------------------------------------------------------

Code/Docs: 2.4   Code/Comments: 2.8   Instrumented Code:  13,516   Ratio: 36.35
Docs/Code: 0.4   Comments/Code: 0.4   Instrumented Test:  16,084   Unity: 118.99
```
