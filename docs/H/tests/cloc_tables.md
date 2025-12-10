# CLOC Tables Script Documentation

## Overview

The `cloc_tables.sh` script provides the CLOC table generation functionality used by the metrics system. This script generates the two CLOC tables (main code table and extended statistics) that appear in the daily metrics files.

## Library Information

- **Script**: `../lib/cloc_tables.sh`
- **Version**: 1.1.0
- **Created**: 2025-09-23
- **Purpose**: Generate CLOC tables for metrics reporting without test framework overhead

## Key Features

- Generates both main CLOC table and extended statistics table
- Integrates with the project's linting exclusion patterns
- Uses the same CLOC analysis as the main test suite
- Optimized for metrics generation (no test framework dependencies)
- Proper error handling and UTF-8 support

## Functions

### Main Function

#### `run_cloc_analysis_for_tables()`

The primary function that generates both CLOC tables for metrics reporting.

**Process:**

1. Sets up temporary files for CLOC analysis
2. Runs CLOC analysis with proper exclusions
3. Outputs the main code table
4. Outputs the extended statistics table (if available)
5. Cleans up temporary files

**Usage:**

```bash
source lib/cloc_tables.sh
run_cloc_analysis_for_tables
```

**Output:**

- Main CLOC table showing lines of code by language
- Extended statistics table with additional metrics
- Both tables are formatted for terminal display with ANSI colors

## Integration with Metrics

This script is called by `test_00_all.sh` to generate the CLOC tables that appear in the daily metrics files:

```bash
# From test_00_all.sh
cloc_output=$(env -i bash "${SCRIPT_DIR}/lib/cloc_tables.sh")
```

The output is appended to the metrics file alongside the test results and coverage tables.

## Dependencies

- **cloc**: The CLOC command-line tool must be available
- **lintignore files**: Uses `.lintignore` for consistent exclusion patterns
- **UTF-8 environment**: Requires proper UTF-8 locale settings

## Error Handling

- Returns exit code 1 if CLOC analysis fails
- Outputs error message to stderr for debugging
- Continues execution even if CLOC tables cannot be generated

## Related Documentation

- [CLOC Library](/docs/H/tests/cloc.md) - Core CLOC functionality and API
- [Test 00 All](/docs/H/tests/test_00_all.md) - How metrics files are generated
- [Build Metrics](/docs/H/metrics/README.md) - Overview of the metrics system