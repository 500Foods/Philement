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
| test_10_compilation.sh           | Test   | Not Migrated     | Planned as the first script to update.     |
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
| test_10_compilation.sh           | Test    | Not Migrated     | Planned as first to update.               | -                  |
| test_12_env_payload.sh           | Test    | Not Migrated     |                                           | -                  |
| test_15_startup_shutdown.sh      | Test    | Not Migrated     |                                           | -                  |
| test_20_shutdown.sh              | Test    | Not Migrated     |                                           | -                  |
| test_25_library_dependencies.sh  | Test    | Not Migrated     |                                           | -                  |
| test_30_unity_tests.sh           | Test    | Not Migrated     |                                           | -                  |
| test_35_env_variables.sh         | Test    | Not Migrated     |                                           | -                  |
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

### New 'lib/' Scripts Documentation

This section will document new scripts created in the 'lib/' directory as part of the migration. Each entry will include the script's purpose, key functions, and usage instructions.

- **No new 'lib/' scripts created yet.**
  - *Details will be added as scripts are created during migration.*

### Refactored Code Documentation

This section will document repetitive or custom code refactored from 'test_*.sh' or 'support_*.sh' scripts into new 'lib/' functions.

- **No code refactored yet.**
  - *Details will be added as code is refactored during migration.*

### User Review Feedback

This section will capture user feedback on updated scripts and new 'lib/' scripts to ensure the system aligns with the desired architecture.

- **No feedback recorded yet.**
  - *Feedback will be added after each review cycle during migration.*

## Summary of Remaining Tasks

As of the creation of this document, all scripts are listed as "Not Migrated." The initial focus will be on updating 'test_10_compilation.sh' as the first test script. Subsequent tasks include:

- Creating new modular 'lib/' scripts based on functionalities in 'support_*.sh' scripts.
- Identifying and refactoring repetitive code in test and support scripts.
- Updating each test script to use the new 'lib/' scripts, ensuring support for parallel execution.
- Reviewing changes with the user after each update to incorporate feedback.

This document will be updated after each migration step to reflect the current status, new scripts, refactored code, and user feedback.

## Related Documentation

- [TESTS.md](TESTS.md) - Table of Contents for all test script documentation.
- [LIBRARIES.md](LIBRARIES.md) - Table of Contents for modular library scripts in the 'lib/' directory.
- [README.md](../README.md) - Main documentation for the Hydrogen test suite.
