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

- [**Curiosities**](/docs/H/CURIOSITIES.md) - Conventions and Curiosities worth noting before proceeding
- [**Databases**](/docs/H/DATABASES.md) - Considerations around using PostgreSQL, MySQL/MariaDB, SQLite, and DB2
- [**AI Instructions**](/docs/H/INSTRUCTIONS.md) - Development guide optimized for AI assistance
- [**AI Prompts**](/docs/H/PROMPTS.md) - Development guide optimized for AI assistance
- [**Project Structure**](/docs/H/STRUCTURE.md) - Complete file organization and architecture overview
- [**Build Environment**](/docs/H/SETUP.md) - Build and runtime requirements, environment setup
- [**Secrets Management**](/docs/H/SECRETS.md) - Environment variables and security configuration
- [**Release Notes**](/RELEASES.md) - Detailed version history and changes
- [**Markdown Site Map**](/docs/H/SITEMAP.md) - Index to all Markdown files in this repository

### Generated Content

- [**Release Notes**](/releases/README.md) - Release notes
- [**Build Metrics**](/docs/H/metrics/README.md) - Daily build metrics and quality tracking
- [**Planning**](/docs/H/plans/README.md) - Planning documents, TO-DO lists, that kind of thing

### Core Documentation

- [**Documentation Hub**](/docs/H/core/README.md) - Start here for comprehensive overview
- [**Subsystems Documentation**](/docs/H/core/subsystems/README.md) - Overview of all Hydrogen subsystems
- [**Developer Onboarding**](/docs/H/core/developer_onboarding.md) - Visual architecture overview and code navigation
- [**Quick Start Guide**](/docs/H/core/guides/quick-start.md) - Get up and running quickly
- [**AI Integration**](/docs/H/core/ai_integration.md) - AI capabilities and implementations

### Modules (aka plugins or extensions)

- [**GAIUS**](/docs/H/modules/GAIUS.md) - Support for the [GAIUS](https://www.gaiusmodel.cmo) project.

### Technical References

- [**API Documentation**](/docs/H/core/subsystems/api/api.md) - REST API reference and implementation details
- [**Configuration Guide**](/docs/H/core/configuration.md) - System configuration and settings
- [**Data Structures**](/docs/H/core/data_structures.md) - Core data structures and interfaces
- [**Testing Framework**](/docs/H/core/testing.md) - Testing documentation and procedures

### Architecture & Design

- [**Service Architecture**](/docs/H/core/service.md) - Service management and lifecycle
- [**Shutdown Architecture**](/docs/H/core/shutdown_architecture.md) - Graceful shutdown procedures
- [**Thread Monitoring**](/docs/H/core/subsystems/threads/threads.md) - Thread management and monitoring
- [**WebSocket Implementation**](/docs/H/core/subsystems/websocket/websocket.md) - WebSocket server architecture
- [**Mirage Proxy**](/docs/H/core/subsystems/mirage/mirage.md) - WebSocket tunneling for remote device access
- [**mDNS Server**](/docs/H/core/subsystems/mdnsserver/mdnsserver.md) - Service discovery implementation
- [**Print Queue System**](/docs/H/core/subsystems/print/print.md) - 3D printing queue management
- [**OIDC Integration**](/docs/H/core/subsystems/oidc/oidc.md) - OpenID Connect authentication
- [**System Information**](/docs/H/core/system_info.md) - System monitoring and reporting

### Implementation Notes

- [**Database**](/docs/H/plans/DATABASE_PLAN.md) - Implementation plan for Database subsystem
- [**Migrations**](/docs/H/plans/MIGRATIONS.md) - Performance improvements to the Migration system
- [**Conduit**](/docs/H/plans/CONDUIT.md) - Implementation plan for Conduit Service endpoints
- [**Auth**](/docs/H/plans/AUTH_PLAN.md) - Implementation plan for Auth subsystem

### Examples & Implementation

- [**Examples Overview**](/elements/001-hydrogen/hydrogen/examples/README.md) - Code examples and implementations
- [**Build Scripts & Utilities**](/elements/001-hydrogen/hydrogen/extras/README.md) - Build scripts and diagnostic tools
- [**API Implementation**](/elements/001-hydrogen/hydrogen/src/api/README.md) - API implementation details
- [**Payload System**](/elements/001-hydrogen/hydrogen/payloads/README.md) - Payload system with encryption
- [**Testing Suite**](/elements/001-hydrogen/hydrogen/tests/README.md) - Test framework and procedures

## Attributions

### Core Libraries

Hydrogen uses several open-source C libraries for core functionality:

- [**jansson**](https://github.com/akheron/jansson) - JSON parsing and generation library
- [**libmicrohttpd**](https://www.gnu.org/software/libmicrohttpd/) - HTTP server library implementing the HTTP/1.1 protocol
- [**libwebsockets**](https://libwebsockets.org/) - WebSocket server and client library
- [**libbrotli**](https://github.com/google/brotli) - Brotli compression algorithm library
- [**libuuid**](https://sourceforge.net/projects/libuuid/) - UUID generation library
- [**libtar**](https://github.com/tklauser/libtar) - TAR file manipulation library
- [**Lua**](https://www.lua.org/) - Lightweight scripting language
- [**OpenSSL**](https://www.openssl.org/) - Cryptography and SSL/TLS toolkit
- **POSIX Threads** - Standard threading library (pthreads)
- **GNU C Library (glibc)** - Core C library providing essential functions

### Web Interface Libraries

Hydrogen embeds several JavaScript libraries for web-based interfaces and API documentation:

- [**xterm.js**](https://xtermjs.org/) - Terminal emulation library for web-based terminal interfaces
- [**SwaggerUI**](https://swagger.io/tools/swagger-ui/) - Interactive API documentation interface for OpenAPI specifications

### Build Tools

Hydrogen's build system and development tools include:

- [**CMake**](https://cmake.org/) - Cross-platform build system generator
- [**Ninja**](https://ninja-build.org/) - Fast build system with low overhead
- [**GCC**](https://gcc.gnu.org/) - GNU Compiler Collection for C compilation
- [**Unity**](https://www.throwtheswitch.org/unity) - Unit test framework for C
- [**UPX**](https://upx.github.io/) - Ultimate Packer for eXecutables (binary compression)
- [**ccache**](https://ccache.dev/) - Compiler cache for faster rebuilds
- [**Make**](https://www.gnu.org/software/make/) - Traditional build automation tool

### Development & Testing Tools

Hydrogen uses comprehensive development and testing tools for code quality and validation:

- [**sqruff**](https://github.com/quarylabs/sqruff) - SQL linting and formatting tool [Test 31](/docs/H/tests/test_31_migrations.md))
- [**markdownlint**](https://github.com/DavidAnson/markdownlint) - Markdown file linting and style checking ([Test 90](/docs/H/tests/test_90_markdownlint.md))
- [**cppcheck**](https://cppcheck.sourceforge.io/) - Static analysis for C/C++ code ([Test 91](/docs/H/tests/test_91_cppcheck.md))
- [**shellcheck**](https://www.shellcheck.net/) - Static linting for bash scripts ([Test 92](/docs/H/tests/test_92_shellcheck.md))
- [**jsonlint**](https://github.com/zaach/jsonlint) - JSON file validation and formatting ([Test 93](/docs/H/tests/test_93_jsonlint.md))
- [**eslint**](https://eslint.org/) - JavaScript code linting and style checking ([Test 94](/docs/H/tests/test_94_eslint.md))
- [**stylelint**](https://stylelint.io/) - CSS/SCSS linting and style checking ([Test 95](/docs/H/tests/test_95_stylelint.md))
- [**htmlhint**](https://htmlhint.com/) - HTML code linting and validation ([Test 96](/docs/H/tests/test_96_htmlhint.md))
- [**xmlstarlet**](https://xmlstar.sourceforge.net/) - XML/SVG validation and processing ([Test 97](/docs/H/tests/test_97_xmlstarlet.md))
- [**luacheck**](https://github.com/mpeterv/luacheck) - Lua code analysis and linting ([Test 98](/docs/H/tests/test_98_luacheck.md))
- [**cloc**](https://github.com/AlDanial/cloc) - Count Lines of Code analysis tool ([Test 99](/docs/H/tests/test_99_code_size.md))
- [**swagger-cli**](https://github.com/APIDevTools/swagger-cli) - OpenAPI/Swagger validation and processing [Payload](/elements/001-hydrogen/hydrogen/payloads/README.md))

### Database Diagrams

The [database diagram generation tool](/docs/H/tests/get_diagram.md) uses embedded icons and fonts:

- [**Font Awesome Free**](https://fontawesome.com) - Embedded SVG icons for database schema elements (keys, relationships, constraints)
- [**Cairo Font Family**](https://fonts.google.com/specimen/Cairo) - Font family used for rendering text in generated diagrams
- [**D3.js**](https://d3js.org/) (v7.8.5) - Data visualization library for creating interactive charts and graphs
- [**JSDOM**](https://github.com/jsdom/jsdom) - JavaScript implementation of the DOM for Node.js CLI functionality

### Hydrogen Build Metrics Browser

The [Hydrogen Build Metrics Browser](/elements/001-hydrogen/hydrogen/extras/hbm_browser/) uses several open-source JavaScript libraries:

- [**Font Awesome Free**](https://fontawesome.com) - Embedded SVG icons for database schema elements (keys, relationships, constraints)
- [**Cairo Font Family**](https://fonts.google.com/specimen/Cairo) - Font family used for rendering
- [**D3.js**](https://d3js.org/) (v7.8.5) - Data visualization library for creating interactive charts and graphs
- [**Flatpickr**](https://flatpickr.js.org/) (v4.6.13) - Lightweight date picker library
- [**Iro.js**](https://iro.js.org/) (v5) - Color picker widget for the web
- [**JSDOM**](https://github.com/jsdom/jsdom) - JavaScript implementation of the DOM for Node.js CLI functionality

## Latest Test Results

<div style="display: flex; flex-direction: column; align-items: start; gap: 0px;">
  <img src="/elements/001-hydrogen/hydrogen/images/COMPLETE.svg" alt="Complete Test Results" style="margin: -25px 0px 0px -20px;">
  <img src="/elements/001-hydrogen/hydrogen/images/COVERAGE.svg" alt="Coverage Test Results" style="margin: -25px 0px 0px -20px;">
</div>

## Repository Information

<div style="display: flex; background: none !important; border: none !important; flex-direction: column; align-items: start; gap: 0px;">
  <img src="/elements/001-hydrogen/hydrogen/images/CLOC_CODE.svg" alt="CLOC Code Analysis" style="margin: -25px 0px 0px -20px;">
  <img src="/elements/001-hydrogen/hydrogen/images/CLOC_STAT.svg" alt="CLOC Extended Statistics" style="margin: -25px 0px 0px -20px;">
</div>

## Repository History

The [Hydrogen Build Metrics Browser](/elements/001-hydrogen/hydrogen/extras/hbm_browser/README.md) is available through the Swagger/OpenAPI page in a running system, locally (eg: `firefox hbm_browser.html`), or can be used with its command-line variant to produce SVG files directly. The tool provides access to nearly 3,000 build metrics - basically everything in the above tables that can be converted to a number. The chart shown here is generated automatically at the same time as the table data that it uses. Archiving of this data began around September 2025. Additional metrics became available automatically as they were added to the table output.

<div style="display: flex; background: none !important; border: none !important; flex-direction: column; align-items: start; gap: 0px;">
  <img src="/elements/001-hydrogen/hydrogen/images/HISTORY.svg" alt="Hydrogen Build Metrics" style="margin: 20px 0px 0px 0px;">
</div>
