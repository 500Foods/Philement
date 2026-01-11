# Test 03: Shell Environment Variables

## Overview

Test 03 validates environment variables used in the Hydrogen project configuration. This test ensures that all required and optional environment variables referenced in JSON configuration files are properly documented and configured.

## Purpose

The primary purpose of this test is to identify and document all environment variables that the Hydrogen project depends on, ensuring that critical dependencies like `HYDROGEN_ROOT` are not overlooked. Many important environment variables are buried deep within the build system or configuration files (such as `HYDROGEN_DEV_EMAIL` which sends completion notifications), and this test helps surface these obscurities to provide a smooth developer experience for anyone working with the project.

The test serves two main functions:

1. **Validation of Defined Variables**: Checks that environment variables explicitly defined in the test's configuration array are set and valid.

2. **Discovery of Missing Variables**: Automatically searches the project's JSON configuration files for environment variable references that are not yet documented in the test, failing the test and prompting for their addition.

## Test Structure

The test maintains an array of environment variables with the following information for each:

- **Variable Name**: The environment variable name (e.g., `HYDROGEN_DEV_EMAIL`)
- **Tests Used In**: Comma-separated list of test numbers where this variable is referenced
- **Brief Description**: Short description of the variable's purpose
- **Long Description**: Detailed explanation of why the variable is needed and its impact

## Environment Variables Checked

### Project Root Variables

- `HYDROGEN_ROOT`: Root directory path of the Hydrogen project (required for all scripts)
- `HELIUM_ROOT`: Root directory path of the Helium project (required for payload generation)

### Core Variables

- `HYDROGEN_DEV_EMAIL`: Developer email for notifications and support
- `PAYLOAD_KEY`: RSA private key for payload encryption
- `WEBSOCKET_KEY`: Authentication key for WebSocket connections

### Database Variables

- `CANVAS_DB_HOST`, `CANVAS_DB_USER`, `CANVAS_DB_PASS`: Canvas database connection
- `ACURANZO_DB_HOST`, `ACURANZO_DB_USER`, `ACURANZO_DB_PASS`: Acuranzo database connection

## Execution Flow

1. **Initialization**: Sets up test framework and loads environment variable definitions
2. **Defined Variable Checks**: Iterates through the predefined array, checking each variable
3. **Project Search**: Scans JSON config files for `${env.VARIABLE}` patterns and shell scripts for `${VARIABLE}` patterns
4. **Missing Variable Detection**: Identifies and fails on undocumented variables found in project files
5. **Reporting**: Provides detailed pass/fail status with descriptions and file locations

## Expected Output

For each environment variable:

- **PASS**: Variable is set and non-empty
- **FAIL**: Variable is missing or empty, with long description displayed
- **Additional FAIL**: Undocumented variables found in configs

## Maintenance

When new environment variables are added to JSON configurations, they should be added to the `ENV_VARS` array in `test_03_shell.sh` with appropriate descriptions before the test will pass.

## Dependencies

- Bash shell with associative array support
- `grep` and `sed` for configuration parsing
- Hydrogen test framework libraries