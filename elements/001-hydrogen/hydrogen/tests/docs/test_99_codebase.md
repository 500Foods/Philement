# Test 99 Codebase Analysis Script Documentation

## Overview

The `test_99_codebase.sh` script is a comprehensive analysis tool within the Hydrogen test suite, focused on evaluating the codebase's structure, size, and quality through various metrics and linting processes.

## Purpose

This script performs an in-depth analysis of the Hydrogen codebase, including build system cleaning, source code line counting, large file detection, and multi-language linting validation. It is essential for maintaining code quality, identifying potential issues, and ensuring adherence to coding standards across the project.

## Key Features

- **Build System Cleaning**: Uses CMake to clean the build environment, ensuring a fresh state for analysis.
- **Source Code Analysis**: Counts lines of code and categorizes files by size to identify overly large files that may need refactoring.
- **Large File Detection**: Identifies non-source files exceeding a size threshold, which could impact repository performance.
- **Multi-Language Linting**: Validates code quality across C/C++, Markdown, shell scripts, JSON, JavaScript, CSS, and HTML files using tools like cppcheck, markdownlint, shellcheck, and others.

## Usage

To run this test as part of the full suite:

```bash
./test_00_all.sh all
```

To run this test individually:

```bash
./test_99_codebase.sh
```

## Additional Notes

- This test is critical for maintaining the overall health of the codebase, ensuring that it remains manageable, efficient, and compliant with best practices.
- Feedback on the structure or additional analysis metrics to include is welcome during the review process to ensure it meets the needs of the test suite.

## Related Documentation

- [TESTS.md](TESTS.md) - Table of Contents for all test script documentation.
- [LIBRARIES.md](LIBRARIES.md) - Table of Contents for modular library scripts in the 'lib/' directory.
- [Migration Plan](Migration_Plan.md) - Overview of the test suite overhaul strategy.
- [Script](../test_99_codebase.sh) - Link to the test script for navigation on GitHub.
