# Hydrogen Test Suite Libraries Documentation

## Overview

This document serves as the central Table of Contents (TOC) for the documentation of all library scripts in the 'lib/' directory of the Hydrogen test suite. These libraries provide modular, reusable functionalities to standardize operations across test scripts, supporting the overhaul and migration plan for the test suite.

## Tables Library

This is an adapted version of the Tables script from [Scripts](https://github.com/500Foods/Scripts).

- **[Tables Core Library](tables.md)** - Documentation for `tables.sh`, the main library for rendering JSON data as ANSI tables. [Script](../lib/tables.sh)
- **[Tables Config Library](tables_config.md)** - Documentation for `tables_config.sh`, focusing on configuration settings for table structures. [Script](../lib/tables_config.sh)
- **[Tables Data Library](tables_data.md)** - Documentation for `tables_data.sh`, focusing on managing data content within tables. [Script](../lib/tables_data.sh)
- **[Tables Datatypes Library](tables_datatypes.md)** - Documentation for `tables_datatypes.sh`, focusing on defining and validating data types for tables. [Script](../lib/tables_datatypes.sh)
- **[Tables Render Library](tables_render.md)** - Documentation for `tables_render.sh`, focusing on rendering and displaying tables. [Script](../lib/tables_render.sh)
- **[Tables Themes Library](tables_themes.md)** - Documentation for `tables_themes.sh`, focusing on visual themes and styling for tables. [Script](../lib/tables_themes.sh)

## GitHub Sitemap Library

- **[GitHub Sitemap Library](github-sitemap.md)** - Documentation for `github-sitemap.sh`, a utility for checking markdown links and generating sitemap reports. [Script](../lib/github-sitemap.sh)

## Test Framework Libraries

These libraries were created as part of the test suite migration to replace support_utils.sh with modular, focused scripts.

- **[CLOC Library](cloc.md)** - Documentation for `cloc.sh`, providing code analysis and line counting functionality with linting integration for test scripts. [Script](../lib/cloc.sh)
- **[Log Output Library](log_output.md)** - Documentation for `log_output.sh`, providing consistent logging, formatting, and display functions for test scripts. [Script](../lib/log_output.sh)
- **[File Utils Library](file_utils.md)** - Documentation for `file_utils.sh`, providing file and path manipulation functions for test scripts. [Script](../lib/file_utils.sh)
- **[Test Framework Library](framework.md)** - Documentation for `framework.sh`, providing test lifecycle management and result tracking functions. [Script](../lib/framework.sh)
- **[Environment Utils Library](env_utils.md)** - Documentation for `env_utils.sh`, providing environment variable handling and validation functions for test scripts. [Script](../lib/env_utils.sh)
- **[Lifecycle Management Library](lifecycle.md)** - Documentation for `lifecycle.sh`, providing functions for managing the lifecycle of the Hydrogen application during testing. [Script](../lib/lifecycle.sh)
- **[Network Utilities Library](network_utils.md)** - Documentation for `network_utils.sh`, providing network-related functions including TIME_WAIT socket management for test scripts. [Script](../lib/network_utils.sh)

## Related Documentation

- [README.md](../README.md) - Main documentation for the Hydrogen test suite.
- [Tables Core Library](tables.md) - Documentation for the main table rendering library.
