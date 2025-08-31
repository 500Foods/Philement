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

Generated on: 2025-Aug-31 (Sun) 16:33:03 PDT

### Summary

| Metric | Value |
| ------ | ----- |
| Total Tests | 29 |
| Passed | 28 |
| Failed | 0 |
| Total Subtests | 400 |
| Passed Subtests | 399 |
| Failed Subtests | 0 |
| Elapsed Time | 00:00:54.674 |
| Cumulative Time | 00:01:06.799 |

[Test Suite Results](COMPLETE.svg) | [Test Suite Coverage](COVERAGE.svg)

### Test Coverage

| Test Type | Files Cover | Files Instr | Lines Cover | Lines Instr | Coverage |
| --------- | ----------- | ----------- | ----------- | ----------- | -------- |
| Unity Tests | 97 | 131 | 5,100 | 13,696 | 37.237% |
| Blackbox Tests | 116 | 131 | 8,174 | 13,696 | 59.682% |
| Combined Tests | 116 | 131 | 9,232 | 13,696 | 67.407% |

### Individual Test Results

| Status | Time | Test | Test Name | Tests | Pass | Fail |
| ------ | ---- | -- | ---- | ----- | ---- | ---- |
| ✅ | 00:00:34.087 | 01-CMP | Compilation (source code: 398 files) | 16 | 16 | 0 |
| ✅ | 00:00:00.394 | 02-ENV | Environment Variables (secrets: 3) | 5 | 5 | 0 |
| ✅ | 00:00:02.448 | 04-LNK | Markdown Links Check (github-sitemap) | 3 | 3 | 0 |
| ✅ | 00:00:07.106 | 10-UNT | Unity (1,345 / 1,345 unit tests passed) | 174 | 174 | 0 |
| ✅ | 00:00:00.580 | 11-SIV | Memory Leak Detection | 4 | 4 | 0 |
| ✅ | 00:00:01.147 | 12-VAR | Env Var Substitution | 15 | 15 | 0 |
| ✅ | 00:00:01.271 | 13-BUG | Crash Handler | 37 | 37 | 0 |
| ✅ | 00:00:00.743 | 14-DEP | Library Dependencies | 13 | 13 | 0 |
| ✅ | 00:00:00.300 | 15-JSN | JSON Error Handling | 4 | 4 | 0 |
| ✅ | 00:00:00.471 | 16-SHD | Shutdown | 5 | 5 | 0 |
| ✅ | 00:00:01.019 | 17-UPD | Startup/Shutdown | 9 | 9 | 0 |
| ✅ | 00:00:00.643 | 18-SIG | Signal Handling | 9 | 9 | 0 |
| ✅ | 00:00:01.029 | 19-SCK | Socket Rebinding | 8 | 8 | 0 |
| ✅ | 00:00:00.626 | 20-PRE | API Prefix | 11 | 11 | 0 |
| ✅ | 00:00:00.811 | 21-SYS | System API | 11 | 11 | 0 |
| ✅ | 00:00:00.779 | 22-SWG | Swagger | 15 | 15 | 0 |
| ✅ | 00:00:01.322 | 23-WSS | WebSockets | 13 | 13 | 0 |
| ✅ | 00:00:01.060 | 24-UPL | Uploads | 13 | 13 | 0 |
| ✅ | 00:00:02.889 | 25-DNS | mDNS | 7 | 7 | 0 |
| ✅ | 00:00:00.649 | 26-TRM | Terminal | 12 | 12 | 0 |
| ❌ | 00:00:01.247 | 90-MKD | Markdown Lint (markdownlint: 253 files) | 1 | 0 | 1 |
| ✅ | 00:00:01.067 | 91-GCC | C Lint (cppcheck: 399 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.739 | 92-BSH | Bash Lint (shellheck: 55 files) | 2 | 2 | 0 |
| ✅ | 00:00:00.327 | 93-JSN | JSON Lint (jsonlint: 30 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.038 | 94-JAV | JavaScript Lint (eslint: 0 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.042 | 95-CSS | CSS Lint (stylelint: 0 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.299 | 96-HTM | HTML Lint (htmlhint: 4 files) | 1 | 1 | 0 |
| ✅ | 00:00:02.080 | 98-SIZ | Code Size Analysis (lines: 86,290) | 4 | 4 | 0 |
| ✅ | 00:00:01.586 | 99-COV | Test Suite Coverage (coverage_table) | 4 | 4 | 0 |

## Repository Information

Generated via cloc: 2025-Aug-31 (Sun) 16:33:03 PDT

```cloc
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C                              311          11552          13232          41539
Markdown                       253           7750             56          23148
Bourne Shell                    55           2595           2979          11480
JSON                            30              0              0           3055
C/C++ Header                    88           1091           3517           2953
SVG                              2              0              0           2881
CMake                            1            139            186            719
HTML                             3             74              0            515
-------------------------------------------------------------------------------
SUM:                           743          23201          19970          86290
-------------------------------------------------------------------------------

Code/Docs: 2.4   Code/Comments: 2.8   Instrumented Code:  13,696   Ratio: 36.15
Docs/Code: 0.4   Comments/Code: 0.4   Instrumented Test:  16,084   Unity: 117.43
```
