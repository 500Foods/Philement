# Coverage Common Library Documentation

## Overview

The `coverage-common.sh` library provides shared utilities, variables, and functions used by all coverage analysis components in the Hydrogen test suite. It handles the core logic for processing gcov files, managing ignore patterns, and caching source file data.

## Library Information

- **Script**: `../lib/coverage-common.sh`
- **Version**: 2.1.0
- **Created**: 2025-07-21
- **Purpose**: Shared utilities for coverage analysis across all coverage scripts

## Key Features

- **Global Coverage Storage**: Manages result files for Unity, blackbox, combined, and overlap coverage
- **Batch Processing**: Efficiently analyzes all gcov files in a single operation
- **Ignore Pattern Management**: Loads and caches `.trial-ignore` patterns for consistent filtering
- **Source File Caching**: Pre-loads source file lists to avoid repeated filesystem scans
- **Union Coverage Calculation**: Handles complex logic for combining Unity and blackbox coverage
- **Performance Optimizations**: Caching and batch processing for large codebases

## Global Variables

### Coverage Result Files

- `UNITY_COVERAGE_FILE`: Stores Unity test coverage percentage
- `BLACKBOX_COVERAGE_FILE`: Stores blackbox test coverage percentage
- `COMBINED_COVERAGE_FILE`: Stores combined coverage percentage
- `OVERLAP_COVERAGE_FILE`: Stores coverage overlap percentage

### Caching Arrays

- `IGNORE_PATTERNS[]`: Cached ignore patterns from `.trial-ignore`
- `SOURCE_FILES_CACHE[]`: Cached list of source files to analyze

## Core Functions

### Batch Analysis Functions

#### `analyze_combined_gcov_coverage(unity_gcov, blackbox_gcov)`

Analyzes coverage from two gcov files for the same source file and calculates true union coverage.

**Parameters:**

- `unity_gcov`: Path to Unity test gcov file
- `blackbox_gcov`: Path to blackbox test gcov file

**Returns:** "instrumented_lines,covered_lines,covered_lines" format

**Process:**

1. Parses both gcov files simultaneously
2. Tracks instrumented lines from both files
3. Calculates union of covered lines (maximum coverage achieved)
4. Returns combined statistics

#### `analyze_all_gcov_coverage_batch(unity_dir, blackbox_dir)`

Performs comprehensive batch analysis of all gcov files from both test types.

**Parameters:**

- `unity_dir`: Directory containing Unity test gcov files
- `blackbox_dir`: Directory containing blackbox test gcov files

**Sets Global Arrays:**

- `unity_covered_lines[]`: Coverage data per file for Unity tests
- `unity_instrumented_lines[]`: Instrumented lines per file for Unity tests
- `coverage_covered_lines[]`: Coverage data per file for blackbox tests
- `coverage_instrumented_lines[]`: Instrumented lines per file for blackbox tests
- `combined_covered_lines[]`: Combined coverage data per file
- `combined_instrumented_lines[]`: Combined instrumented lines per file

**Features:**

- Caches results to avoid redundant processing
- Handles ignore pattern changes automatically
- Processes files in parallel for performance
- Filters out test files and external libraries

### Pattern and File Management

#### `load_ignore_patterns(project_root)`

Loads and caches ignore patterns from `.trial-ignore` file.

**Parameters:**

- `project_root`: Root directory of the project

**Process:**

1. Loads hardcoded ignore patterns (mock_, jansson, microhttpd, etc.)
2. Reads `.trial-ignore` file and adds custom patterns
3. Caches patterns for subsequent calls

#### `should_ignore_file(file_path, project_root)`

Checks if a file should be excluded from coverage analysis.

**Parameters:**

- `file_path`: Relative path to check
- `project_root`: Project root directory

**Returns:**

- `0`: File should be ignored
- `1`: File should be included

#### `load_source_files(project_root)`

Loads and caches all source files (.c and .h) from the project.

**Parameters:**

- `project_root`: Root directory of the project

**Process:**

1. Scans `src/` directory for C/C++ files
2. Applies ignore pattern filtering
3. Excludes test files (*_test*)
4. Caches results for performance

### File Processing Functions

#### `analyze_gcov_file(gcov_file, coverage_type)`

Analyzes a single gcov file and extracts coverage statistics.

**Parameters:**

- `gcov_file`: Path to gcov file to analyze
- `coverage_type`: "unity" or "coverage" (blackbox)

**Process:**

1. Parses gcov format using AWK
2. Counts instrumented and covered lines
3. Extracts source file path from gcov metadata
4. Stores results in appropriate global arrays

#### `collect_gcov_files(build_dir, coverage_type)`

Collects and processes all gcov files from a build directory.

**Parameters:**

- `build_dir`: Directory containing gcov files
- `coverage_type`: "unity" or "coverage"

**Process:**

1. Finds all .gcov files in directory
2. Applies comprehensive filtering (system libraries, test files, ignored files)
3. Calls `analyze_gcov_file` for each valid file
4. Returns count of files processed

## Performance Optimizations

### Caching Strategy

- **Pattern Caching**: Ignore patterns loaded once and reused
- **Source File Caching**: Source file lists cached to avoid repeated scans
- **Result Caching**: Batch analysis results cached with checksum-based invalidation
- **Marker Files**: Used to track cache validity and avoid redundant work

### Batch Processing

- **Single AWK Pass**: Processes multiple files in one AWK execution
- **Memory-Based Union**: Calculates coverage unions in memory for speed
- **Parallel File Reading**: Uses efficient file reading patterns
- **Temporary File Management**: Proper cleanup of intermediate files

## Integration with Coverage System

This library serves as the foundation for all coverage analysis:

1. **Coverage Table Generation**: Provides data for `coverage_table.sh`
2. **Combined Analysis**: Powers `coverage-combined.sh` functions
3. **Individual Coverage**: Supports `coverage.sh` and `coverage-unity.sh`/`coverage-blackbox.sh`
4. **Metrics Integration**: Supplies data for daily metrics generation

## Dependencies

- **AWK**: For gcov file parsing and statistics calculation
- **find/grep**: For file system operations and filtering
- **sha256sum**: For cache invalidation checksums
- **mktemp**: For temporary file creation

## Error Handling

- Graceful handling of missing or corrupted gcov files
- Default values (0) for failed parsing operations
- Continues processing even with individual file errors
- Comprehensive logging for debugging

## Usage Examples

### Basic Batch Analysis

```bash
source lib/coverage-common.sh

# Analyze all coverage data
analyze_all_gcov_coverage_batch "build/unity" "build/coverage"

# Access results from global arrays
for file in "${!combined_covered_lines[@]}"; do
    echo "$file: ${combined_covered_lines[$file]} lines covered"
done
```

### File Filtering

```bash
source lib/coverage-common.sh

# Check if file should be ignored
if should_ignore_file "src/main.c" "/project/root"; then
    echo "File ignored"
else
    echo "File included in analysis"
fi
```

## Related Documentation

- [Coverage Libraries](coverage.md) - Complete coverage system overview
- [Coverage Combined](coverage-combined.md) - Combined coverage analysis
- [Coverage Table](coverage_table.md) - Table generation for metrics
- [Build Metrics](/docs/H/metrics/README.md) - How coverage data appears in metrics