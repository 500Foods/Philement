# Hydrogen Test Suite Libraries Documentation

## Overview

This document serves as the central Table of Contents (TOC) for the documentation of all library scripts in the 'lib/' directory of the Hydrogen test suite. These libraries provide modular, reusable functionalities to standardize operations across test scripts, supporting the overhaul and migration plan for the test suite.

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
