# test_99_coverage.md

## Overview

The `test_89_coverage.sh` script provides comprehensive code coverage analysis by running blackbox tests with coverage instrumentation and generating detailed coverage reports.

## Purpose

This test focuses on coverage analysis and reporting by:

- Building Hydrogen with coverage instrumentation (`--coverage` flag)
- Running blackbox integration tests to exercise the application
- Collecting coverage data from both Unity tests and blackbox tests
- Generating comprehensive coverage reports using advanced table formatting
- Analyzing uncovered files and coverage gaps
- Providing combined coverage metrics across test types

## Coverage Analysis Features

### Multi-Source Coverage Collection

- **Unity Test Coverage**: Analyzes coverage from unit tests (Test 11)
- **Blackbox Test Coverage**: Captures coverage from integration/system tests
- **Combined Coverage**: Merges coverage data from both test types
- **Coverage Overlap**: Identifies shared coverage between test types

### Advanced Reporting

- **Tabular Reports**: Uses the `coverage_table.sh` script for professional formatting
- **Visual Highlighting**: Color-coded output with uncovered files highlighted
- **Detailed Metrics**: Line-by-line coverage analysis per source file
- **Summary Statistics**: Overall coverage percentages and file counts

## Test Execution Process

### Coverage Build

1. Builds Hydrogen with coverage instrumentation enabled
2. Creates coverage-specific build directory (`build/coverage`)
3. Ensures gcov data files (.gcno) are generated during compilation

### Blackbox Test Execution

1. Starts Hydrogen server with coverage instrumentation
2. Runs integration tests against the live server
3. Collects runtime coverage data (.gcda files)
4. Properly shuts down server to ensure data is written

### Coverage Data Analysis

1. Processes gcov files from both Unity and blackbox test runs
2. Applies `.trial-ignore` filtering to exclude irrelevant files
3. Calculates coverage percentages with high precision (3 decimal places)
4. Generates detailed coverage reports and summaries

## Coverage Libraries Integration

This test leverages the comprehensive coverage library system:

- **coverage.sh**: Main orchestration and API functions
- **coverage-common.sh**: Shared utilities and file filtering
- **coverage-blackbox.sh**: Blackbox coverage calculation
- **coverage_table.sh**: Advanced tabular reporting with visual formatting

## Output Files

### Coverage Data Files

- `blackbox_coverage.txt` - Blackbox test coverage percentage
- `combined_coverage.txt` - Combined coverage from all test types
- `overlap_coverage.txt` - Coverage overlap analysis
- `*.detailed` files - Detailed metrics with timestamps and line counts

### Coverage Reports

- Formatted table showing per-file coverage metrics
- Summary statistics with overall coverage percentages
- Identification of uncovered files for further testing
- Comparison between Unity and blackbox coverage

## Coverage Metrics

The script provides multiple perspectives on code coverage:

- **Line Coverage**: Percentage of instrumented lines executed
- **File Coverage**: Number of files with any coverage vs. total files
- **Test Type Comparison**: Unity vs. blackbox coverage comparison
- **Coverage Gaps**: Files and functions not covered by any tests

## Expected Outcomes

- **Pass**: Coverage analysis completed successfully, reports generated
- **Fail**: Coverage build failed, server startup issues, or analysis errors
- **Info**: Coverage percentages, uncovered files, and detailed metrics

## Performance Considerations

- **Parallel Processing**: Uses parallel gcov generation when available
- **File Caching**: Caches source files and gcov data to reduce filesystem overhead
- **Efficient Filtering**: Optimized pattern matching for .trial-ignore processing
- **Memory Management**: Temporary file cleanup and controlled memory usage

## Integration with Test Suite

- **Depends on Test 11**: Uses Unity test coverage data as baseline
- **Standalone Execution**: Can run independently to analyze current coverage
- **Report Generation**: Feeds coverage data into overall test reporting
- **Quality Gates**: Can be used to enforce minimum coverage thresholds

## Troubleshooting

Common issues and solutions:

- **No Coverage Data**: Ensure both Unity tests and coverage build completed successfully
- **Missing gcov Files**: Verify coverage instrumentation is enabled in build
- **External Library Pollution**: Check .trial-ignore patterns exclude system files
- **Table Formatting Issues**: Ensure `tables` executable is available and accessible
- **Server Startup Failures**: Check port availability and configuration files

## Usage in CI/CD

This test is particularly valuable in continuous integration:

- **Coverage Tracking**: Monitor coverage trends over time
- **Quality Gates**: Fail builds that drop below coverage thresholds
- **Regression Detection**: Identify when new code lacks test coverage
- **Reporting**: Generate coverage reports for development teams
