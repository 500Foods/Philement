# Hydrogen Testing Framework

This directory contains the comprehensive testing framework for the Hydrogen 3D printer control server. It includes test scripts, configuration files, support utilities, and detailed documentation for validating the functionality, performance, and reliability of the Hydrogen system.

## Overview

The Hydrogen testing framework is designed to ensure the robustness and correctness of the server through a structured suite of tests. These tests cover various aspects including compilation, startup/shutdown sequences, API functionality, system endpoints, and code quality checks.

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

Below is a comprehensive list of all test scripts currently available in the suite, organized by category:

### Test Suite Management

- **[test_00_all.sh](docs/test_00_all.md)**: Test suite runner for orchestrating all tests (described above).

### Core Functional Tests

- **[test_01_compilation.sh](docs/test_01_compilation.md)**: Verifies successful compilation and build processes.
- **[test_10_leaks_like_a_sieve.sh](docs/test_10_leaks_like_a_sieve.md)**: Detects memory leaks and resource issues using Valgrind.
- **[test_11_unity.sh](docs/test_11_unity.md)**: Integrates Unity testing framework for unit tests.
- **[test_12_env_variables.sh](docs/test_12_env_variables.md)**: Tests that configuration values can be provided via environment variables.
- **[test_14_env_payload.sh](docs/test_14_env_payload.md)**: Tests environment variable handling for the payload system.
- **[test_16_library_dependencies.sh](docs/test_16_library_dependencies.md)**: Checks for required library dependencies.
- **[test_18_json_error_handling.sh](docs/test_18_json_error_handling.md)**: Tests JSON configuration error handling.
- **[test_20_crash_handler.sh](docs/test_20_crash_handler.md)**: Tests that the crash handler correctly generates and formats core dumps.
- **[test_22_startup_shutdown.sh](docs/test_22_startup_shutdown.md)**: Validates complete startup and shutdown lifecycles.
- **[test_24_signals.sh](docs/test_24_signals.md)**: Tests signal handling (e.g., SIGINT, SIGTERM, SIGHUP).
- **[test_26_shutdown.sh](docs/test_26_shutdown.md)**: Tests the shutdown functionality of the application with a minimal configuration.
- **[test_28_socket_rebind.sh](docs/test_28_socket_rebind.md)**: Tests socket rebinding behavior.
- **[test_30_api_prefixes.sh](docs/test_30_api_prefixes.md)**: Validates API prefix configurations.
- **[test_32_system_endpoints.sh](docs/test_32_system_endpoints.md)**: Tests system endpoint functionality.
- **[test_34_swagger.sh](docs/test_34_swagger.md)**: Verifies Swagger documentation and UI integration.
- **[test_36_websockets.sh](docs/test_36_websockets.md)**: Tests WebSocket server functionality and integration.

### Static Analysis & Code Quality Tests

- **[test_03_code_size.sh](docs/test_03_code_size.md)**: Analyzes code size metrics and file distribution.
- **[test_06_check_links.sh](docs/test_06_check_links.md)**: Validates links in documentation files.
- **[test_91_cppcheck.sh](docs/test_91_cppcheck.md)**: Performs C/C++ static analysis using cppcheck.
- **[test_92_shellcheck.sh](docs/test_92_shellcheck.md)**: Validates shell scripts using shellcheck with exception justification checks.
- **[test_94_eslint.sh](docs/test_94_eslint.md)**: Lints JavaScript files using eslint.
- **[test_95_stylelint.sh](docs/test_95_stylelint.md)**: Validates CSS files using stylelint.
- **[test_96_htmlhint.sh](docs/test_96_htmlhint.md)**: Validates HTML files using htmlhint.
- **[test_97_jsonlint.sh](docs/test_97_jsonlint.md)**: Validates JSON file syntax and structure.
- **[test_98_markdownlint.sh](docs/test_98_markdownlint.md)**: Lints Markdown documentation using markdownlint.
- **[test_99_coverage.sh](docs/test_99_coverage.md)**: Performs comprehensive coverge analysis.

## Configuration Files

The `configs/` directory contains JSON files for various test scenarios:

- **hydrogen_test_min.json**: Minimal configuration for testing core functionality with optional subsystems disabled.
- **hydrogen_test_max.json**: Maximal configuration with all subsystems enabled for full feature testing.
- **hydrogen_test_api_*.json**: Configurations for API-related tests with different prefixes and settings.
- **hydrogen_test_swagger_*.json**: Configurations for testing Swagger UI and documentation endpoints.
- **hydrogen_test_system_endpoints.json**: Configuration for testing system endpoints.

### Port Configuration

To prevent conflicts, tests use dedicated port ranges for different configurations:

| Test Configuration          | Config File                              | Web Server Port | WebSocket Port |
|-----------------------------|------------------------------------------|-----------------|----------------|
| Default Min/Max             | hydrogen_test_min.json                   | 5000            | 5001           |
|                             | hydrogen_test_max.json                   | 5000            | 5001           |
| API Prefix Test             | hydrogen_test_api_test_1.json            | 5030            | 5031           |
|                             | hydrogen_test_api_test_2.json            | 5050            | 5051           |
| Swagger UI Test (Default)   | hydrogen_test_swagger_test_1.json        | 5040            | 5041           |
| Swagger UI Test (Custom)    | hydrogen_test_swagger_test_2.json        | 5060            | 5061           |
| System Endpoints Test       | hydrogen_test_system_endpoints.json      | 5070            | 5071           |

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
