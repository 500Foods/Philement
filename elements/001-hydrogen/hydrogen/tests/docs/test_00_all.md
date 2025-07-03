# Test 00 All Script Documentation

## Overview

The `test_00_all.sh` script serves as the primary test orchestration tool for the Hydrogen test suite. It executes all test scripts in sequence, ensuring comprehensive validation of the Hydrogen 3D printer control server.

## Purpose

This script is designed to automate the execution of the entire test suite, providing a single entry point for running all tests. It handles test execution, result summarization, and README updates, ensuring a consistent testing process.

## Key Features

- **Library-Based Architecture**: Uses modular library scripts from the `lib/` directory for table rendering, logging, and framework functions
- **Flexible Test Selection**: Can run all tests or specific tests by name
- **Formatted Output**: Provides beautifully formatted table output with test results using the table rendering system
- **Special Test Ordering**: Runs `test_10_compilation.sh` first as a prerequisite, then other tests in order, ending with `test_99_codebase.sh`
- **Dynamic Test Discovery**: Automatically discovers all test scripts and makes them available
- **Comprehensive Summary**: Generates detailed summary tables with individual test results, subtest counts, timing information, and statistics
- **Skip Option**: Can skip actual test execution while showing what would run using the `--skip-tests` flag
- **README Update**: Automatically updates the project `README.md` with test results and code statistics using cloc
- **Help System**: Provides detailed help with available test names and usage examples

## Usage

### Run All Tests

```bash
./test_00_all.sh
```

### Run Specific Tests

```bash
./test_00_all.sh 10_compilation 32_system_endpoints
```

### Skip Test Execution (Show What Would Run)

```bash
./test_00_all.sh --skip-tests
```

### Get Help and Available Tests

```bash
./test_00_all.sh --help
```

## Script Architecture

### Version Information

- **Current Version**: 3.0.1
- **Major Rewrite**: Version 3.0.0 introduced complete library-based architecture

### Library Dependencies

The script sources several library modules:

- `tables_config.sh` - Table configuration
- `tables_data.sh` - Table data management  
- `tables_render.sh` - Table rendering
- `tables_themes.sh` - Table themes and styling
- `log_output.sh` - Log output formatting
- `framework.sh` - Test framework functions
- `cloc.sh` - Code line counting utilities

### Test Execution Flow

1. Parse command line arguments
2. Discover available test scripts
3. Order tests (compilation first, codebase last)
4. Execute tests individually or collectively
5. Capture timing and subtest results
6. Generate formatted summary table
7. Update project README with results

### Result Tracking

The script tracks comprehensive metrics for each test:

- Test number and name
- Total subtests executed
- Passed/failed subtest counts
- Execution timing
- Overall pass/fail status

## Output Format

The script generates a beautifully formatted table showing:

- Test numbers and names
- Subtest counts and pass/fail statistics
- Execution timing for each test
- Summary totals with elapsed vs running time
- Grouped display by test number ranges

## README Integration

The script automatically updates the project README.md with:

- **Latest Test Results** section with summary statistics
- **Individual Test Results** table with detailed per-test information
- **Repository Information** section with cloc-generated code statistics
- Timestamp information for when results were generated

## Related Documentation

- [LIBRARIES.md](LIBRARIES.md) - Table of Contents for modular library scripts in the 'lib/' directory
- [framework.md](framework.md) - Testing framework overview and architecture
- [tables.md](tables.md) - Table system overview documentation
