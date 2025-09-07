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

Generated on: 2025-Sep-06 (Sat) 23:58:19 PDT

### Summary

| Metric | Value |
| ------ | ----- |
| Total Tests | 30 |
| Passed | 29 |
| Failed | 0 |
| Total Subtests | 431 |
| Passed Subtests | 421 |
| Failed Subtests | 0 |
| Elapsed Time | 00:00:28.318 |
| Cumulative Time | 00:00:48.777 |

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
| ✅ | 00:00:07.292 | 01-CMP | Compilation (source code: 410 files) | 17 | 17 | 0 |
| ✅ | 00:00:00.449 | 02-ENV | Environment Variables (secrets: 5) | 9 | 9 | 0 |
| ✅ | 00:00:03.757 | 04-LNK | Markdown Links Check (github-sitemap) | 3 | 3 | 0 |
| ✅ | 00:00:05.659 | 10-UNT | Unity (1,347 / 1,347 unit tests passed) | 173 | 173 | 0 |
| ✅ | 00:00:00.703 | 11-SIV | Memory Leak Detection | 4 | 4 | 0 |
| ✅ | 00:00:01.533 | 12-VAR | Env Var Substitution | 15 | 15 | 0 |
| ✅ | 00:00:01.490 | 13-BUG | Crash Handler | 37 | 37 | 0 |
| ✅ | 00:00:00.903 | 14-DEP | Library Dependencies | 17 | 17 | 0 |
| ✅ | 00:00:00.418 | 15-JSN | JSON Error Handling | 4 | 4 | 0 |
| ✅ | 00:00:00.591 | 16-SHD | Shutdown | 5 | 5 | 0 |
| ✅ | 00:00:01.254 | 17-UPD | Startup/Shutdown | 9 | 9 | 0 |
| ✅ | 00:00:04.360 | 18-SIG | Signal Handling | 9 | 9 | 0 |
| ✅ | 00:00:01.239 | 19-SCK | Socket Rebinding | 8 | 8 | 0 |
| ✅ | 00:00:00.774 | 20-PRE | Prefix | 11 | 11 | 0 |
| ✅ | 00:00:01.026 | 21-SYS | Endpoints | 11 | 11 | 0 |
| ✅ | 00:00:00.991 | 22-SWG | Swagger | 15 | 15 | 0 |
| ✅ | 00:00:01.305 | 23-WSS | WebSockets | 13 | 13 | 0 |
| ✅ | 00:00:01.137 | 24-UPL | Uploads | 13 | 13 | 0 |
| ✅ | 00:00:04.182 | 25-DNS | mDNS | 7 | 7 | 0 |
| ✅ | 00:00:00.968 | 26-TRM | Terminal | 17 | 17 | 0 |
| ❌ | 00:00:01.080 | 27-DBS | Databases | 18 | 8 | 10 |
| ✅ | 00:00:01.205 | 90-MKD | Markdown Lint (markdownlint: 254 files) | 1 | 1 | 0 |
| ✅ | 00:00:01.216 | 91-GCC | C Lint (cppcheck: 412 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.788 | 92-BSH | Bash Lint (shellheck: 61 files) | 2 | 2 | 0 |
| ✅ | 00:00:00.496 | 93-JSN | JSON Lint (jsonlint: 36 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.136 | 94-JAV | JavaScript Lint (eslint: 0 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.672 | 95-CSS | CSS Lint (stylelint: 1 files) | 1 | 1 | 0 |
| ✅ | 00:00:00.273 | 96-HTM | HTML Lint (htmlhint: 5 files) | 1 | 1 | 0 |
| ✅ | 00:00:02.706 | 98-SIZ | Code Size Analysis | 4 | 4 | 0 |
| ✅ | 00:00:00.174 | 99-COV | Test Suite Coverage (coverage_table) | 4 | 4 | 0 |

## Repository Information

Generated via cloc: 2025-Sep-06 (Sat) 23:58:19 PDT

```cloc
cloc command failed
```
