# Hydrogen Test Suite Libraries Documentation

## Overview

This document serves as the central Table of Contents (TOC) for the documentation of all library scripts in the 'lib/' directory of the Hydrogen test suite. These libraries provide modular, reusable functionalities to standardize operations across test scripts, supporting the overhaul and migration plan for the test suite.

## Tables Library

This is a separate executable file that takes a JSON layout file and a JSON data file and produces a nicely formatted ANSI table that has rounded corners, color support, themes, titles, footers, summaries, and so on. More information can be found at its GitHub Repo here: [Tables](https://github.com/500Foods/Scripts/tables.sh/tables.md)

## GitHub Sitemap Library

- **[GitHub Sitemap Library](github-sitemap.md)** - Documentation for `github-sitemap.sh`, a utility for checking markdown links and generating sitemap reports. [Script](/elements/001-hydrogen/hydrogen/tests/lib/github-sitemap.sh)

## Test Framework Libraries

These libraries were created as part of the test suite migration to replace support_utils.sh with modular, focused scripts.

- **[CLOC Library](/docs/H/tests/cloc.md)** - Documentation for `cloc.sh`, providing code analysis and line counting functionality with linting integration for test scripts. [Script](/elements/001-hydrogen/hydrogen/tests/lib/cloc.sh)
- **[Log Output Library](/docs/H/tests/log_output.md)** - Documentation for `log_output.sh`, providing consistent logging, formatting, and display functions for test scripts. [Script](/elements/001-hydrogen/hydrogen/tests/lib/log_output.sh)
- **[File Utils Library](/docs/H/tests/file_utils.md)** - Documentation for `file_utils.sh`, providing file and path manipulation functions for test scripts. [Script](/elements/001-hydrogen/hydrogen/tests/lib/file_utils.sh)
- **[Test Framework Library](/docs/H/tests/framework.md)** - Documentation for `framework.sh`, providing test lifecycle management and result tracking functions. [Script](/elements/001-hydrogen/hydrogen/tests/lib/framework.sh)
- **[Environment Utils Library](/docs/H/tests/env_utils.md)** - Documentation for `env_utils.sh`, providing environment variable handling and validation functions for test scripts. [Script](/elements/001-hydrogen/hydrogen/tests/lib/env_utils.sh)
- **[Lifecycle Management Library](/docs/H/tests/lifecycle.md)** - Documentation for `lifecycle.sh`, providing functions for managing the lifecycle of the Hydrogen application during testing. [Script](/elements/001-hydrogen/hydrogen/tests/lib/lifecycle.sh)
- **[Network Utilities Library](/docs/H/tests/network_utils.md)** - Documentation for `network_utils.sh`, providing network-related functions including TIME_WAIT socket management for test scripts. [Script](/elements/001-hydrogen/hydrogen/tests/lib/network_utils.sh)

## Coverage Libraries

- **[Coverage Analysis System](/docs/H/tests/coverage.md)** - Comprehensive documentation for the coverage analysis system, including `coverage.sh`, `coverage-common.sh`, `coverage-combined.sh`, and the advanced `coverage_table.sh` script for detailed visual coverage reporting.
- **[Coverage Common Library](/docs/H/tests/coverage-common.md)** - Documentation for `coverage-common.sh`, providing shared utilities, batch processing, and ignore pattern management for coverage analysis.
- **[Coverage Combined Library](/docs/H/tests/coverage-combined.md)** - Documentation for `coverage-combined.sh`, providing combined coverage calculation and uncovered file identification.
- **[Coverage Table Library](/docs/H/tests/coverage_table.md)** - Documentation for `coverage_table.sh`, providing professional ANSI table formatting for coverage metrics display.

## CLOC Libraries

- **[CLOC Tables Library](/docs/H/tests/cloc_tables.md)** - Documentation for `cloc_tables.sh`, generating CLOC table data for metrics reporting without test framework overhead.

## Database Migration Libraries

- **[Migrations Lua Module](/docs/H/tests/migrations.md)** - Documentation for `migrations.lua`, providing multi-engine SQL generation and database abstraction for migration testing.
- **[Get Migration Wrapper](/docs/H/tests/get_migration_wrapper.md)** - Documentation for `get_migration_wrapper.lua`, providing command-line interface for database migration SQL generation.

## Profiling Libraries

- **[Profile Test Suite](/docs/H/tests/profile_test_suite.md)** - Documentation for `profile_test_suite.sh`, providing strace-based performance profiling and system call analysis for test scripts.

## Related Documentation

- [TESTING.md](/docs/H/tests/TESTING.md) - Main documentation for the Hydrogen test suite.
