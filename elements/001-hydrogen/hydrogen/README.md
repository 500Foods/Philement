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

Generated on: 2025-Aug-25 (Mon) 03:08:19 PDT

### Summary

| Metric | Value |
| ------ | ----- |
| Total Tests | 27 |
| Passed | 27 |
| Failed | 0 |
| Total Subtests | 338 |
| Passed Subtests | 338 |
| Failed Subtests | 0 |
| Elapsed Time | 00:00:43.465 |
| Cumulative Time | 00:00:56.704 |

[Test Suite Results](COMPLETE.svg) | [Test Suite Coverage](COVERAGE.svg)

### Test Coverage

| Test Type | Files Cover | Files Instr | Lines Cover | Lines Instr | Coverage |
| --------- | ----------- | ----------- | ----------- | ----------- | -------- |
| Unity Tests | 89 | 124 | 4,989 | 12,378 | 40.305% |
| Blackbox Tests | 103 | 124 | 6,658 | 12,378 | 53.789% |
| Combined Tests | 105 | 124 | 7,805 | 12,378 | 63.055% |

### Individual Test Results

| Status | Time | Test | Test Name | Tests | Pass | Fail |
| ------ | ---- | -- | ---- | ----- | ---- | ---- |
| ✅ | 00:00:28.009 | 01-CMP | Compilation (source code: 349 files) | 16 | 16 | 0 |
| ✅ | 00:00:00.401 | 02-ENV | Environment Variables (secrets: 3) | 5 | 5 | 0 |
| ✅ | 00:00:02.466 | 04-LNK | Markdown Links Check (github-sitemap) | 3 | 3 | 0 |
| ✅ | 00:00:06.580 | 10-UNT | Unity (1,182 / 1,182 unit tests passed) | 134 | 134 | 0 |
| ✅ | 00:00:00.613 | 11-SIV | Memory Leak Detection | 4 | 4 | 0 |
| ✅ | 00:00:01.300 | 12-VAR | Env Var Substitution | 15 | 15 | 0 |
| ✅ | 00:00:01.471 | 13-BUG | Crash Handler | 37 | 37 | 0 |
| ✅ | 00:00:00.866 | 14-DEP | Library Dependencies | 13 | 13 | 0 |
| ✅ | 00:00:00.274 | 15-JSN | JSON Error Handling | 4 | 4 | 0 |
| ✅ | 00:00:00.566 | 16-SHD | Shutdown | 5 | 5 | 0 |
| ✅ | 00:00:00.906 | 17-UPD | Startup/Shutdown | 9 | 9 | 0 |
| ✅ | 00:00:00.684 | 18-SIG | Signal Handling | 9 | 9 | 0 |
| ✅ | 00:00:01.107 | 19-SCK | Socket Rebinding | 8 | 8 | 0 |
| ✅ | 00:00:00.565 | 20-PRE | API Prefix | 11 | 11 | 0 |
| ✅ | 00:00:00.686 | 21-SYS | System API | 10 | 10 | 0 |
| ✅ | 00:00:00.539 | 22-SWG | Swagger | 15 | 15 | 0 |
| ✅ | 00:00:01.028 | 23-WSS | WebSockets | 13 | 13 | 0 |
| ✅ | 00:00:00.842 | 24-UPL | Uploads | 11 | 11 | 0 |
| ✅ | 00:00:01.397 | 90-MKD | Markdown Lint (markdownlint: 249 files) | 1 | 1 | 0 |
| ✅ | 00:00:01.055 | 91-GCC | C Lint (cppcheck: 350 files) | 1 | 1 | 0 |
| ✅ | 00:00:01.005 | 92-BSH | Bash Lint (shellheck: 52 files) | 2 | 2 | 0 |
| ✅ | 00:00:00.277 | 93-JSN | JSON Lint (jsonlint: 25 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.042 | 94-JAV | JavaScript Lint (eslint: 0 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.042 | 95-CSS | CSS Lint (stylelint: 0 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.275 | 96-HTM | HTML Lint (htmlhint: 1 files) | 1 | 1 | 0 |
| ✅ | 00:00:02.038 | 98-SIZ | Code Size Analysis (lines: 78,077) | 4 | 4 | 0 |
| ✅ | 00:00:01.670 | 99-COV | Test Suite Coverage (coverage_table) | 4 | 4 | 0 |

## Repository Information

Generated via cloc: 2025-Aug-25 (Mon) 03:08:19 PDT

```cloc
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C                              265          10038          11681          36980
Markdown                       249           7092             55          21338
Bourne Shell                    52           2300           2706          10115
JSON                            25              0              0           2854
C/C++ Header                    85           1055           3394           2829
SVG                              2              0              0           2747
CMake                            1            139            186            721
HTML                             1             74              0            493
-------------------------------------------------------------------------------
SUM:                           680          20698          18022          78077
-------------------------------------------------------------------------------

Code/Docs: 2.4   Code/Comments: 2.8   Instrumented Code:  12,378   Ratio: 35.09
Docs/Code: 0.4   Comments/Code: 0.4   Instrumented Test:  13,971   Unity: 112.86
```
