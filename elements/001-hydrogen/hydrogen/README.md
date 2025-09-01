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

Generated on: 2025-Sep-01 (Mon) 02:08:59 PDT

### Summary

| Metric | Value |
| ------ | ----- |
| Total Tests | 29 |
| Passed | 28 |
| Failed | 0 |
| Total Subtests | 399 |
| Passed Subtests | 398 |
| Failed Subtests | 0 |
| Elapsed Time | 00:00:28.630 |
| Cumulative Time | 00:00:38.769 |

[Test Suite Results](COMPLETE.svg) | [Test Suite Coverage](COVERAGE.svg)

### Test Coverage

| Test Type | Files Cover | Files Instr | Lines Cover | Lines Instr | Coverage |
| --------- | ----------- | ----------- | ----------- | ----------- | -------- |
| Unity Tests | 97 | 134 | 5,068 | 14,091 | 35.966% |
| Blackbox Tests | 116 | 134 | 8,210 | 14,091 | 58.264% |
| Combined Tests | 116 | 134 | 9,227 | 14,091 | 65.482% |

### Individual Test Results

| Status | Time | Test | Test Name | Tests | Pass | Fail |
| ------ | ---- | -- | ---- | ----- | ---- | ---- |
| ✅ | 00:00:08.349 | 01-CMP | Compilation (source code: 402 files) | 16 | 16 | 0 |
| ✅ | 00:00:00.369 | 02-ENV | Environment Variables (secrets: 3) | 5 | 5 | 0 |
| ✅ | 00:00:03.004 | 04-LNK | Markdown Links Check (github-sitemap) | 3 | 3 | 0 |
| ✅ | 00:00:05.459 | 10-UNT | Unity (1,325 / 1,325 unit tests passed) | 173 | 173 | 0 |
| ✅ | 00:00:00.478 | 11-SIV | Memory Leak Detection | 4 | 4 | 0 |
| ✅ | 00:00:01.156 | 12-VAR | Env Var Substitution | 15 | 15 | 0 |
| ✅ | 00:00:01.340 | 13-BUG | Crash Handler | 37 | 37 | 0 |
| ✅ | 00:00:00.755 | 14-DEP | Library Dependencies | 13 | 13 | 0 |
| ✅ | 00:00:00.189 | 15-JSN | JSON Error Handling | 4 | 4 | 0 |
| ✅ | 00:00:00.525 | 16-SHD | Shutdown | 5 | 5 | 0 |
| ✅ | 00:00:00.932 | 17-UPD | Startup/Shutdown | 9 | 9 | 0 |
| ✅ | 00:00:00.658 | 18-SIG | Signal Handling | 9 | 9 | 0 |
| ✅ | 00:00:01.049 | 19-SCK | Socket Rebinding | 8 | 8 | 0 |
| ✅ | 00:00:00.605 | 20-PRE | API Prefix | 11 | 11 | 0 |
| ✅ | 00:00:00.692 | 21-SYS | System Endpoints | 11 | 11 | 0 |
| ✅ | 00:00:00.745 | 22-SWG | Swagger | 15 | 15 | 0 |
| ✅ | 00:00:01.111 | 23-WSS | WebSockets | 13 | 13 | 0 |
| ✅ | 00:00:00.955 | 24-UPL | Uploads | 13 | 13 | 0 |
| ✅ | 00:00:04.108 | 25-DNS | mDNS | 7 | 7 | 0 |
| ✅ | 00:00:00.654 | 26-TRM | Terminal | 12 | 12 | 0 |
| ❌ | 00:00:01.145 | 90-MKD | Markdown Lint (markdownlint: 253 files) | 1 | 0 | 1 |
| ✅ | 00:00:01.026 | 91-GCC | C Lint (cppcheck: 403 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.630 | 92-BSH | Bash Lint (shellheck: 55 files) | 2 | 2 | 0 |
| ✅ | 00:00:00.216 | 93-JSN | JSON Lint (jsonlint: 30 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.059 | 94-JAV | JavaScript Lint (eslint: 0 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.054 | 95-CSS | CSS Lint (stylelint: 0 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.222 | 96-HTM | HTML Lint (htmlhint: 4 files) | 1 | 1 | 0 |
| ✅ | 00:00:02.132 | 98-SIZ | Code Size Analysis (lines: 86,669) | 4 | 4 | 0 |
| ✅ | 00:00:00.152 | 99-COV | Test Suite Coverage (coverage_table) | 4 | 4 | 0 |

## Repository Information

Generated via cloc: 2025-Sep-01 (Mon) 02:08:59 PDT

```cloc
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C                              312          11613          13391          41711
Markdown                       253           7739             56          23099
Bourne Shell                    55           2601           2960          11558
C/C++ Header                    91           1154           3822           3070
JSON                            30              0              0           3055
SVG                              2              0              0           2936
CMake                            1            139            186            725
HTML                             3             74              0            515
-------------------------------------------------------------------------------
SUM:                           747          23320          20415          86669
-------------------------------------------------------------------------------

Code/Docs: 2.5   Code/Comments: 2.8   Instrumented Code:  14,091   Ratio: 35.21
Docs/Code: 0.4   Comments/Code: 0.4   Instrumented Test:  15,769   Unity: 111.90
```
