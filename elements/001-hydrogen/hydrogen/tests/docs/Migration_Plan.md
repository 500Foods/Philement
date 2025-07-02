# Hydrogen Test Suite Migration Plan

## Overview

This document outlines the plan for overhauling the Hydrogen test suite to standardize functions, modularize support scripts into the 'lib/' directory, refactor repetitive code, enable parallel test execution, and incorporate detailed user feedback. The migration will be conducted incrementally, one script at a time, with careful review at each step to ensure alignment with the desired system architecture.

The primary goals are:

- Create new, focused modular scripts in the 'lib/' directory to replace functionalities currently in 'support_*.sh' scripts.
- Update 'test_*.sh' scripts to exclusively use the new 'lib/' scripts, phasing out reliance on old 'support_*.sh' scripts.
- Identify and refactor repetitive or custom code in 'test_*.sh' and 'support_*.sh' scripts into reusable 'lib/' functions.
- Design the updated test suite to support parallel test execution.
- Document progress, new functions, and user feedback meticulously to maintain clarity and continuity.

## Current Script Inventory and Migration Status

### Test Scripts

These scripts perform specific tests for the Hydrogen project and will be updated to use new 'lib/' scripts.

| Script Name                       | Type   | Migration Status | Notes                                      |
|-----------------------------------|--------|------------------|--------------------------------------------|
| test_00_all.sh                   | Test   | Not Migrated     | To be updated last as per user request.    |
| test_10_compilation.sh           | Test   | Migrated         | Successfully migrated to use lib/ scripts. |
| test_12_env_payload.sh           | Test   | Not Migrated     |                                            |
| test_15_startup_shutdown.sh      | Test   | Not Migrated     |                                            |
| test_20_shutdown.sh              | Test   | Not Migrated     |                                            |
| test_25_library_dependencies.sh  | Test   | Not Migrated     |                                            |
| test_30_unity_tests.sh           | Test   | Not Migrated     |                                            |
| test_35_env_variables.sh         | Test   | Not Migrated     |                                            |
| test_40_json_error_handling.sh   | Test   | Not Migrated     |                                            |
| test_45_signals.sh               | Test   | Not Migrated     |                                            |
| test_50_crash_handler.sh         | Test   | Not Migrated     |                                            |
| test_55_socket_rebind.sh         | Test   | Not Migrated     |                                            |
| test_60_api_prefixes.sh          | Test   | Not Migrated     |                                            |
| test_65_system_endpoints.sh      | Test   | Not Migrated     |                                            |
| test_70_swagger.sh               | Test   | Not Migrated     |                                            |
| test_95_leaks_like_a_sieve.sh    | Test   | Not Migrated     |                                            |
| test_98_check_links.sh           | Test   | Not Migrated     |                                            |
| test_99_codebase.sh              | Test   | Not Migrated     |                                            |
| test_template.sh                 | Test   | Not Migrated     |                                            |

### Support Scripts

These scripts provide utility functions for tests and will serve as the basis for new modular 'lib/' scripts. They will remain in place for compatibility with unmigrated test scripts.

| Script Name                          | Type    | Migration Status | Notes                                      |
|--------------------------------------|---------|------------------|--------------------------------------------|
| support_analyze_stuck_threads.sh    | Support | Not Migrated     | To be modularized into 'lib/' scripts.     |
| support_cleanup.sh                  | Support | Not Migrated     | To be modularized into 'lib/' scripts.     |
| support_cppcheck.sh                 | Support | Not Migrated     | To be modularized into 'lib/' scripts.     |
| support_monitor_resources.sh        | Support | Not Migrated     | To be modularized into 'lib/' scripts.     |
| support_sitemap.sh                  | Support | Not Migrated     | To be modularized into 'lib/' scripts.     |
| support_tables_test_01_basic.sh     | Support | Not Migrated     | To be modularized into 'lib/' scripts.     |
| support_tables.sh                   | Support | Not Migrated     | To be modularized into 'lib/' scripts.     |
| support_timewait.sh                 | Support | Not Migrated     | To be modularized into 'lib/' scripts.     |
| support_utils.sh                    | Support | Not Migrated     | To be modularized into 'lib/' scripts.     |

### Existing 'lib/' Scripts

These scripts are already in the 'lib/' directory and may be integrated or expanded during the migration.

| Script Name               | Type    | Notes                                      |
|---------------------------|---------|--------------------------------------------|
| tables_config.sh         | Library | Existing modular script for table config.  |
| tables_data.sh           | Library | Existing modular script for table data.    |
| tables_datatypes.sh      | Library | Existing modular script for datatypes.     |
| tables_render.sh         | Library | Existing modular script for rendering.     |
| tables_themes.sh         | Library | Existing modular script for themes.        |

## Migration Instructions

### General Process

1. **Preparation**:
   - Create a backup of the entire 'tests/' directory before starting any migration to prevent loss of functionality.
   - Review the next test script to be migrated (starting with 'test_10_compilation.sh') to understand its dependencies on 'support_*.sh' scripts and any custom code.
   - Run `shellcheck -x -f gcc <script.sh>` on the script to identify and address any shell scripting issues or warnings before migration.

2. **Creating New 'lib/' Scripts**:
   - Analyze 'support_*.sh' scripts to identify functionalities that can be modularized.
   - Break down functionalities into focused scripts in the 'lib/' directory. For example:
     - From 'support_utils.sh', create scripts like 'lib/log_formatting.sh' for output formatting, 'lib/server_startup.sh' for server initialization, and 'lib/server_shutdown.sh' for server termination.
     - Ensure each new script has a specific purpose to enhance maintainability.
   - Document each new 'lib/' script with its purpose, functions, and usage instructions (see 'New 'lib/' Scripts Documentation' section below).

3. **Refactoring Repetitive Code**:
   - During the review of each 'test_*.sh' and 'support_*.sh' script, identify custom or repetitive code blocks.
   - Extract these into new reusable functions within appropriate 'lib/' scripts. For instance, repeated test setup code in multiple test scripts can be moved to 'lib/test_setup.sh'.
   - Update the test script being migrated to call these new functions instead of using inline code.

4. **Updating Test Scripts**:
   - Select the next test script to migrate based on the sequence (excluding 'test_00_all.sh' until last).
   - Replace all 'source' commands and function calls to 'support_*.sh' scripts with equivalents from the new 'lib/' scripts.
   - If a required functionality is not yet available in a 'lib/' script, create a new one as per step 2.
   - Adjust the script to support parallel execution by ensuring unique log file naming, independent resource usage, and minimal inter-script dependencies (see 'Parallel Test Execution' below).

5. **Validation and Review**:
   - Run the updated test script to confirm it functions correctly with the new 'lib/' scripts.
   - Review the changes with the user to ensure they align with the desired system architecture. Capture feedback and make adjustments as needed.
   - Update the migration status in this document after each script is successfully migrated.

6. **Documentation**:
   - Update the 'Migration Tracking' table with the new status and notes.
   - Document any new 'lib/' scripts or refactored functions in the respective sections below.

### Parallel Test Execution

To enable parallel test execution, the following strategies will be applied during script updates:

- **Unique Resource Naming**: Ensure log files, temporary files, and other resources use unique identifiers (e.g., timestamps or test-specific prefixes) to prevent conflicts when tests run simultaneously.
- **Independent Operation**: Minimize dependencies between test scripts, allowing them to run without requiring sequential execution.
- **Resource Management**: Design server startup/shutdown and other shared resource interactions to handle multiple instances gracefully, potentially using port allocation or configuration overrides for parallel runs.
- These considerations will be documented for each updated script to track how parallel execution is supported.

### Review and Feedback

Given the user's strong opinions on the new system, each migration step will include a review phase:

- After updating a test script or creating new 'lib/' scripts, present the changes for user feedback.
- Incorporate user preferences on script structure, function naming, and overall design.
- Document feedback in the 'User Review Feedback' section to maintain a record of decisions and adjustments.

## Migration Tracking

### Scripts Migration Status

This table will be updated as migration progresses to reflect the current status of each script.

| Script Name                       | Type    | Migration Status | Notes                                      | Date Updated       |
|-----------------------------------|---------|------------------|--------------------------------------------|--------------------|
| test_00_all.sh                   | Test    | Not Migrated     | To be updated last.                       | -                  |
| test_10_compilation.sh           | Test    | Migrated         | Successfully migrated to use lib/ scripts | 2025-07-02         |
| test_12_env_payload.sh           | Test    | Migrated         | Successfully migrated to use lib/ scripts | 2025-07-02         |
| test_15_startup_shutdown.sh      | Test    | Migrated         | Successfully migrated to use lib/ scripts | 2025-07-02         |
| test_20_shutdown.sh              | Test    | Migrated         | Successfully migrated to use lib/ scripts, added validate_config_file function for single config validation, corrected subtest count for accurate reporting | 2025-07-02         |
| test_25_library_dependencies.sh  | Test    | Migrated         | Successfully migrated to use lib/ scripts, fixed parameter order issues in start_hydrogen calls, corrected function signatures, and resolved subtest counting for accurate dependency testing | 2025-07-02         |
| test_30_unity_tests.sh           | Test    | Migrated         | Successfully migrated to use lib/ scripts, updated log_output.sh with DATA icon and pale yellow color for output visibility, adjusted subtest naming to 'Enumerate Unity Tests' for clarity | 2025-07-02         |
| test_35_env_variables.sh         | Test    | Migrated         | Successfully migrated to use lib/ scripts, restructured to match test 15 pattern with proper library function usage, added missing environment utility functions | 2025-07-02         |
| test_40_json_error_handling.sh   | Test    | Not Migrated     |                                           | -                  |
| test_45_signals.sh               | Test    | Not Migrated     |                                           | -                  |
| test_50_crash_handler.sh         | Test    | Not Migrated     |                                           | -                  |
| test_55_socket_rebind.sh         | Test    | Not Migrated     |                                           | -                  |
| test_60_api_prefixes.sh          | Test    | Not Migrated     |                                           | -                  |
| test_65_system_endpoints.sh      | Test    | Not Migrated     |                                           | -                  |
| test_70_swagger.sh               | Test    | Not Migrated     |                                           | -                  |
| test_95_leaks_like_a_sieve.sh    | Test    | Not Migrated     |                                           | -                  |
| test_98_check_links.sh           | Test    | Not Migrated     |                                           | -                  |
| test_99_codebase.sh              | Test    | Not Migrated     |                                           | -                  |
| test_template.sh                 | Test    | Not Migrated     |                                           | -                  |
| support_analyze_stuck_threads.sh | Support | Not Migrated     | To be modularized into 'lib/' scripts.    | -                  |
| support_cleanup.sh               | Support | Not Migrated     | To be modularized into 'lib/' scripts.    | -                  |
| support_cppcheck.sh              | Support | Not Migrated     | To be modularized into 'lib/' scripts.    | -                  |
| support_monitor_resources.sh     | Support | Not Migrated     | To be modularized into 'lib/' scripts.    | -                  |
| support_sitemap.sh               | Support | Not Migrated     | To be modularized into 'lib/' scripts.    | -                  |
| support_tables_test_01_basic.sh  | Support | Not Migrated     | To be modularized into 'lib/' scripts.    | -                  |
| support_tables.sh                | Support | Not Migrated     | To be modularized into 'lib/' scripts.    | -                  |
| support_timewait.sh              | Support | Not Migrated     | To be modularized into 'lib/' scripts.    | -                  |
| support_utils.sh                 | Support | Not Migrated     | To be modularized into 'lib/' scripts.    | -                  |

## New 'lib/' Scripts Documentation

### log_output.sh Updates

- **Purpose**: Provides consistent logging, formatting, and display functions for test scripts.
- **New Function Added**: `format_file_size` - Formats file sizes with thousands separators for better readability in output.
- **Usage**: Used by test scripts to format output of file sizes, ensuring consistency across all test logs.

### framework.sh Updates

- **Purpose**: Provides test lifecycle management and result tracking functions.
- **New Function Added**: `navigate_to_project_root` - Navigates to the project root directory, ensuring scripts operate from the correct location.
- **Usage**: Used by test scripts to set up the environment by navigating to the project root before executing test operations.

### env_utils.sh Updates

- **Purpose**: Provides functions for environment variable handling and validation.
- **New Functions Added**:
  - `check_env_var` - Checks if an environment variable is set and non-empty.
  - `validate_rsa_key` - Validates the format of RSA keys (private or public).
  - `set_type_conversion_environment` - Sets environment variables for testing type conversion (string to boolean, number, etc.).
  - `set_validation_test_environment` - Sets environment variables with invalid values for testing validation logic.
- **Usage**: Used by test scripts to ensure required environment variables are set and valid, particularly for payload security configurations and environment variable substitution testing.
- **Version**: Updated to 1.1.0 on 2025-07-02 for test_35_env_variables.sh migration support.

## User Review Feedback

- **2025-07-02**: User feedback on initial migration of `test_10_compilation.sh` emphasized the importance of modularizing scripts into the 'lib/' directory, refactoring repetitive code, and ensuring parallel test execution support. This feedback has been integrated into the migration plan.
- **2025-07-02**: User requested that new functions be placed in existing relevant 'lib/' scripts rather than creating generically named files like `test_utils.sh`. As a result, functions were redistributed to `log_output.sh` and `framework.sh`, and `test_utils.sh` was removed.

## Lessons Learned from Migrated Scripts

### test_10_compilation.sh

- **Migration Date**: 2025-07-02
- **Lessons Learned**:
  - Modularizing scripts into the 'lib/' directory improves maintainability and reusability of code.
  - Refactoring repetitive code into functions (e.g., file size formatting, directory navigation) reduces duplication and enhances consistency.
  - Ensuring parallel test execution support requires careful management of resource naming and independent operation, which should be considered in future migrations.
  - Detailed documentation and user feedback integration are crucial for aligning the migration with user expectations and system architecture goals.

### test_12_env_payload.sh

- **Migration Date**: 2025-07-02
- **Lessons Learned**:
  - Properly scoping variables within functions is essential to avoid errors like 'local' variable usage outside functions.
  - All console output must be routed through logging functions (e.g., log_output.sh) to maintain consistency and prevent unexpected messages.
  - Displaying actual commands in EXEC lines enhances transparency and aids debugging, making test execution more understandable.
  - Repetitive test logic, such as tracking passed/failed checks, can be abstracted into reusable functions in 'framework.sh' to streamline test scripts.
  - Continuous user feedback during migration helps refine the approach, ensuring the final structure meets expectations and addresses issues promptly.

### test_15_startup_shutdown.sh

- **Migration Date**: 2025-07-02
- **Lessons Learned**:
  - Creating domain-specific libraries (e.g., lifecycle.sh) is crucial for complex test scenarios like application lifecycle management to encapsulate specialized functionality such as process startup, shutdown, and monitoring.
  - Dynamic process management, including PID tracking and diagnostics capture, is essential for runtime behavior tests and should be prioritized in library design for similar tests.
  - Detailed diagnostics and logging are vital for debugging failures in lifecycle tests, requiring robust mechanisms to capture process state and log activity during test execution.

### test_20_shutdown.sh

- **Migration Date**: 2025-07-02
- **Lessons Learned**:
  - Ensuring accurate function names and signatures from library scripts is critical to avoid runtime errors during test execution.
  - Adjusting the total subtest count (`TOTAL_SUBTESTS`) to match the actual number of executed subtests is necessary for consistent reporting, especially when tests are run through an orchestrator.
  - Adding modular functions to library scripts (e.g., `validate_config_file` for single configuration validation) supports varied test requirements and enhances reusability across different test scripts.

### test_30_unity_tests.sh

- **Migration Date**: 2025-07-02
- **Lessons Learned**:
  - Migrating tests that involve external frameworks like Unity requires careful integration with existing library functions for output and diagnostics to maintain consistency in reporting.
  - Dynamic subtest counting based on runtime results (e.g., from `ctest` output) is essential for accurate test reporting when the number of tests isn't fixed.
  - Preserving specialized functionality like framework downloading and compilation within the test script can be necessary if not generalizable to other tests, but should still use library functions for logging and result reporting.

### test_25_library_dependencies.sh

- **Migration Date**: 2025-07-02
- **Lessons Learned**:
  - **🚨 CRITICAL: Library Function Reuse Priority**: Always start by identifying which existing library functions can be used before writing custom implementations. Test 25 initially used custom startup/shutdown code that duplicated functionality already available in `start_hydrogen` and `stop_hydrogen` from lifecycle.sh. This violated the core principle of the migration project: code reuse and consistency.
  - **Process Management Consistency**: Use library functions like `start_hydrogen` and `stop_hydrogen` for all process lifecycle management. These functions properly handle job control, timing extraction, and error conditions without the "Killed" messages that appear with custom implementations.
  - **Avoid Reinventing the Wheel**: When a test has specialized needs (like dependency checking), combine the specialized logic with existing library functions rather than reimplementing common functionality like process startup/shutdown.
  - **Configuration Path Issues**: Always verify config file paths are correct - test configs are in `tests/configs/` not `hydrogen/configs/`. Empty or invalid config files cause "failed to load configuration" errors.
  - **Library Functions Handle Edge Cases**: Library functions like `start_hydrogen` already handle timing extraction, job control message suppression, and proper error handling. Custom implementations often miss these details.
  - **Version Information Extraction**: Log parsing can extract detailed version information (Expected vs Found versions) from dependency checks, providing much more valuable output than simple pass/fail results.
  - **Migration Approach**: When migrating tests, first identify what library functions exist, then adapt the test to use them. Don't start by copying old custom code patterns.

### test_35_env_variables.sh

- **Migration Date**: 2025-07-02
- **Lessons Learned**:
  - **🚨 CRITICAL: Structural Consistency is Essential**: Test 35 was initially marked as "migrated" but didn't follow the standardized structure established in test 15. This highlighted the importance of not just using library functions, but following the complete structural pattern for consistency across all tests.
  - **Follow Established Patterns**: When a reference test (like test 15) establishes a clean, standardized pattern, all subsequent tests should follow the same structure: header format, variable organization, library sourcing, subtest management, and main execution flow.
  - **Avoid Custom Implementations**: Test 35 initially had custom functions like `run_basic_env_test()`, `run_fallback_test()`, etc., instead of using the standardized `run_lifecycle_test()` function from lifecycle.sh. This violated the principle of code reuse and consistency.
  - **Library Function Completeness**: When migrating specialized tests, ensure all required utility functions are available in the appropriate library scripts. Test 35 required additional environment setup functions (`set_type_conversion_environment`, `set_validation_test_environment`) that needed to be added to env_utils.sh.
  - **Subtest Count Accuracy**: The `TOTAL_SUBTESTS` count must accurately reflect the actual number of subtests being executed. Test 35 was adjusted from 10 to 12 subtests to match the actual test structure.
  - **Migration Verification**: Just because a test is marked as "migrated" doesn't mean it follows the established patterns. Regular verification against reference tests (like test 15) is essential to ensure true consistency.
  - **Environment Variable Testing Strategy**: Environment variable tests should use the standardized lifecycle testing approach with different environment configurations rather than custom startup/shutdown implementations.
  - **🚨 CRITICAL: Avoid Duplicate PASS/FAIL Results**: When library functions already generate PASS/FAIL results (like `validate_config_file`), the calling test should not add duplicate results. Only increment the pass counter and add informational messages. This ensures each subtest has exactly one PASS/FAIL line.
  - **Subtest Count Verification**: After migration, always verify that the actual number of subtests executed matches the `TOTAL_SUBTESTS` declaration. Use the test output to count the actual subtests (35-001, 35-002, etc.) and adjust accordingly.
  - **Library Function Result Handling**: When using library functions that generate their own results, understand what they output before adding additional result lines. Check the library function implementation to see if it calls `print_result`.
