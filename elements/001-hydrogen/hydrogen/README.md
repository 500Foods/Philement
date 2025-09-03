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

Generated on: 2025-Sep-03 (Wed) 05:14:52 PDT

### Summary

| Metric | Value |
| ------ | ----- |
| Total Tests | 29 |
| Passed | 29 |
| Failed | 0 |
| Total Subtests | 404 |
| Passed Subtests | 404 |
| Failed Subtests | 0 |
| Elapsed Time | 00:00:25.639 |
| Cumulative Time | 00:00:42.506 |

[Test Suite Results](COMPLETE.svg) | [Test Suite Coverage](COVERAGE.svg)

### Test Coverage

| Test Type | Files Cover | Files Instr | Lines Cover | Lines Instr | Coverage |
| --------- | ----------- | ----------- | ----------- | ----------- | -------- |
| Unity Tests | 99 | 135 | 5,088 | 14,322 | 35.526% |
| Blackbox Tests | 118 | 135 | 8,324 | 14,322 | 58.120% |
| Combined Tests | 119 | 135 | 9,373 | 14,322 | 65.445% |

### Individual Test Results

| Status | Time | Test | Test Name | Tests | Pass | Fail |
| ------ | ---- | -- | ---- | ----- | ---- | ---- |
| ✅ | 00:00:06.859 | 01-CMP | Compilation (source code: 404 files) | 16 | 16 | 0 |
| ✅ | 00:00:00.302 | 02-ENV | Environment Variables (secrets: 3) | 5 | 5 | 0 |
| ✅ | 00:00:05.245 | 04-LNK | Markdown Links Check (github-sitemap) | 3 | 3 | 0 |
| ✅ | 00:00:04.220 | 10-UNT | Unity (1,338 / 1,338 unit tests passed) | 173 | 173 | 0 |
| ✅ | 00:00:00.483 | 11-SIV | Memory Leak Detection | 4 | 4 | 0 |
| ✅ | 00:00:01.101 | 12-VAR | Env Var Substitution | 15 | 15 | 0 |
| ✅ | 00:00:01.252 | 13-BUG | Crash Handler | 37 | 37 | 0 |
| ✅ | 00:00:00.665 | 14-DEP | Library Dependencies | 13 | 13 | 0 |
| ✅ | 00:00:00.107 | 15-JSN | JSON Error Handling | 4 | 4 | 0 |
| ✅ | 00:00:00.412 | 16-SHD | Shutdown | 5 | 5 | 0 |
| ✅ | 00:00:01.004 | 17-UPD | Startup/Shutdown | 9 | 9 | 0 |
| ✅ | 00:00:00.573 | 18-SIG | Signal Handling | 9 | 9 | 0 |
| ✅ | 00:00:01.024 | 19-SCK | Socket Rebinding | 8 | 8 | 0 |
| ✅ | 00:00:00.603 | 20-PRE | Prefix | 11 | 11 | 0 |
| ✅ | 00:00:00.719 | 21-SYS | Endpoints | 11 | 11 | 0 |
| ✅ | 00:00:00.768 | 22-SWG | Swagger | 15 | 15 | 0 |
| ✅ | 00:00:01.080 | 23-WSS | WebSockets | 13 | 13 | 0 |
| ✅ | 00:00:00.931 | 24-UPL | Uploads | 13 | 13 | 0 |
| ✅ | 00:00:03.236 | 25-DNS | mDNS | 7 | 7 | 0 |
| ✅ | 00:00:00.782 | 26-TRM | Terminal | 17 | 17 | 0 |
| ✅ | 00:00:01.429 | 90-MKD | Markdown Lint (markdownlint: 254 files) | 1 | 1 | 0 |
| ✅ | 00:00:01.213 | 91-GCC | C Lint (cppcheck: 405 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.968 | 92-BSH | Bash Lint (shellheck: 55 files) | 2 | 2 | 0 |
| ✅ | 00:00:00.887 | 93-JSN | JSON Lint (jsonlint: 30 files) | 1 | 1 | 0 |
| ✅ | 00:00:01.573 | 94-JAV | JavaScript Lint (eslint: 3 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.774 | 95-CSS | CSS Lint (stylelint: 3 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.329 | 96-HTM | HTML Lint (htmlhint: 6 files) | 1 | 1 | 0 |
| ✅ | 00:00:03.740 | 98-SIZ | Code Size Analysis (lines: 88,488) | 4 | 4 | 0 |
| ✅ | 00:00:00.227 | 99-COV | Test Suite Coverage (coverage_table) | 4 | 4 | 0 |

## Repository Information

Generated via cloc: 2025-Sep-03 (Wed) 05:14:52 PDT

```cloc
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C                              314          11777          13715          42244
Markdown                       254           7939             56          23763
Bourne Shell                    55           2635           2994          11779
C/C++ Header                    91           1154           3822           3071
JSON                            30              0              0           3055
SVG                              2              0              0           2959
HTML                             4             98              0            725
CMake                            1            139            186            708
CSS                              2             37             52            178
JavaScript                       3              0              3              3
Text                             1              0              0              3
-------------------------------------------------------------------------------
SUM:                           757          23779          20828          88488
-------------------------------------------------------------------------------

Code/Docs: 2.4   Code/Comments: 2.8   Instrumented Code:  14,322   Ratio: 35.19
Docs/Code: 0.4   Comments/Code: 0.4   Instrumented Test:  15,950   Unity: 111.36
```
