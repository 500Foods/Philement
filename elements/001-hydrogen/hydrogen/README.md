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

Generated on: 2025-Aug-28 (Thu) 12:55:54 PDT

### Summary

| Metric | Value |
| ------ | ----- |
| Total Tests | 28 |
| Passed | 27 |
| Failed | 0 |
| Total Subtests | 389 |
| Passed Subtests | 383 |
| Failed Subtests | 0 |
| Elapsed Time | 00:00:27.121 |
| Cumulative Time | 00:00:41.806 |

[Test Suite Results](COMPLETE.svg) | [Test Suite Coverage](COVERAGE.svg)

### Test Coverage

| Test Type | Files Cover | Files Instr | Lines Cover | Lines Instr | Coverage |
| --------- | ----------- | ----------- | ----------- | ----------- | -------- |
| Unity Tests | 96 | 127 | 5,001 | 12,813 | 39.031% |
| Blackbox Tests | 110 | 127 | 7,394 | 12,813 | 57.707% |
| Combined Tests | 112 | 127 | 8,667 | 12,813 | 67.642% |

### Individual Test Results

| Status | Time | Test | Test Name | Tests | Pass | Fail |
| ------ | ---- | -- | ---- | ----- | ---- | ---- |
| ✅ | 00:00:07.500 | 01-CMP | Compilation (source code: 390 files) | 16 | 16 | 0 |
| ✅ | 00:00:00.407 | 02-ENV | Environment Variables (secrets: 3) | 5 | 5 | 0 |
| ✅ | 00:00:03.064 | 04-LNK | Markdown Links Check (github-sitemap) | 3 | 3 | 0 |
| ✅ | 00:00:05.448 | 10-UNT | Unity (1,332 / 1,332 unit tests passed) | 172 | 172 | 0 |
| ✅ | 00:00:00.462 | 11-SIV | Memory Leak Detection | 4 | 4 | 0 |
| ✅ | 00:00:01.127 | 12-VAR | Env Var Substitution | 15 | 15 | 0 |
| ✅ | 00:00:01.319 | 13-BUG | Crash Handler | 37 | 37 | 0 |
| ✅ | 00:00:00.690 | 14-DEP | Library Dependencies | 13 | 13 | 0 |
| ✅ | 00:00:00.157 | 15-JSN | JSON Error Handling | 4 | 4 | 0 |
| ✅ | 00:00:00.417 | 16-SHD | Shutdown | 5 | 5 | 0 |
| ✅ | 00:00:01.009 | 17-UPD | Startup/Shutdown | 9 | 9 | 0 |
| ✅ | 00:00:00.590 | 18-SIG | Signal Handling | 9 | 9 | 0 |
| ✅ | 00:00:01.093 | 19-SCK | Socket Rebinding | 8 | 8 | 0 |
| ✅ | 00:00:00.583 | 20-PRE | API Prefix | 11 | 11 | 0 |
| ✅ | 00:00:00.745 | 21-SYS | System API | 10 | 10 | 0 |
| ✅ | 00:00:00.603 | 22-SWG | Swagger | 15 | 15 | 0 |
| ✅ | 00:00:01.437 | 23-WSS | WebSockets | 13 | 13 | 0 |
| ✅ | 00:00:01.064 | 24-UPL | Uploads | 13 | 13 | 0 |
| ❌ | 00:00:07.559 | 25-MDNS | mDNS Server and Client | 11 | 5 | 6 |
| ✅ | 00:00:01.195 | 90-MKD | Markdown Lint (markdownlint: 252 files) | 1 | 1 | 0 |
| ✅ | 00:00:01.079 | 91-GCC | C Lint (cppcheck: 391 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.878 | 92-BSH | Bash Lint (shellheck: 53 files) | 2 | 2 | 0 |
| ✅ | 00:00:00.316 | 93-JSN | JSON Lint (jsonlint: 27 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.086 | 94-JAV | JavaScript Lint (eslint: 0 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.096 | 95-CSS | CSS Lint (stylelint: 0 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.310 | 96-HTM | HTML Lint (htmlhint: 1 files) | 1 | 1 | 0 |
| ✅ | 00:00:02.248 | 98-SIZ | Code Size Analysis (lines: 82,971) | 4 | 4 | 0 |
| ✅ | 00:00:00.324 | 99-COV | Test Suite Coverage (coverage_table) | 4 | 4 | 0 |

## Repository Information

Generated via cloc: 2025-Aug-28 (Thu) 12:55:54 PDT

```cloc
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C                              306          11191          12853          40173
Markdown                       252           7509             55          22444
Bourne Shell                    53           2387           2792          10537
JSON                            27              0              0           2952
C/C++ Header                    85           1056           3422           2858
SVG                              2              0              0           2795
CMake                            1            139            186            719
HTML                             1             74              0            493
-------------------------------------------------------------------------------
SUM:                           727          22356          19308          82971
-------------------------------------------------------------------------------

Code/Docs: 2.4   Code/Comments: 2.8   Instrumented Code:  12,813   Ratio: 37.36
Docs/Code: 0.4   Comments/Code: 0.4   Instrumented Test:  16,078   Unity: 125.48
```
