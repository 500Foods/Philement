# Coverage Table Script Documentation

## Overview

The `coverage_table.sh` script is the most sophisticated component of the Hydrogen coverage analysis system. It provides advanced tabular coverage reporting using the external `tables` executable for professional formatting and appears as the coverage table in daily metrics files.

## Library Information

- **Script**: `../lib/coverage_table.sh`
- **Version**: Part of coverage system v1.0.0
- **Created**: 2025-07-02
- **Purpose**: Generate professional coverage tables for metrics reporting

## Key Features

- **Professional Table Formatting**: Uses external `tables` executable for ANSI-colored, rounded-corner tables
- **Comprehensive Data Analysis**: Processes gcov files from both Unity and blackbox test builds
- **Intelligent File Organization**: Groups files by directory structure with alphabetical sorting
- **Real-time Statistics**: Shows coverage percentages in table headers
- **Visual Indicators**: Highlights uncovered files in yellow for easy identification
- **Performance Optimized**: Caches file locations and uses parallel processing where available

## Core Functionality

### Main Function

#### `generate_coverage_table()`

The primary function that creates the coverage table for metrics reporting.

**Process:**

1. Scans for gcov files in Unity and blackbox build directories
2. Applies `.trial-ignore` filtering for consistent exclusions
3. Calculates coverage percentages for each source file
4. Organizes files by directory structure
5. Generates professional table using external `tables` command
6. Returns formatted table suitable for terminal display

**Usage:**

```bash
source lib/coverage_table.sh
generate_coverage_table
```

### Data Processing

The script processes coverage data from multiple sources:

- **Unity Coverage**: From `build/unity/src/` directory gcov files
- **Blackbox Coverage**: From `build/coverage/src/` directory gcov files
- **Combined Coverage**: Union of both test types (maximum coverage achieved)
- **File Filtering**: Uses `.trial-ignore` patterns to exclude irrelevant files

### Table Structure

The generated table includes:

- **Cover %**: Combined coverage percentage for each file
- **Lines**: Total instrumented lines in the file
- **Source File**: File path with directory grouping and highlighting
- **Unity**: Lines covered by Unity tests
- **Tests**: Lines covered by blackbox tests
- **Cover**: Total lines covered (combined)

## Integration with Metrics

Called by `test_00_all.sh` to generate the coverage table in daily metrics:

```bash
# From test_00_all.sh
coverage_table_file="${RESULTS_DIR}/coverage_table.txt"
/usr/bin/env -i bash "${COVERAGE}" | tee "${coverage_table_file}"
```

The coverage table appears as the third table in the metrics file, providing comprehensive coverage analysis.

## Dependencies

- **tables**: External executable for professional table rendering
- **gcov**: GNU coverage analysis tool
- **find/xargs**: File system scanning and parallel processing
- **awk**: Text processing for coverage calculations
- **.trial-ignore file**: Coverage exclusion patterns

## Performance Features

- **File Caching**: Avoids repeated filesystem scans
- **Parallel Processing**: Uses `xargs` for concurrent gcov generation when available
- **Efficient Filtering**: Fast pattern matching for file exclusions
- **Temporary File Management**: Automatic cleanup of intermediate files

## Error Handling

- Continues execution even if coverage data is incomplete
- Gracefully handles missing gcov files
- Provides fallback behavior for table generation failures
- Returns appropriate exit codes for integration

## Usage Examples

### Direct Execution

```bash
./tests/lib/coverage_table.sh
```

### Integration in Scripts

```bash
source tests/lib/coverage_table.sh
table_output=$(generate_coverage_table)
echo "$table_output" >> metrics.txt
```

## Related Documentation

- [Coverage Libraries](/docs/H/tests/coverage.md) - Complete coverage system overview
- [Test 00 All](/docs/H/tests/test_00_all.md) - Metrics file generation
- [Build Metrics](/docs/H/metrics/README.md) - Metrics system overview