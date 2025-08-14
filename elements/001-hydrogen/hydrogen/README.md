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

Generated on: 2025-Aug-13 (Wed) 23:13:57 PDT

### Summary

| Metric | Value |
| ------ | ----- |
| Total Tests | 26 |
| Passed | 26 |
| Failed | 0 |
| Total Subtests | 231 |
| Passed Subtests | 231 |
| Failed Subtests | 0 |
| Elapsed Time | 00:00:13.056 |
| Cumulative Time | 00:00:27.726 |

[Test Suite Results](COMPLETE.svg) | [Test Suite Coverage](COVERAGE.svg)

### Test Coverage

| Test Type | Files Cover | Files Instr | Lines Cover | Lines Instr | Coverage | Timestamp |
| --------- | ----------- | ----------- | ----------- | ----------- | -------- | --------- |
| Unity Tests | 16 | 121 | 479 | 12,016 | 3.986% | 20250813_231357 |
| Blackbox Tests | 98 | 121 | 6,156 | 12,016 | 51.232% | 20250813_231357 |
| Combined Tests | 98 | 121 | 6,352 | 12,016 | 52.863% | 20250813_231409 |

### Individual Test Results

| Status | Time | Test | Test Name | Tests | Pass | Fail |
| ------ | ---- | -- | ---- | ----- | ---- | ---- |
| ✅ | 00:00:07.901 | 01-CMP | Compilation (source code: 271 files) | 16 | 16 | 0 |
| ✅ | 00:00:00.121 | 02-ENV | Environment Variables (secrets: 3) | 5 | 5 | 0 |
| ✅ | 00:00:02.127 | 03-SIZ | Code Size Analysis (lines: 67,375) | 4 | 4 | 0 |
| ✅ | 00:00:02.197 | 04-LNK | Markdown Links Check (github-sitemap) | 3 | 3 | 0 |
| ✅ | 00:00:01.423 | 10-UNT | Unity (403 / 403 unit tests passed) | 38 | 38 | 0 |
| ✅ | 00:00:00.387 | 11-SIV | Memory Leak Detection | 4 | 4 | 0 |
| ✅ | 00:00:01.210 | 12-VAR | Env Var Substitution | 15 | 15 | 0 |
| ✅ | 00:00:01.312 | 13-BUG | Crash Handler | 37 | 37 | 0 |
| ✅ | 00:00:00.626 | 14-DEP | Library Dependencies | 13 | 13 | 0 |
| ✅ | 00:00:00.115 | 15-JSN | JSON Error Handling | 4 | 4 | 0 |
| ✅ | 00:00:00.292 | 20-SHD | Shutdown | 5 | 5 | 0 |
| ✅ | 00:00:00.707 | 21-UPD | Startup/Shutdown | 9 | 9 | 0 |
| ✅ | 00:00:00.565 | 22-SIG | Signal Handling | 9 | 9 | 0 |
| ✅ | 00:00:00.971 | 23-SCK | Socket Rebinding | 8 | 8 | 0 |
| ✅ | 00:00:00.740 | 24-PRE | API Prefix | 10 | 10 | 0 |
| ✅ | 00:00:01.000 | 25-SYS | System API | 10 | 10 | 0 |
| ✅ | 00:00:00.728 | 26-SWG | Swagger | 14 | 14 | 0 |
| ✅ | 00:00:01.174 | 27-WSS | WebSockets | 15 | 15 | 0 |
| ✅ | 00:00:00.953 | 90-MKD | Markdown Lint (markdownlint: 237 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.586 | 91-GCC | C Lint (cppcheck: 271 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.675 | 92-BSH | Bash Lint (shellheck: 51 files) | 2 | 2 | 0 |
| ✅ | 00:00:00.217 | 93-JSN | JSON Lint (jsonlint: 21 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.036 | 94-JAV | JavaScript Lint (eslint: 0 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.031 | 95-CSS | CSS Lint (stylelint: 0 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.264 | 96-HTM | HTML Lint (htmlhint: 1 files) | 1 | 1 | 0 |
| ✅ | 00:00:01.368 | 99-COV | Test Suite Coverage (coverage_table) | 4 | 4 | 0 |

## Repository Information

Generated via cloc: 2025-Aug-13 (Wed) 23:13:57 PDT

```cloc
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C                              166           6289           7232          26135
Markdown                       237           7254             57          22060
Bourne Shell                    51           2193           2461          10110
JSON                            21              0              0           2722
C/C++ Header                   105           1100           4277           2692
SVG                              2              6              4           2463
CMake                            1            139            182            700
HTML                             1             74              0            493
-------------------------------------------------------------------------------
SUM:                           584          17055          14213          67375
-------------------------------------------------------------------------------

Code/Docs: 1.8    Code/Comments: 2.8
Docs/Code: 0.6    Comments/Code: 0.4
```
