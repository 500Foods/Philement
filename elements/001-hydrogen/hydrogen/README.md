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

Generated on: 2025-Aug-26 (Tue) 01:00:33 PDT

### Summary

| Metric | Value |
| ------ | ----- |
| Total Tests | 27 |
| Passed | 27 |
| Failed | 0 |
| Total Subtests | 370 |
| Passed Subtests | 370 |
| Failed Subtests | 0 |
| Elapsed Time | 00:00:24.110 |
| Cumulative Time | 00:00:33.857 |

[Test Suite Results](COMPLETE.svg) | [Test Suite Coverage](COVERAGE.svg)

### Test Coverage

| Test Type | Files Cover | Files Instr | Lines Cover | Lines Instr | Coverage |
| --------- | ----------- | ----------- | ----------- | ----------- | -------- |
| Unity Tests | 92 | 126 | 5,194 | 12,529 | 41.456% |
| Blackbox Tests | 106 | 126 | 6,827 | 12,529 | 54.490% |
| Combined Tests | 108 | 126 | 8,081 | 12,529 | 64.498% |

### Individual Test Results

| Status | Time | Test | Test Name | Tests | Pass | Fail |
| ------ | ---- | -- | ---- | ----- | ---- | ---- |
| ✅ | 00:00:07.822 | 01-CMP | Compilation (source code: 379 files) | 16 | 16 | 0 |
| ✅ | 00:00:00.105 | 02-ENV | Environment Variables (secrets: 3) | 5 | 5 | 0 |
| ✅ | 00:00:03.101 | 04-LNK | Markdown Links Check (github-sitemap) | 3 | 3 | 0 |
| ✅ | 00:00:05.130 | 10-UNT | Unity (1,245 / 1,245 unit tests passed) | 166 | 166 | 0 |
| ✅ | 00:00:00.522 | 11-SIV | Memory Leak Detection | 4 | 4 | 0 |
| ✅ | 00:00:01.162 | 12-VAR | Env Var Substitution | 15 | 15 | 0 |
| ✅ | 00:00:01.325 | 13-BUG | Crash Handler | 37 | 37 | 0 |
| ✅ | 00:00:00.685 | 14-DEP | Library Dependencies | 13 | 13 | 0 |
| ✅ | 00:00:00.212 | 15-JSN | JSON Error Handling | 4 | 4 | 0 |
| ✅ | 00:00:00.525 | 16-SHD | Shutdown | 5 | 5 | 0 |
| ✅ | 00:00:00.815 | 17-UPD | Startup/Shutdown | 9 | 9 | 0 |
| ✅ | 00:00:00.624 | 18-SIG | Signal Handling | 9 | 9 | 0 |
| ✅ | 00:00:01.070 | 19-SCK | Socket Rebinding | 8 | 8 | 0 |
| ✅ | 00:00:00.598 | 20-PRE | API Prefix | 11 | 11 | 0 |
| ✅ | 00:00:00.719 | 21-SYS | System API | 10 | 10 | 0 |
| ✅ | 00:00:00.601 | 22-SWG | Swagger | 15 | 15 | 0 |
| ✅ | 00:00:01.543 | 23-WSS | WebSockets | 13 | 13 | 0 |
| ✅ | 00:00:01.052 | 24-UPL | Uploads | 11 | 11 | 0 |
| ✅ | 00:00:01.341 | 90-MKD | Markdown Lint (markdownlint: 250 files) | 1 | 1 | 0 |
| ✅ | 00:00:01.071 | 91-GCC | C Lint (cppcheck: 380 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.719 | 92-BSH | Bash Lint (shellheck: 52 files) | 2 | 2 | 0 |
| ✅ | 00:00:00.263 | 93-JSN | JSON Lint (jsonlint: 25 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.100 | 94-JAV | JavaScript Lint (eslint: 0 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.088 | 95-CSS | CSS Lint (stylelint: 0 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.258 | 96-HTM | HTML Lint (htmlhint: 1 files) | 1 | 1 | 0 |
| ✅ | 00:00:02.206 | 98-SIZ | Code Size Analysis (lines: 79,885) | 4 | 4 | 0 |
| ✅ | 00:00:00.200 | 99-COV | Test Suite Coverage (coverage_table) | 4 | 4 | 0 |

## Repository Information

Generated via cloc: 2025-Aug-26 (Tue) 01:00:33 PDT

```cloc
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C                              296          10524          12330          38116
Markdown                       250           7329             55          21948
Bourne Shell                    52           2303           2709          10132
JSON                            25              0              0           2863
C/C++ Header                    84           1053           3389           2835
SVG                              2              0              0           2777
CMake                            1            139            186            721
HTML                             1             74              0            493
-------------------------------------------------------------------------------
SUM:                           711          21422          18669          79885
-------------------------------------------------------------------------------

Code/Docs: 2.4   Code/Comments: 2.8   Instrumented Code:  12,529   Ratio: 35.72
Docs/Code: 0.4   Comments/Code: 0.4   Instrumented Test:  14,628   Unity: 116.75
```
