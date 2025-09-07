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

Generated on: 2025-Sep-07 (Sun) 09:34:03 PDT

### Summary

| Metric | Value |
| ------ | ----- |
| Total Tests | 30 |
| Passed | 29 |
| Failed | 0 |
| Total Subtests | 431 |
| Passed Subtests | 421 |
| Failed Subtests | 0 |
| Elapsed Time | 00:00:28.146 |
| Cumulative Time | 00:00:56.750 |

[Test Suite Results](COMPLETE.svg) | [Test Suite Coverage](COVERAGE.svg)

### Test Coverage

| Test Type | Files Cover | Files Instr | Lines Cover | Lines Instr | Coverage |
| --------- | ----------- | ----------- | ----------- | ----------- | -------- |
| Unity Tests | 99 | 139 | 5,009 | 16,103 | 31.106% |
| Blackbox Tests | 122 | 139 | 8,964 | 16,103 | 55.667% |
| Combined Tests | 123 | 139 | 10,001 | 16,103 | 62.106% |

### Individual Test Results

| Status | Time | Test | Test Name | Tests | Pass | Fail |
| ------ | ---- | -- | ---- | ----- | ---- | ---- |
| ✅ | 00:00:06.317 | 01-CMP | Compilation (source code: 410 files) | 17 | 17 | 0 |
| ✅ | 00:00:00.464 | 02-ENV | Environment Variables (secrets: 5) | 9 | 9 | 0 |
| ✅ | 00:00:05.272 | 04-LNK | Markdown Links Check (github-sitemap) | 3 | 3 | 0 |
| ✅ | 00:00:05.574 | 10-UNT | Unity (1,338 / 1,338 unit tests passed) | 173 | 173 | 0 |
| ✅ | 00:00:00.665 | 11-SIV | Memory Leak Detection | 4 | 4 | 0 |
| ✅ | 00:00:01.549 | 12-VAR | Env Var Substitution | 15 | 15 | 0 |
| ✅ | 00:00:01.569 | 13-BUG | Crash Handler | 37 | 37 | 0 |
| ✅ | 00:00:00.923 | 14-DEP | Library Dependencies | 17 | 17 | 0 |
| ✅ | 00:00:00.424 | 15-JSN | JSON Error Handling | 4 | 4 | 0 |
| ✅ | 00:00:00.590 | 16-SHD | Shutdown | 5 | 5 | 0 |
| ✅ | 00:00:01.257 | 17-UPD | Startup/Shutdown | 9 | 9 | 0 |
| ✅ | 00:00:03.807 | 18-SIG | Signal Handling | 9 | 9 | 0 |
| ✅ | 00:00:01.316 | 19-SCK | Socket Rebinding | 8 | 8 | 0 |
| ✅ | 00:00:00.842 | 20-PRE | Prefix | 11 | 11 | 0 |
| ✅ | 00:00:01.011 | 21-SYS | Endpoints | 11 | 11 | 0 |
| ✅ | 00:00:01.083 | 22-SWG | Swagger | 15 | 15 | 0 |
| ✅ | 00:00:01.327 | 23-WSS | WebSockets | 13 | 13 | 0 |
| ✅ | 00:00:01.250 | 24-UPL | Uploads | 13 | 13 | 0 |
| ✅ | 00:00:02.784 | 25-DNS | mDNS | 7 | 7 | 0 |
| ✅ | 00:00:01.017 | 26-TRM | Terminal | 17 | 17 | 0 |
| ❌ | 00:00:01.121 | 27-DBS | Databases | 18 | 8 | 10 |
| ✅ | 00:00:00.940 | 90-MKD | Markdown Lint (markdownlint: 254 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.959 | 91-GCC | C Lint (cppcheck: 412 files) | 1 | 1 | 0 |
| ✅ | 00:00:09.833 | 92-BSH | Bash Lint (shellheck: 61 files) | 2 | 2 | 0 |
| ✅ | 00:00:00.740 | 93-JSN | JSON Lint (jsonlint: 36 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.065 | 94-JAV | JavaScript Lint (eslint: 0 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.658 | 95-CSS | CSS Lint (stylelint: 1 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.282 | 96-HTM | HTML Lint (htmlhint: 5 files) | 1 | 1 | 0 |
| ✅ | 00:00:02.901 | 98-SIZ | Code Size Analysis | 4 | 4 | 0 |
| ✅ | 00:00:00.210 | 99-COV | Test Suite Coverage (coverage_table) | 4 | 4 | 0 |

## Repository Information

Generated via cloc: 2025-Sep-07 (Sun) 09:34:03 PDT

```cloc
[0;31m╭─────────────────────────────────╮[0m
[0;31m│ [1;37m[1m[1;37mgithub.com/AlDanial/cloc v 2.06[0m[0m [0;31m│
[0;31m├────────────────┬───────┬────────┼─────────┬────────┬─────────╮[0m
[0;31m│[0;32m Language       [0;31m│[0;32m Files [0;31m│[0;32m  Blank [0;31m│[0;32m Comment [0;31m│[0;32m   Code [0;31m│[0;32m   Lines [0;31m│[0m
[0;31m├────────────────┼───────┼────────┼─────────┼────────┼─────────┤[0m
[0;31m│[0m Core C/Headers [0;31m│[0m   240 [0;31m│[0m  7,066 [0;31m│[0m  11,447 [0;31m│[0m 28,433 [0;31m│[0m  46,946 [0;31m│[0m
[0;31m│[0m Test C/Headers [0;31m│[0m   171 [0;31m│[0m  6,518 [0;31m│[0m   6,655 [0;31m│[0m 19,735 [0;31m│[0m  32,908 [0;31m│[0m
[0;31m│[0m Markdown       [0;31m│[0m   253 [0;31m│[0m  7,971 [0;31m│[0m      56 [0;31m│[0m 23,856 [0;31m│[0m  31,883 [0;31m│[0m
[0;31m│[0m Bourne Shell   [0;31m│[0m    61 [0;31m│[0m  2,846 [0;31m│[0m   3,208 [0;31m│[0m 12,905 [0;31m│[0m  18,959 [0;31m│[0m
[0;31m│[0m JSON           [0;31m│[0m    35 [0;31m│[0m        [0;31m│[0m         [0;31m│[0m  3,439 [0;31m│[0m   3,439 [0;31m│[0m
[0;31m│[0m SVG            [0;31m│[0m     2 [0;31m│[0m        [0;31m│[0m         [0;31m│[0m  3,051 [0;31m│[0m   3,051 [0;31m│[0m
[0;31m│[0m HTML           [0;31m│[0m     4 [0;31m│[0m     98 [0;31m│[0m         [0;31m│[0m    725 [0;31m│[0m     823 [0;31m│[0m
[0;31m│[0m CMake          [0;31m│[0m     1 [0;31m│[0m    139 [0;31m│[0m     186 [0;31m│[0m    708 [0;31m│[0m   1,033 [0;31m│[0m
[0;31m│[0m CSS            [0;31m│[0m     1 [0;31m│[0m     11 [0;31m│[0m       5 [0;31m│[0m     42 [0;31m│[0m      58 [0;31m│[0m
[0;31m│[0m SQL            [0;31m│[0m     4 [0;31m│[0m      4 [0;31m│[0m         [0;31m│[0m     33 [0;31m│[0m      37 [0;31m│[0m
[0;31m├────────────────┼───────┼────────┼─────────┼────────┼─────────┤[0m
[0;31m│[1;37m                [0;31m│[1;37m   772 [0;31m│[1;37m 24,653 [0;31m│[1;37m  21,557 [0;31m│[1;37m 92,927 [0;31m│[1;37m 139,137 [0;31m│[0m
[0;31m╰────────────────┴───────┴────────┴─────────┴────────┴─────────╯[0m
[0;31m╭─────────────────────╮[0m
[0;31m│ [1;37m[1m[1;37mExtended Statistics[0m[0m [0;31m│
[0;31m├────────────────────┬┴────────┬────────────────────────────────────────────╮[0m
[0;31m│[0;32m Metric             [0;31m│[0;32m   Value [0;31m│[0;32m Description                                [0;31m│[0m
[0;31m├────────────────────┼─────────┼────────────────────────────────────────────┤[0m
[0;31m│[0m Code/Docs          [0;31m│[0m     1.8 [0;31m│[0m Ratio of code lines to documentation lines [0;31m│[0m
[0;31m│[0m Docs/Code          [0;31m│[0m     0.6 [0;31m│[0m Ratio of documentation lines to code lines [0;31m│[0m
[0;31m│[0m Code/Comments      [0;31m│[0m     2.8 [0;31m│[0m Ratio of code lines to comment lines       [0;31m│[0m
[0;31m│[0m Comments/Code      [0;31m│[0m     0.3 [0;31m│[0m Ratio of comment lines to code lines       [0;31m│[0m
[0;31m│[0m Instrumented Black [0;31m│[0m  16,103 [0;31m│[0m Lines of code with coverage - Blackbox     [0;31m│[0m
[0;31m│[0m Instrumented Unity [0;31m│[0m  15,950 [0;31m│[0m Lines of code with coverage - Unity        [0;31m│[0m
[0;31m│[0m Unity Ratio        [0;31m│[0m   99.0% [0;31m│[0m Unity instrumented / Blackbox instrumented [0;31m│[0m
[0;31m╰────────────────────┴─────────┴────────────────────────────────────────────╯[0m
```
