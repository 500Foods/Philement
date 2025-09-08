# Hydrogen Testing Framework

This folder contains the comprehensive testing framework for the Hydrogen Server.
It includes test scripts, configuration files, support utilities, and detailed documentation for validating the functionality, performance, and reliability of the Hydrogen system.

## Overview

The Hydrogen testing framework is designed to ensure the robustness and correctness of the server through a structured suite of blackbox (aka integration) tests. These tests cover various aspects including compilation, startup/shutdown sequences, API functionality, system endpoints, and code quality checks.

NOTE: A separate document covers the [Unity Unit Test Framework](./UNITY.md).

## Core Components

### Test Suite Runner (test_00_all.sh)

**test_00_all.sh** is the central orchestration script for executing the entire test suite. It manages the execution of all test scripts, either in parallel batches or sequentially, and generates a comprehensive summary of results.

- **Version**: 4.0.0 (Last updated: 2025-07-04)
- **Key Features**:
  - Executes tests in parallel batches grouped by tens digit (e.g., 0x, 1x, 2x) for improved performance, or sequentially if specified.
  - Supports skipping test execution for quick summary generation or README updates.
  - Automatically updates the project README.md with test results and repository statistics using the `cloc` tool.
  - Provides detailed output with test status, subtest counts, pass/fail statistics, and elapsed time.
- **Usage**:

  ```bash
  # Run all tests in parallel batches
  ./test_00_all.sh

  # Run all tests sequentially
  ./test_00_all.sh --sequential

  # Run specific tests
  ./test_00_all.sh 01_compilation 22_startup_shutdown

  # Skip test execution and update README
  ./test_00_all.sh --skip-tests
  ```

## Test Scripts

Below is a comprehensive list of all test scripts currently available in the suite, organized by category.
With the exception of Test 01 (Compilation) and Test 11 (Unity) these are all considered "blackbox" tests in that they simply run the server and test it from an external point of view.
Test 01 is just the script that bulids everything, so not really a test in itself, though it does do a number of checks for important things.  
And Test 11 is all about Unity unit tests, where we have custom code written to call our app's functions directly, which is described in more detail below.

### Test Suite Management - Orchestration

- **[test_00_all.sh](docs/test_00_all.md)**: Test suite runner for orchestrating all tests (described above).

### Compilation and Static Analysis

- **[test_01_compilation.sh](docs/test_01_compilation.md)**: Verifies successful compilation and build processes.
- **[test_02_secrets.sh](docs/test_02_secrets.md)**: Checks validity of key environment variables
- **[test_04_check_links.sh](docs/test_04_check_links.md)**: Validates links in documentation files.

### Core Functional Tests

- **[test_10_unity.sh](docs/test_10_unity.md)**: Integrates Unity testing framework for unit tests.
- **[test_11_leaks_like_a_sieve.sh](docs/test_11_leaks_like_a_sieve.md)**: Detects memory leaks and resource issues using Valgrind.
- **[test_12_env_variables.sh](docs/test_12_env_variables.md)**: Tests that configuration values can be provided via environment variables.
- **[test_13_crash_handler.sh](docs/test_13_crash_handler.md)**: Tests that the crash handler correctly generates and formats core dumps.
- **[test_14_library_dependencies.sh](docs/test_14_library_dependencies.md)**: Checks for required library dependencies.
- **[test_15_json_error_handling.sh](docs/test_15_json_error_handling.md)**: Tests JSON configuration error handling.
- **[test_16_shutdown.sh](docs/test_16_shutdown.md)**: Tests the shutdown functionality of the application with a minimal configuration.
- **[test_17_startup_shutdown.sh](docs/test_17_startup_shutdown.md)**: Validates complete startup and shutdown lifecycles.
- **[test_18_signals.sh](docs/test_18_signals.md)**: Tests signal handling (e.g., SIGINT, SIGTERM, SIGHUP).
- **[test_19_socket_rebind.sh](docs/test_19_socket_rebind.md)**: Tests socket rebinding behavior.

### Server Tests

- **[test_20_api_prefix.sh](docs/test_20_api_prefix.md)**: Validates API prefix configurations.
- **[test_21_system_endpoints.sh](docs/test_21_system_endpoints.md)**: Tests system endpoint functionality.
- **[test_22_swagger.sh](docs/test_22_swagger.md)**: Verifies Swagger documentation and UI integration.
- **[test_23_websockets.sh](docs/test_23_websockets.md)**: Tests WebSocket server functionality and integration.
- **[test_24_uploads.sh](docs/test_24_uploads.md)**: Tests uploading a file to the server
- **[test_25_mdns.sh](docs/test_25_mdns.md)**: Tests mDNS server and client functionality and integration

### Static Analysis & Code Quality Tests

- **[test_90_markdownlint.sh](docs/test_90_markdownlint.md)**: Lints Markdown documentation using markdownlint.
- **[test_91_cppcheck.sh](docs/test_91_cppcheck.md)**: Performs C/C++ static analysis using cppcheck.
- **[test_92_shellcheck.sh](docs/test_92_shellcheck.md)**: Validates shell scripts using shellcheck with exception justification checks.
- **[test_93_jsonlint.sh](docs/test_93_jsonlint.md)**: Validates JSON file syntax and structure.
- **[test_94_eslint.sh](docs/test_94_eslint.md)**: Lints JavaScript files using eslint.
- **[test_95_stylelint.sh](docs/test_95_stylelint.md)**: Validates CSS files using stylelint.
- **[test_96_htmlhint.sh](docs/test_96_htmlhint.md)**: Validates HTML files using htmlhint.
- **[test_97_xmllint.sh](docs/test_97_xmllint.md)**: Validates XML/SVG files using xmllint.
- **[test_98_code_size.sh](docs/test_98_code_size.md)**: Analyzes code size metrics and file distribution.
- **[test_99_coverage.sh](docs/test_99_coverage.md)**: Performs comprehensive coverge analysis.

## Configuration Files

The `configs/` directory contains JSON files for various test scenarios:

## Linting Configurations

Linting behavior is controlled by configuration files to ensure code quality:

- **.lintignore**: Global exclusion patterns for all linting tools.
- **.lintignore-c**: C/C++ specific configurations for `cppcheck`, defining enabled checks and suppressions.
- **.lintignore-markdown**: Markdown linting rules for documentation consistency.

## Test Output Directories

Test execution generates output in the following directories, which are cleaned at the start of each run:

- **./logs/**: Detailed execution logs for each test run.
- **./results/**: Summaries, build outputs, and result data.
- **./diagnostics/**: Diagnostic data and analysis for debugging issues.

## Documentation

Detailed documentation for each test script and library is available in the `docs/` directory:

- **test_00_all.md**: Documentation for the test suite runner.
- **test_01_compilation.md** to **test_99_cleanup.md**: Individual test script documentation.
- **LIBRARIES.md**: Overview of modular library scripts in the `lib/` directory.
- Additional guides and reference materials for test framework components.

## Developing New Tests

When creating new test scripts, adhere to the following standards:

1. **Naming Convention**: Use `test_NN_descriptive_name.sh` following the numbering system.
2. **Headers**: Include version, title, and change history at the top of the script.
3. **Shellcheck Compliance**: Ensure scripts pass `shellcheck` validation for quality and consistency.
4. **Integration**: Ensure compatibility with `test_00_all.sh` for automatic discovery and execution.
5. **Documentation**: Add corresponding documentation in the `docs/` directory and update this README.md.

### Workflow for New Tests

1. Copy an existing test script as a template.
2. Modify the script with necessary test logic and headers.
3. Validate with `shellcheck` to ensure compliance.
4. Test the script independently before integrating into the suite.
5. Document the test purpose and usage in the `docs/` directory.

## Usage Examples

### Running the Full Test Suite

```bash
# Run all tests in parallel batches (default mode)
./test_00_all.sh all

# Run all tests sequentially
./test_00_all.sh --sequential
```

### Running Specific Tests

```bash
# Run specific tests sequentially
./test_00_all.sh 01_compilation 22_startup_shutdown
```

### Skipping Test Execution

```bash
# Skip test execution but update README with what would run
./test_00_all.sh --skip-tests
```

### Manual Testing with Configurations

```bash
# Run Hydrogen with minimal configuration
../hydrogen ./configs/hydrogen_test_min.json

# Run Hydrogen with maximal configuration
../hydrogen ./configs/hydrogen_test_max.json
```

For more detailed information on the Hydrogen testing approach, refer to the [Testing Documentation](../docs/testing.md).

## Test Renumbering / Renaming

For various reasons, we sometimes renumber or rename tests. Often this is to ensure that the ordering in the main orchestration runs is optimal.
Sometimes it is because a test function changes. Or maybe due to just wanting the ordering to be more sensible. Whatever the reason,
this causes a number of issues that need to be resolved, mostly involving links to the file.

- STRUCTURE.md must be updated to reflect the new name/number
- tests/docs must be updated with the new documentation for the test
- tests/README.md must be updated with a new link to the test and documentation
- SITEMAP.md must be updated to reflect the new documentation
- RELEASES.md must be reviewed as this will undoubtedly need to be noted in the release notes
- RECIPE.md must be updated with the new test list
