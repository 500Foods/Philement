# Test 12 Environment Variables Script Documentation

## Overview

The `test_12_env_variables.sh` script (Test 12: Env Var Substitution) is a comprehensive testing tool within the Hydrogen test suite that validates environment variable substitution and configuration handling. This test ensures that configuration values can be properly provided and processed via environment variables.

## Purpose

This script verifies that the Hydrogen server correctly processes environment variable substitution in configuration files, handles missing environment variables with appropriate fallbacks, performs type conversion, and validates environment variable inputs. It is essential for ensuring deployment flexibility through environment-based configuration.

## Key Features

- **Environment Variable Substitution**: Tests how environment variables are substituted into configuration values
- **Missing Variable Fallback**: Validates behavior when environment variables are not set
- **Type Conversion Testing**: Ensures proper conversion of environment variable string values to appropriate data types
- **Environment Variable Validation**: Tests validation mechanisms for environment variable inputs
- **Lifecycle Testing**: Uses the new modular library system for comprehensive startup/shutdown testing

## Technical Details

- **Script Version**: 3.0.1 (Complete rewrite using new modular test libraries)
- **Test Number**: 12 (internally referenced as Test 12)
- **Total Subtests**: 16
- **Architecture**: Library-based using modular scripts from lib/ directory

### Dependencies

The script uses several modular libraries:

- `log_output.sh` - Logging and output formatting
- `file_utils.sh` - File operations and validation
- `framework.sh` - Core testing framework
- `env_utils.sh` - Environment variable utilities
- `lifecycle.sh` - Server lifecycle management

### Test Scenarios

1. **Basic Environment Variables**: Tests standard environment variable substitution
2. **Missing Environment Variables**: Validates fallback behavior when variables are undefined
3. **Type Conversion**: Tests conversion of string environment variables to proper data types
4. **Environment Variable Validation**: Tests validation mechanisms for environment inputs

## Configuration

- **Config File**: `configs/hydrogen_test_min.json`
- **Startup Timeout**: 10 seconds
- **Shutdown Timeout**: 90 seconds (hard limit)
- **Shutdown Activity Timeout**: 5 seconds (no new log activity)

## Usage

To run this test as part of the full suite:

```bash
./test_00_all.sh all
```

To run this test individually:

```bash
./test_12_env_variables.sh
```

## Output and Logging

- **Results Directory**: `tests/results/`
- **Log File**: `tests/logs/hydrogen_test.log`
- **Diagnostics**: `tests/diagnostics/env_variables_test_[timestamp]/`
- **Result Log**: `tests/results/test_12_[timestamp].log`

## Version History

- **3.0.1** (2025-07-06): Added missing shellcheck justifications
- **3.0.0** (2025-07-02): Complete rewrite to use new modular test libraries and match test 15 structure
- **2.1.0** (2025-07-02): Migrated to use modular lib/ scripts
- **2.0.0** (2025-06-17): Major refactoring with shellcheck fixes, improved modularity, enhanced comments
- **1.0.0**: Original version with basic environment variable testing functionality

## Related Documentation

- [test_00_all.md](test_00_all.md) - Main test orchestration system
- [LIBRARIES.md](LIBRARIES.md) - Table of Contents for modular library scripts in the 'lib/' directory
