# Coverage Libraries Documentation

## Overview

The Hydrogen test suite includes a comprehensive coverage analysis system that tracks code coverage from both Unity tests and blackbox tests. The coverage system is built around a modular architecture with specialized scripts for different aspects of coverage calculation and reporting.

## Coverage Architecture

The coverage system consists of several interconnected components:

- **coverage.sh** - Main coverage orchestration and API functions
- **coverage-common.sh** - Shared utilities, variables, and common functions
- **coverage-unity.sh** - Unity test coverage calculation from gcov data
- **coverage-blackbox.sh** - Blackbox test coverage calculation and combined analysis
- **coverage_table.sh** - Advanced tabular coverage reporting with visual formatting

## Core Coverage Scripts

### coverage.sh - Main Coverage API

**Location**: `tests/lib/coverage.sh`

The main coverage script provides high-level functions for managing and calculating test coverage data across both Unity and blackbox test suites.

**Key Functions**:

- `validate_coverage_consistency()` - Validates coverage data consistency and provides detailed breakdowns
- `get_coverage_summary()` - Returns formatted coverage summary for display

**Dependencies**: Sources all other coverage modules automatically

### coverage-common.sh - Shared Utilities

**Location**: `tests/lib/coverage-common.sh`

Provides shared variables, functions, and utilities used by all coverage calculation modules. This includes file filtering, ignore pattern handling, and result storage management.

**Key Features**:

- Global coverage file locations and paths
- `.trial-ignore` pattern loading and caching
- Source file caching for performance
- Combined coverage analysis functions
- Coverage data cleanup utilities

**Key Functions**:

- `load_ignore_patterns()` - Efficiently loads and caches .trial-ignore patterns
- `should_ignore_file()` - Fast file filtering based on ignore patterns
- `analyze_combined_gcov_coverage()` - Analyzes combined coverage from Unity and blackbox gcov files
- `cleanup_coverage_data()` - Cleans up coverage result files

### coverage-unity.sh - Unity Test Coverage

**Location**: `tests/lib/coverage-unity.sh`

Calculates code coverage from Unity test execution by analyzing gcov files generated during Unity test runs.

**Key Functions**:

- `calculate_unity_coverage()` - Processes gcov files from Unity build directory
- Parallel gcov file generation for improved performance
- Filtering of external libraries and system files
- Line-by-line coverage analysis

**Output**: Stores results in `unity_coverage.txt` and `unity_coverage.txt.detailed`

### coverage-blackbox.sh - Blackbox Test Coverage

**Location**: `tests/lib/coverage-blackbox.sh`

Handles blackbox test coverage calculation, combined coverage analysis, and identification of uncovered files.

**Key Functions**:

- `collect_blackbox_coverage()` - Calculates coverage from blackbox test execution
- `collect_blackbox_coverage_from_dir()` - Alternative coverage collection from specific directory
- `calculate_combined_coverage()` - Combines Unity and blackbox coverage for comprehensive analysis
- `calculate_coverage_overlap()` - Identifies overlap between test types
- `identify_uncovered_files()` - Lists files with no coverage from either test type

**Output**: Stores results in `blackbox_coverage.txt`, `combined_coverage.txt`, and `overlap_coverage.txt`

## Coverage Tables Script (Highlighted)

### coverage_table.sh - Advanced Coverage Reporting

**Location**: `tests/lib/coverage_table.sh`

This is the most sophisticated component of the coverage system, providing detailed visual coverage analysis using the external `tables` executable for professional formatting.

**Special Features**:

#### Comprehensive Data Analysis

- Processes gcov files from both Unity (`build/unity/src`) and Coverage (`build/coverage/src`) build directories
- Applies consistent `.trial-ignore` filtering across all analysis
- Combines coverage data from multiple test types using union logic (maximum coverage achieved)

#### Advanced Table Formatting

- Uses the external `tables` executable for professional table rendering
- Supports ANSI colors and themes for visual distinction
- Rounded corners, color support, and customizable layouts
- Real-time summary statistics in table title

#### Detailed Coverage Metrics

The script provides multiple coverage perspectives:

- **Unity Coverage**: Coverage from unit tests only
- **Blackbox Coverage**: Coverage from integration/system tests only  
- **Combined Coverage**: Union of both test types (maximum lines covered)
- **Per-file Analysis**: Line-by-line breakdown for each source file

#### Intelligent File Organization

- Groups files by directory structure (src/folder/subfolder)
- Sorts files alphabetically within each group
- Highlights uncovered files in yellow for easy identification
- Shows instrumented line counts and coverage percentages

#### Performance Optimizations

- Caches gcov file locations to avoid repeated filesystem scans
- Parallel gcov generation when `xargs` is available
- Efficient awk-based line counting for large files
- Temporary file management with automatic cleanup

**Usage**:

```bash
./coverage_table.sh
```

**Output Format**:
The script generates a formatted table showing:

- Coverage percentage per file
- Instrumented line counts
- Individual Unity and blackbox test coverage
- Combined coverage results
- Folder-based organization with summary statistics

**Table Columns**:

- **Cover %**: Combined coverage percentage for the file
- **Lines**: Total instrumented lines in the file
- **Source File**: File path with highlighting for uncovered files
- **Unity**: Lines covered by Unity tests
- **Tests**: Lines covered by blackbox tests
- **Cover**: Total lines covered (combined)

## Coverage Data Storage

All coverage results are stored in `tests/results/` with specific file formats:

- `unity_coverage.txt` - Unity test coverage percentage
- `blackbox_coverage.txt` - Blackbox test coverage percentage
- `combined_coverage.txt` - Combined coverage percentage
- `overlap_coverage.txt` - Coverage overlap percentage
- `*.detailed` files - Detailed metrics with timestamps and line counts

## Integration with Test Suite

The coverage system integrates with the broader test suite through:

- **Test 11 (Unity)**: Generates gcov data during unit test execution
- **Test 99 (Coverage)**: Runs blackbox tests with coverage instrumentation
- **Test Results**: Coverage data feeds into overall test reporting

## Dependencies

- **gcov**: GNU coverage analysis tool
- **tables**: External executable for advanced table formatting
- **awk**: Text processing for coverage calculations
- **find/xargs**: File system operations and parallel processing

## Best Practices

1. **Run Unity tests first** to generate baseline coverage data
2. **Execute blackbox tests** to capture integration coverage
3. **Use coverage_table.sh** for comprehensive analysis and reporting
4. **Maintain .trial-ignore** file to exclude irrelevant files from coverage analysis
5. **Review uncovered files** regularly to identify gaps in test coverage

## Performance Considerations

The coverage system is optimized for large codebases:

- File caching reduces repeated filesystem scans
- Parallel processing speeds up gcov generation
- Efficient pattern matching for file filtering
- Temporary file management prevents memory issues

## Troubleshooting

Common issues and solutions:

- **No gcov files found**: Ensure tests have been run with coverage instrumentation
- **Missing coverage data**: Check that build directories contain .gcno and .gcda files
- **External library pollution**: Verify .trial-ignore patterns are correctly excluding system files
- **Table formatting issues**: Ensure tables executable is available and accessible
