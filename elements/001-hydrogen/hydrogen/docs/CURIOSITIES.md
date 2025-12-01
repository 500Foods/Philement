# Hydrogen Project: Behind the Scenes

This document explores the unique aspects, design decisions, and conventions of the Hydrogen project. It's designed to help developers understand the project's approach, whether you're deploying it, contributing code, or just curious about its inner workings.

## Table of Contents

- [Project Structure](#project-structure)
- [Testing Philosophy](#testing-philosophy)
- [Build System](#build-system)
- [Code Quality & Linting](#code-quality--linting)
- [Key Reports & Analytics](#key-reports--analytics)
- [Subsystem Architecture](#subsystem-architecture)
- [Design Principles](#design-principles)
- [Database Migrations](#database-migrations)
- [Lua Scripting](#lua-scripting)

## Project Structure

The Hydrogen project lives in `elements/001-hydrogen/hydrogen/`. While there are related projects like [Deuterium](https://github.com/500Foods/Philement/blob/main/elements/001-hydrogen/deuterium/README.md) and [Tritium](https://github.com/500Foods/Philement/blob/main/elements/001-hydrogen/tritium/README.md) in the same directory, they are separate initiatives with their own documentation.

### Directory Layout

The main project contains:

**Core Components:**

- **Top-level documents** - Project documentation and guides
- **Build executables** - `hydrogen`, `hydrogen_release`, `hydrogen_coverage`

**Project Folders:**

```directory
hydrogen/
├── build/          # Temporary build artifacts (object files, coverage data)
├── cmake/          # Build system (CMakeLists.txt + 12+ included files)
├── configs/        # JSON configuration examples
├── docs/           # Main documentation (plus tests/docs/)
├── examples/       # Client code samples
├── extras/         # Helper scripts and utilities
├── images/         # Generated SVG reports and diagrams
├── installer/      # Bash installers and Docker images
├── payloads/       # Payload generation system
├── src/            # Main C source code
├── tests/          # Blackbox test scripts
└── tests/unity/    # Unity unit testing framework
```

**Configuration Files:**

- `.gitignore` - Git exclusion rules
- `.lintignore*` - Linting exception patterns
- `.sqruff_*` - SQL linting configurations per database engine
- `.trial-ignore` - Files excluded from final executables

The directory structure is intentionally clean - temporary files from builds or crashes may appear but aren't part of the permanent codebase.

## Testing Philosophy

Testing is central to Hydrogen's development approach, with two complementary frameworks working together.

### Test Types

**Blackbox Tests (Integration):**

- ~40 Bash scripts testing the running server via external interfaces
- Focus on "happy path" scenarios using curl, log analysis, etc.
- Emphasis on speed - full suite completes in under a minute
- Located in `tests/`, orchestrated by `test_00_all.sh`

**Unit Tests (Unity Framework):**

- 4,000+ individual tests across 500+ source files
- More test code than production code
- Focus on edge cases, parameter validation, and code coverage
- Extremely fast execution with comprehensive mocking

### Coverage Strategy

Coverage analysis combines both test types using specialized builds:

- `hydrogen_coverage` - Instrumented build for blackbox testing
- Unity build - Coverage from unit tests
- Combined reporting shows per-file coverage metrics

### Coverage Goals

- **Universal Coverage**: Every source file except `hydrogen.c` (excluded due to unit test architecture)
- **Tiered Standards**:
  - Files <100 instrumented lines: 50% minimum coverage
  - Files ≥100 instrumented lines: 75% minimum coverage
- **Size Monitoring**: Test 99 flags files exceeding 1,000 lines

### Coverage Challenges & Solutions

System-level code often resists testing. Our approaches:

1. **Function Decomposition**: Break large functions into testable helper functions
2. **File Splitting**: Divide oversized files to meet coverage thresholds
3. **Mock Libraries**: Simulate system calls (malloc, network, etc.) for isolated testing

See [Unity Mocks](../tests/unity/mocks/README.md) for detailed mocking strategies.

## Build System

Hydrogen uses CMake with GCC, primarily developed on Fedora 41 with Ubuntu 24 support. Cross-platform compatibility is achieved through Docker rather than native ports.

### Build Integration

Test scripts double as build tools:

- **[Test 01: Compilation](../tests/docs/test_01_compilation.md)** - Full project build including all Unity tests
- **[Test 10: Unity](../tests/docs/test_10_unity.md)** - Executes unit test suite
- **[Test 99: Coverage](../tests/docs/test_99_coverage.md)** - Generates coverage reports

## Code Quality & Linting

Code quality is enforced through comprehensive linting, established from project inception to ensure consistency.

### Primary Linters

**C/C++ (cppcheck):**

- All directives enabled from day one
- Strict enforcement of coding standards
- Inline exceptions for justified deviations

**Bash (shellcheck):**

- Comprehensive directive compliance
- **[Test 92: Shellcheck](../tests/docs/test_92_shellcheck.md)** requires justification for exceptions
- Prevents accumulation of technical debt

### Additional Quality Tools

- **[Test 31: Migrations](../tests/docs/test_31_migrations.md)**: SQL validation with sqruff across database engines
- **[Test 38: Luacheck](../tests/docs/test_38_luacheck.md)**: Lua code analysis with luacheck
- **[Test 90: Markdownlint](../tests/docs/test_90_markdownlint.md)**: Lints Markdown documentation using markdownlint
- **[Test 91: Cppcheck](../tests/docs/test_91_cppcheck.md)**: Performs C/C++ static analysis using cppcheck
- **[Test 92: Shellcheck](../tests/docs/test_92_shellcheck.md)**: Validates shell scripts using shellcheck
- **[Test 93: JSONLint](../tests/docs/test_93_jsonlint.md)**: Validates JSON file syntax and structure
- **[Test 94: ESLint](../tests/docs/test_94_eslint.md)**: Lints JavaScript files using eslint
- **[Test 95: Stylelint](../tests/docs/test_95_stylelint.md)**: Validates CSS files using stylelint
- **[Test 96: HTMLHint](../tests/docs/test_96_htmlhint.md)**: Validates HTML files using htmlhint
- **[Test 97: XMLStarlet](../tests/docs/test_97_xmlstarlet.md)**: Validates XML/SVG files using xmlstarlet
- **[Test 98: Code Size](../tests/docs/test_98_code_size.md)**: Analyzes code size metrics and file distribution
- **[Test 99: Coverage](../tests/docs/test_99_coverage.md)**: Performs comprehensive coverage analysis

### SQL Linting Philosophy

While SQL linting is unconventional, we use sqruff primarily for syntax validation to catch migration-breaking errors. Style enforcement is secondary to functionality, with engine-specific exceptions for compatibility.

**Configuration Files:**

- [DB2](../.sqruff_db2) - DB2-specific sqruff rules
- [MySQL/MariaDB](../.sqruff_mysql) - MySQL/MariaDB-specific sqruff rules
- [PostgreSQL](../.sqruff_postgresql) - PostgreSQL-specific sqruff rules
- [SQLite](../.sqruff_sqlite) - SQLite-specific sqruff rules

## Key Reports & Analytics

The build system generates comprehensive analytics automatically.

### Primary Reports

**[Test Suite Results](../images/COMPLETE.svg)** ([view SVG](../images/COMPLETE.svg)):

- Complete test status overview in tabular format
- Coverage statistics (overall + blackbox vs. unit breakdown)
- Parallel execution with extensive caching
- Generated only during [Test 01: Compilation](../tests/docs/test_01_compilation.md)

**[Test Suite Coverage](../images/COVERAGE.svg)** ([view SVG](../images/COVERAGE.svg)):

- Per-file coverage analysis (200+ files, 4,000+ tests, 20,000+ lines)
- Metrics: lines of code, test counts, coverage percentages
- Generated via [Coverage Table](../tests/docs/coverage_table.md) script

**[Code Metrics (CLOC)](../images/CLOC_CODE.svg)** ([view SVG](../images/CLOC_CODE.svg)):

- Lines of code by filetype with comment/blank line separation
- Project size and complexity assessment
- Useful for comparing against other open-source projects

**[Enhanced Metrics](../images/CLOC_STAT.svg)** ([view SVG](../images/CLOC_STAT.svg)):

- CLOC data combined with coverage analysis
- Comment-to-code ratios, test coverage breakdowns

### Report Generation

All reports use custom tools:

- `tables` - Data formatting
- `Oh.sh` - ANSI-to-SVG conversion
- Both developed by the Hydrogen project maintainer

## Subsystem Architecture

Hydrogen's scale requires modular design. Components are organized as independent subsystems with dedicated threads, queues, and lifecycle management.

### Launch & Landing System

Each subsystem follows a controlled startup/shutdown sequence:

- **Launch Phase**: Readiness checks → Configuration validation → Subsystem initialization
- **Landing Phase**: Graceful shutdown → Resource cleanup → State validation

This allows partial system operation - for example, running without Swagger if web server requirements aren't met.

### Current Subsystems

1. **Registry** - Component registration and discovery
2. **Payload** - Embedded resource management
3. **Threads** - Thread pool management
4. **Network** - Network interface handling
5. **Database** - Multi-engine database connectivity
6. **Logging** - Structured logging system
7. **WebServer** - HTTP server implementation
8. **API** - REST API endpoints
9. **Swagger** - API documentation
10. **WebSocket** - Real-time communication
11. **Terminal** - Terminal emulation
12. **mDNS Server** - Service discovery server
13. **mDNS Client** - Service discovery client
14. **Mail Relay** - Email functionality
15. **Print** - Print subsystem
16. **Resources** - Resource management
17. **OIDC** - OpenID Connect authentication
18. **Notify** - Notification system

## Design Principles

Hydrogen makes some unconventional design choices that prioritize flexibility and maintainability.

### SQL Architecture

**Zero Embedded SQL:**
Despite being a database-backed REST API server, Hydrogen contains virtually no SQL in its source code. Database schemas, queries, and logic are entirely externalized.

**Benefits:**

- **Customization**: Clients can modify database structures without code changes
- **Portability**: Same server infrastructure supports different data models
- **Atomic Design**: Server acts as a generic database API platform

**Implementation:**

- Database operations defined in external migration files
- Core queries (authentication, etc.) referenced by ID from configuration
- Schema flexibility through configurable query templates

### Database Migrations

Database migrations use Lua as a wrapper language for cross-engine compatibility.

**Key Features:**

- **Engine Agnostic**: Single migration works across PostgreSQL, MySQL, SQLite, DB2
- **Lua Wrappers**: `[[string]]` syntax avoids quote nesting issues
- **Compression**: Brotli + base64 encoding for efficient storage
- **Bidirectional**: Each migration includes forward and reverse operations
- **Test Mode**: Full migration cycle testing ensures reliability

**Migration Types:**

- **Schema Changes**: Table creation, column modifications
- **Data Population**: Lookup tables, initial data
- **Assets**: CSS themes, configuration data
- **Diagrams**: JSON metadata for automatic diagram generation (Test 39)

This approach ensures database operations are maintainable, testable, and portable across all supported engines.

## Lua Scripting

Hydrogen uses Lua as a lightweight scripting language for database migrations and configuration processing. Lua is embedded directly into the C application, requiring only ~200KB of additional space.

### Why Lua?

- **Lightweight**: Minimal memory footprint compared to Python or other scripting languages
- **Embedded**: No external dependencies or processes
- **Multiline Strings**: Unique `[[string]]` syntax handles complex content without quote escaping
- **Performance**: Fast execution for migration processing

### Lua Syntax Basics

**Comments:**

```lua
-- This is a comment
```

**Multiline Strings:**

```lua
local sql = [[
CREATE TABLE users (
    id INTEGER PRIMARY KEY,
    name TEXT NOT NULL
);
]]
```

**Nested Strings:**

```lua
-- For deeply nested content
local nested = [=[
This can contain [[regular]] multiline strings
without conflicts
]=]
```

**Macro Replacement:**

```lua
-- Use ${MACRO} syntax (same as Bash)
local table_name = "${DATABASE_TABLE}"
```

### Migration Features

- **Automatic Base64 Encoding**: Multiline strings are automatically encoded for database storage
- **Cross-Engine Compatibility**: Single migration works across PostgreSQL, MySQL, SQLite, DB2
- **Bidirectional Migrations**: Each migration includes both forward and reverse operations
- **Content Types**: Supports SQL, JSON, CSS, HTML, and other text formats

### Testing & Validation

- **[Test 38: Luacheck](../tests/docs/test_38_luacheck.md)** validates Lua syntax and coding standards
- Migration test mode runs forward and reverse operations to ensure reliability
- Automatic diagram generation from JSON metadata in migrations

For detailed migration examples, see the [Helium project documentation](../../002-helium/README.md).
