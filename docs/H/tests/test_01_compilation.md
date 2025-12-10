# Test 01 Compilation Script Documentation

## Overview

The `test_01_compilation.sh` script is a comprehensive build verification tool within the Hydrogen test suite, focused on ensuring that all components of the Hydrogen project compile successfully using the CMake build system.

## Purpose

This script validates the complete build process for the Hydrogen server and related components, ensuring that the codebase is in a compilable state before further testing. It is a prerequisite for other tests and is run first by `test_00_all.sh`.

## Key Features

- **Library-Based Architecture**: Uses modular library scripts for logging, file utilities, and framework functions
- **CMake Build System**: Tests compilation using CMake with preset configurations
- **Multiple Build Variants**: Verifies compilation of all build variants including:
  - Default build (`hydrogen`)
  - Debug build (`hydrogen_debug`)
  - Performance build (`hydrogen_perf`)
  - Valgrind build (`hydrogen_valgrind`)
  - Release build (`hydrogen_release`)
  - Naked build (`hydrogen_naked`)
- **Examples Compilation**: Tests compilation of OIDC client examples
- **Comprehensive Validation**: Performs 14 subtests covering all aspects of the build process
- **File Size Reporting**: Reports executable sizes for verification
- **Fast Failure**: Fails immediately if any component fails to compile

## Script Architecture

### Version Information

- **Current Version**: 2.1.0
- **Major Rewrite**: Version 2.0.0 introduced complete library-based architecture

### Library Dependencies

The script sources several library modules:

- `log_output.sh` - Log output formatting and printing functions
- `file_utils.sh` - File utility functions for size checking and navigation
- `framework.sh` - Test framework functions for subtest management

### Test Execution Flow

1. **Environment Setup**: Check CMake availability and project structure
2. **Build Configuration**: Configure CMake with default preset
3. **Variant Compilation**: Build all executable variants
4. **Executable Verification**: Verify all expected executables were created
5. **Examples Build**: Compile OIDC client examples
6. **Examples Verification**: Verify example executables were created

## Subtests Performed

1. **Check CMake Availability** - Verifies CMake is installed and accessible
2. **Check CMakeLists.txt** - Confirms CMake configuration file exists
3. **Check Source Files** - Validates source directory and main source file
4. **Create Build Directory** - Creates/verifies build directory structure
5. **CMake Configuration** - Configures build with default preset
6. **Check and Generate Payload** - Verifies or generates payload files
7. **Build All Variants** - Compiles all executable variants
8. **Verify Default Executable** - Confirms `hydrogen` executable creation
9. **Verify Debug Executable** - Confirms `hydrogen_debug` executable creation
10. **Verify Performance Executable** - Confirms `hydrogen_perf` executable creation
11. **Verify Valgrind Executable** - Confirms `hydrogen_valgrind` executable creation
12. **Verify Release and Naked Executables** - Confirms `hydrogen_release` and `hydrogen_naked` creation
13. **Build Examples** - Compiles OIDC client examples
14. **Verify Examples Executables** - Confirms example executables creation

## Usage

### Run as Part of Full Test Suite

```bash
./test_00_all.sh
```

### Run Individually

```bash
./test_01_compilation.sh
```

### Run Specific Test

```bash
./test_00_all.sh 01_compilation
```

## Build Targets

### Main Executables

- `hydrogen` - Default build configuration
- `hydrogen_debug` - Debug build with debugging symbols
- `hydrogen_perf` - Performance-optimized build
- `hydrogen_valgrind` - Build optimized for Valgrind analysis
- `hydrogen_release` - Release build configuration
- `hydrogen_naked` - Minimal build configuration

### Example Executables

- `examples/C/auth_code_flow` - OAuth2 authorization code flow example
- `examples/C/client_credentials` - OAuth2 client credentials flow example
- `examples/C/password_flow` - OAuth2 password flow example

## Output Format

The script provides detailed output including:

- Subtest progress with clear pass/fail indicators
- File sizes for created executables
- Command execution details
- Comprehensive test completion summary

## Integration

- **Test Suite Integration**: Automatically discovered and run by `test_00_all.sh`
- **Result Tracking**: Exports subtest results for summary reporting
- **Prerequisite Role**: Other tests depend on successful compilation

## Related Documentation

- [LIBRARIES.md](/docs/H/tests/LIBRARIES.md) - Table of Contents for modular library scripts in the 'lib/' directory
- [framework.md](/docs/H/tests/framework.md) - Testing framework overview and architecture
- [file_utils.md](/docs/H/tests/file_utils.md) - File utility functions documentation
- [log_output.md](/docs/H/tests/log_output.md) - Log output formatting and analysis
