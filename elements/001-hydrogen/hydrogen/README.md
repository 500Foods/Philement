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

Generated on: 2025-Aug-13 (Wed) 01:03:35 PDT

### Summary

| Metric | Value |
| ------ | ----- |
| Total Tests | 26 |
| Passed | 26 |
| Failed | 0 |
| Total Subtests | 209 |
| Passed Subtests | 209 |
| Failed Subtests | 0 |
| Elapsed Time | 00:00:12.284 |
| Cumulative Time | 00:00:26.383 |

[Test Suite Results](COMPLETE.svg) | [Test Suite Coverage](COVERAGE.svg)

### Test Coverage

| Test Type | Files Cover | Files Instr | Lines Cover | Lines Instr | Coverage | Timestamp |
| --------- | ----------- | ----------- | ----------- | ----------- | -------- | --------- |
| Unity Tests | 12 | 121 | 266 | 12,015 | 2.214% | 20250813_010335 |
| Blackbox Tests | 98 | 121 | 6,251 | 12,016 | 52.022% | 20250813_010335 |
| Combined Tests | 98 | 121 | 6,336 | 12,016 | 52.730% | 20250813_010347 |

### Individual Test Results

| Status | Time | Test | Test Name | Tests | Pass | Fail |
| ------ | ---- | -- | ---- | ----- | ---- | ---- |
| ✅ | 00:00:07.621 | 01-CMP | Compilation (source code: 249 files) | 16 | 16 | 0 |
| ✅ | 00:00:00.111 | 02-ENV | Environment Variables (secrets: 3) | 5 | 5 | 0 |
| ✅ | 00:00:01.826 | 03-SIZ | Code Size Analysis (lines: 68,817) | 4 | 4 | 0 |
| ✅ | 00:00:01.904 | 04-LNK | Markdown Links Check (github-sitemap) | 3 | 3 | 0 |
| ✅ | 00:00:01.174 | 10-UNT | Unity (187 / 187 unit tests passed) | 16 | 16 | 0 |
| ✅ | 00:00:00.393 | 11-SIV | Memory Leak Detection | 4 | 4 | 0 |
| ✅ | 00:00:01.136 | 12-VAR | Env Var Substitution | 15 | 15 | 0 |
| ✅ | 00:00:01.242 | 13-BUG | Crash Handler | 37 | 37 | 0 |
| ✅ | 00:00:00.754 | 14-DEP | Library Dependencies | 13 | 13 | 0 |
| ✅ | 00:00:00.122 | 15-JSN | JSON Error Handling | 4 | 4 | 0 |
| ✅ | 00:00:00.286 | 20-SHD | Shutdown | 5 | 5 | 0 |
| ✅ | 00:00:00.662 | 21-UPD | Startup/Shutdown | 9 | 9 | 0 |
| ✅ | 00:00:00.526 | 22-SIG | Signal Handling | 9 | 9 | 0 |
| ✅ | 00:00:00.928 | 23-SCK | Socket Rebinding | 8 | 8 | 0 |
| ✅ | 00:00:00.696 | 24-PRE | API Prefix | 10 | 10 | 0 |
| ✅ | 00:00:00.973 | 25-SYS | System API | 10 | 10 | 0 |
| ✅ | 00:00:00.687 | 26-SWG | Swagger | 14 | 14 | 0 |
| ✅ | 00:00:01.087 | 27-WSS | WebSockets | 15 | 15 | 0 |
| ✅ | 00:00:00.939 | 90-MKD | Markdown Lint (markdownlint: 236 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.736 | 91-GCC | C Lint (cppcheck: 249 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.573 | 92-BSH | Bash Lint (shellheck: 51 files) | 2 | 2 | 0 |
| ✅ | 00:00:00.341 | 93-JSN | JSON Lint (jsonlint: 38 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.033 | 94-JAV | JavaScript Lint (eslint: 0 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.032 | 95-CSS | CSS Lint (stylelint: 0 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.263 | 96-HTM | HTML Lint (htmlhint: 1 files) | 1 | 1 | 0 |
| ✅ | 00:00:01.338 | 99-COV | Test Suite Coverage (coverage_table) | 4 | 4 | 0 |

## Repository Information

Generated via cloc: 2025-Aug-13 (Wed) 01:03:35 PDT

```cloc
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C                              144           5161           6354          22799
Markdown                       236           7166             57          21882
Bourne Shell                    51           2183           2422          10076
JSON                            36              2              0           7949
C/C++ Header                   105           1100           4277           2692
CMake                            1            139            182            700
HTML                             1             74              0            493
SVG                              1              3              2            492
-------------------------------------------------------------------------------
SUM:                           575          15828          13294          67083
-------------------------------------------------------------------------------

Code/Docs: 1.7    Code/Comments: 2.7
Docs/Code: 0.6    Comments/Code: 0.4
```
