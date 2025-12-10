# Coverage Combined Library Documentation

## Overview

The `coverage-combined.sh` library provides functions for calculating combined coverage analysis and identifying uncovered files. It handles the complex logic of merging coverage data from Unity and blackbox tests to provide comprehensive coverage metrics.

## Library Information

- **Script**: `../lib/coverage-combined.sh`
- **Version**: 1.1.0
- **Created**: 2025-07-21
- **Purpose**: Combined coverage analysis and uncovered file identification

## Key Features

- **Combined Coverage Calculation**: Merges Unity and blackbox coverage data
- **Overlap Analysis**: Identifies coverage overlap between test types
- **Uncovered File Detection**: Lists source files with no test coverage
- **Caching Support**: Avoids redundant calculations
- **Batch Processing**: Efficient analysis of multiple gcov files

## Functions

### Core Functions

#### `calculate_combined_coverage()`

Calculates the combined coverage percentage from both Unity and blackbox test results.

**Process:**

1. Retrieves Unity and blackbox coverage data
2. Performs batch analysis of all gcov files
3. Merges coverage data using union logic (maximum coverage)
4. Calculates overall combined percentage
5. Caches results for performance

**Returns:** Combined coverage percentage (e.g., "85.432")

**Usage:**

```bash
source lib/coverage-combined.sh
combined_pct=$(calculate_combined_coverage)
echo "Combined coverage: ${combined_pct}%"
```

#### `calculate_coverage_overlap()`

Calculates the overlap percentage between Unity and blackbox test coverage.

**Process:**

1. Reads individual coverage percentages from cache files
2. Determines minimum coverage as overlap indicator
3. Stores overlap result for reporting

**Returns:** Overlap percentage representing lines covered by both test types

**Usage:**

```bash
overlap=$(calculate_coverage_overlap)
echo "Coverage overlap: ${overlap}%"
```

#### `identify_uncovered_files(project_root)`

Identifies source files that have no test coverage from either test type.

**Parameters:**

- `project_root`: Root directory of the project

**Process:**

1. Loads all source files using cached data
2. Checks each file for corresponding gcov coverage
3. Categorizes files as covered or uncovered
4. Returns structured analysis

**Output Format:**

```log
COVERED_FILES_COUNT:42
UNCOVERED_FILES_COUNT:8
TOTAL_SOURCE_FILES:50
UNCOVERED_FILES:
/path/to/uncovered1.c
/path/to/uncovered2.c
```

**Usage:**

```bash
analysis=$(identify_uncovered_files "/path/to/project")
echo "$analysis" | grep "UNCOVERED_FILES_COUNT"
```

## Data Structures

The library uses global arrays populated by batch analysis functions:

- `combined_instrumented_lines[]`: Maps file paths to instrumented line counts
- `combined_covered_lines[]`: Maps file paths to covered line counts

## Integration with Coverage System

This library is a key component of the coverage analysis pipeline:

1. **Coverage Table Generation**: Provides combined data for `coverage_table.sh`
2. **Metrics Reporting**: Supplies combined percentages for daily metrics
3. **Quality Analysis**: Enables identification of coverage gaps

## Dependencies

- **coverage-common.sh**: Shared utilities and data structures
- **gcov files**: Generated during test execution
- **AWK**: For percentage calculations
- **find/grep**: For file system operations

## Performance Optimizations

- **Result Caching**: Avoids recalculating combined coverage
- **Batch Processing**: Analyzes multiple gcov files efficiently
- **File Caching**: Uses pre-loaded source file lists
- **Marker Files**: Prevents redundant computations

## Error Handling

- Gracefully handles missing coverage data
- Returns default values (0.000) for failed calculations
- Continues execution even with incomplete data
- Provides structured error output

## Usage Examples

### Combined Coverage Analysis

```bash
source lib/coverage-combined.sh

# Get combined coverage percentage
combined=$(calculate_combined_coverage)
echo "Total coverage across all tests: ${combined}%"

# Check coverage overlap
overlap=$(calculate_coverage_overlap)
echo "Lines covered by both test types: ${overlap}%"
```

### Coverage Gap Analysis

```bash
# Identify files needing test coverage
project_root="$(cd ../.. && pwd)"
analysis=$(identify_uncovered_files "$project_root")

# Extract uncovered file count
uncovered_count=$(echo "$analysis" | grep "UNCOVERED_FILES_COUNT" | cut -d: -f2)
echo "Files needing coverage: $uncovered_count"

# List uncovered files
echo "$analysis" | sed -n '/UNCOVERED_FILES:/,/^$/p' | tail -n +2
```

## Related Documentation

- [Coverage Libraries](/docs/H/tests/coverage.md) - Complete coverage system overview
- [Coverage Common](/docs/H/tests/coverage-common.md) - Shared coverage utilities
- [Coverage Table](/docs/H/tests/coverage_table.md) - Table generation for metrics
- [Build Metrics](/docs/H/metrics/README.md) - How combined coverage appears in metrics