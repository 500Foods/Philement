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

Generated on: 2025-Aug-19 (Tue) 02:43:03 PDT

### Summary

| Metric | Value |
| ------ | ----- |
| Total Tests | 26 |
| Passed | 26 |
| Failed | 0 |
| Total Subtests | 231 |
| Passed Subtests | 231 |
| Failed Subtests | 0 |
| Elapsed Time | 00:00:16.723 |
| Cumulative Time | 00:00:33.725 |

[Test Suite Results](COMPLETE.svg) | [Test Suite Coverage](COVERAGE.svg)

### Test Coverage

| Test Type | Files Cover | Files Instr | Lines Cover | Lines Instr | Coverage |
| --------- | ----------- | ----------- | ----------- | ----------- | -------- |
| Unity Tests | 15 | 122 | 477 | 12,065 | 3.954% |
| Blackbox Tests | 99 | 122 | 6,154 | 12,065 | 51.007% |
| Combined Tests | 99 | 122 | 6,350 | 12,065 | 52.632% |

### Individual Test Results

| Status | Time | Test | Test Name | Tests | Pass | Fail |
| ------ | ---- | -- | ---- | ----- | ---- | ---- |
| ✅ | 00:00:09.996 | 01-CMP | Compilation (source code: 259 files) | 16 | 16 | 0 |
| ✅ | 00:00:00.242 | 02-ENV | Environment Variables (secrets: 3) | 5 | 5 | 0 |
| ✅ | 00:00:04.967 | 04-LNK | Markdown Links Check (github-sitemap) | 3 | 3 | 0 |
| ✅ | 00:00:01.718 | 10-UNT | Unity (402 / 402 unit tests passed) | 38 | 38 | 0 |
| ✅ | 00:00:00.411 | 11-SIV | Memory Leak Detection | 4 | 4 | 0 |
| ✅ | 00:00:01.340 | 12-VAR | Env Var Substitution | 15 | 15 | 0 |
| ✅ | 00:00:01.477 | 13-BUG | Crash Handler | 37 | 37 | 0 |
| ✅ | 00:00:00.753 | 14-DEP | Library Dependencies | 13 | 13 | 0 |
| ✅ | 00:00:00.130 | 15-JSN | JSON Error Handling | 4 | 4 | 0 |
| ✅ | 00:00:00.454 | 16-SHD | Shutdown | 5 | 5 | 0 |
| ✅ | 00:00:00.821 | 17-UPD | Startup/Shutdown | 9 | 9 | 0 |
| ✅ | 00:00:00.608 | 18-SIG | Signal Handling | 9 | 9 | 0 |
| ✅ | 00:00:01.299 | 19-SCK | Socket Rebinding | 8 | 8 | 0 |
| ✅ | 00:00:00.524 | 20-PRE | API Prefix | 10 | 10 | 0 |
| ✅ | 00:00:00.786 | 21-SYS | System API | 10 | 10 | 0 |
| ✅ | 00:00:00.462 | 22-SWG | Swagger | 14 | 14 | 0 |
| ✅ | 00:00:00.944 | 23-WSS | WebSockets | 15 | 15 | 0 |
| ✅ | 00:00:01.237 | 90-MKD | Markdown Lint (markdownlint: 235 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.673 | 91-GCC | C Lint (cppcheck: 260 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.874 | 92-BSH | Bash Lint (shellheck: 50 files) | 2 | 2 | 0 |
| ✅ | 00:00:00.267 | 93-JSN | JSON Lint (jsonlint: 21 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.043 | 94-JAV | JavaScript Lint (eslint: 0 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.041 | 95-CSS | CSS Lint (stylelint: 0 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.314 | 96-HTM | HTML Lint (htmlhint: 1 files) | 1 | 1 | 0 |
| ✅ | 00:00:01.854 | 98-SIZ | Code Size Analysis (lines: 77,755) | 4 | 4 | 0 |
| ✅ | 00:00:01.490 | 99-COV | Test Suite Coverage (coverage_table) | 4 | 4 | 0 |

## Repository Information

Generated via cloc: 2025-Aug-19 (Tue) 02:43:03 PDT

```cloc
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C                              168           6470           7434          27005
Markdown                       235           7033             55          21288
Bourne Shell                    50           2141           2473           9643
CMake                          120            337            378           8239
JSON                            21              0              0           2723
C/C++ Header                    92           1046           4029           2707
SVG                              2              0              0           2464
Text                            52            105              0           1794
make                             1            381            480            907
HTML                             1             74              0            493
YAML                             1             15              2            284
TypeScript                     104              0              0            208
-------------------------------------------------------------------------------
SUM:                           847          17602          14851          77755
-------------------------------------------------------------------------------

Code/Docs: 2.2   Code/Comments: 3.3   Instrumented Code:  12,065   Ratio: 17.66
Docs/Code: 0.4   Comments/Code: 0.3   Instrumented Test:   5,250   Unity: 43.51
```
