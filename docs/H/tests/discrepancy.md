# Discrepancy Analysis Library Documentation

## Overview

The Discrepancy Analysis Library (`discrepancy.sh`) provides advanced coverage analysis functionality to identify and analyze mismatches between different coverage calculation methods used in the Hydrogen test suite. This tool is essential for maintaining accurate coverage metrics and tuning discrepancy adjustments.

## Purpose

This library compares coverage line counts from two different calculation methods:

1. **Test 89 method** (calculate_unity_coverage/calculate_blackbox_coverage): Processes gcov files with specific filtering and discrepancy adjustments
2. **coverage_table.sh method** (analyze_all_gcov_coverage_batch): Processes gcov files for coverage table display

By comparing these methods, developers can identify where counting differences occur and tune the `DISCREPANCY_UNITY` and `DISCREPANCY_COVERAGE` values in [`coverage.sh`](/elements/001-hydrogen/hydrogen/tests/lib/coverage.sh).

## Key Features

- Side-by-side comparison of coverage counts per source file
- Global concatenation method analysis (what Test 89 actually uses)
- Per-file sum method comparison
- Actual stored coverage value verification
- Filtering difference detection
- Comprehensive summary totals
- Suggested discrepancy adjustments
- Optional Unity-only or Blackbox-only analysis
- Summary-only mode for quick overview

## Usage

### Basic Usage

```bash
# Run full analysis
./discrepancy.sh

# Show only Unity test discrepancies
./discrepancy.sh --unity-only

# Show only Blackbox test discrepancies
./discrepancy.sh --blackbox-only

# Show summary totals only (skip per-file details)
./discrepancy.sh --summary
```

### Command Line Options

- `-h, --help` - Display help information
- `-v, --version` - Show version information  
- `--unity-only` - Show only Unity test discrepancies
- `--blackbox-only` - Show only Blackbox test discrepancies
- `--summary` - Show summary totals only, skip per-file details

## Script Architecture

### Version Information

- **Current Version**: 2.0.0
- **Major Update**: Version 2.0.0 added GLOBAL concat method analysis and stored file comparison

### Environment Requirements

- `HYDROGEN_ROOT` - Path to Hydrogen project root directory
- `HELIUM_ROOT` - Path to Helium project root directory

### Library Dependencies

The script sources the following libraries:

- [`framework.sh`](/elements/001-hydrogen/hydrogen/tests/lib/framework.sh) - Test framework functions

## Analysis Methods

### Method 1: Test 89 Style

Mimics `calculate_coverage_generic` from [`coverage.sh`](/elements/001-hydrogen/hydrogen/tests/lib/coverage.sh):

- Concatenates ALL gcov files into a single file
- Counts lines globally (handles duplicates through concatenation)
- Applies same filtering as Test 89:
  - Skips unity.c, system libraries, external dependencies
  - Filters based on `.trial-ignore` patterns
  - Applies extra filters for Unity tests

### Method 2: coverage_table.sh Style  

Uses same batch processing as [`coverage_table.sh`](/elements/001-hydrogen/hydrogen/tests/lib/coverage_table.sh):

- Processes each gcov file individually
- Sums results per file
- Uses union of files from both Unity and Blackbox directories
- Applies similar ignore patterns

## Output Format

### Per-File Comparison Table

```issues
Source File                                  │ M1-Unity │ M2-Unity │ Diff     │ M1-BB    │ M2-BB
─────────────────────────────────────────────┼──────────┼──────────┼──────────┼──────────┼──────────
src/utils/utils_crypto.c                     │      156 │      156 │        0 │       89 │       89
```

### Summary Totals

Shows:

- Unity Coverage (per-file sum method)
- Unity Coverage (GLOBAL concat method - actual Test 89 behavior)
- Blackbox Coverage (per-file sum method)
- Blackbox Coverage (GLOBAL concat method - actual Test 89 behavior)
- Files with differences count
- Concat vs Sum differences

### Actual Stored Values

Reads and compares against stored coverage files:

- `coverage_unity.txt.detailed`
- `coverage_blackbox.txt.detailed`

### Suggested Adjustments

Provides recommended `DISCREPANCY_UNITY` and `DISCREPANCY_COVERAGE` adjustment values based on instrumented line differences.

### Filtering Differences

Lists files that only appear in one method (potential filtering issues).

## Key Differences Between Methods

The critical difference is in how duplicate lines are handled:

- **Method 1 (Test 89)**: Concatenates all gcov files first, then counts. Duplicate lines across files are naturally deduplicated through the concatenation process.

- **Method 2 (coverage_table)**: Counts each file separately, then sums. Duplicate lines across files are counted multiple times.

This explains why Method 1 often shows lower counts - it's more accurate for shared code that appears in multiple gcov files.

## Integration with Coverage System

The discrepancy analysis helps maintain the accuracy of:

- [`coverage.sh`](/elements/001-hydrogen/hydrogen/tests/lib/coverage.sh) - Main coverage orchestration
- [`coverage_table.sh`](/elements/001-hydrogen/hydrogen/tests/lib/coverage_table.sh) - Coverage table generation
- Test 89 (Coverage Analysis) - Uses results to tune adjustments

## Best Practices

1. **Run After Major Changes**: Run discrepancy analysis after adding/removing tests or changing coverage filters
2. **Monitor Discrepancy Values**: Keep DISCREPANCY_* values in [`coverage.sh`](/elements/001-hydrogen/hydrogen/tests/lib/coverage.sh) updated based on analysis
3. **Investigate Large Differences**: Files with large differences may indicate filtering issues
4. **Use Summary Mode First**: Start with `--summary` to get quick overview before diving into per-file details
5. **Check Stored Values**: Always verify against actual stored coverage values

## Troubleshooting

### High Discrepancy Counts

If many files show discrepancies:

- Check `.trial-ignore` patterns  
- Verify filtering logic matches between methods
- Look for files only in one method (filtering difference)

### GLOBAL vs Sum Differences

Differences between GLOBAL concat and per-file sum indicate:

- Shared code appearing in multiple gcov files
- This is normal and expected
- Method 1 (GLOBAL) is more accurate for final coverage numbers

### Stored Values Mismatch

If stored values don't match Script GLOBAL:

- Re-run Test 89 to regenerate stored files
- Check for recent changes to coverage calculation logic
- Verify gcov files haven't been regenerated since Test 89

## Related Documentation

- [Coverage Analysis System](/docs/H/tests/coverage.md) - Main coverage documentation
- [Coverage Table Library](/docs/H/tests/coverage_table.md) - Table generation
- [CLOC Library](/docs/H/tests/cloc.md) - Code line counting
- [LIBRARIES.md](/docs/H/tests/LIBRARIES.md) - All test libraries
- [Test 89 Documentation](/docs/H/tests/test_89_coverage.md) - Coverage analysis test

## Version History

- **2.0.0** (2026-01-13): Added GLOBAL concat method analysis and stored file comparison
- **1.0.0** (2026-01-12): Initial version for identifying coverage discrepancies
