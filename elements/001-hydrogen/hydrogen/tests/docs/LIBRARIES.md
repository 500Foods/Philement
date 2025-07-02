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

## Future Libraries

As part of the test suite overhaul, additional libraries will be created by modularizing functionalities from existing 'support_*.sh' scripts. Documentation for these new libraries will be added here as they are developed.

- *Placeholder for future libraries to be added during migration.*

## Related Documentation

.

- [Migration Plan](Migration_Plan.md) - Overview of the test suite overhaul strategy, including the process for creating and documenting new libraries.
- [TESTS.md](TESTS.md) - Table of Contents for all test script documentation.
- [README.md](../README.md) - Main documentation for the Hydrogen test suite.
- [Tables Core Library](tables.md) - Documentation for the main table rendering library.
