# CLOC (Count Lines of Code) Library Documentation

## Overview

The CLOC Library (`cloc.sh`) provides shared functionality for counting lines of code across the Hydrogen test suite. This library standardizes code analysis operations and integrates with the project's linting exclusion patterns to provide accurate code metrics.

## Library Information

- **Script**: `../lib/cloc.sh`
- **Version**: 1.0.0
- **Created**: 2025-07-02
- **Purpose**: Extracted from `test_99_codebase.sh` and `test_00_all.sh` to provide modular CLOC functionality

## Key Features

- Integration with `.lintignore` files for consistent exclusion patterns
- Support for default exclusion patterns (build directories, logs, etc.)
- Multiple output formats (stdout, file, README.md format)
- Statistics extraction capabilities
- Proper environment handling for UTF-8 support

## Functions

### Core Functions

#### `print_cloc_lib_version()`

Displays the library name and version information.

**Usage:**

```bash
print_cloc_lib_version
```

#### `should_exclude_file(file, [lint_ignore_file])`

Checks if a file should be excluded from CLOC analysis based on exclusion patterns.

**Parameters:**

- `file`: Path to the file to check
- `lint_ignore_file`: Optional path to lintignore file (default: `.lintignore`)

**Returns:**

- `0`: File should be excluded
- `1`: File should be included

**Usage:**

```bash
if should_exclude_file "src/main.c" ".lintignore"; then
    echo "File excluded from analysis"
fi
```

#### `generate_cloc_exclude_list(base_dir, lint_ignore_file, exclude_list_file)`

Generates a CLOC-compatible exclude list file based on lintignore patterns and defaults.

**Parameters:**

- `base_dir`: Base directory to scan (default: current directory)
- `lint_ignore_file`: Path to lintignore file (default: `.lintignore`)
- `exclude_list_file`: Output file for exclude list (required)

**Usage:**

```bash
generate_cloc_exclude_list "." ".lintignore" "/tmp/excludes.txt"
```

### Analysis Functions

#### `run_cloc_analysis(base_dir, lint_ignore_file, [output_file])`

Runs CLOC analysis with proper exclusions and environment settings.

**Parameters:**

- `base_dir`: Base directory to analyze (default: current directory)
- `lint_ignore_file`: Path to lintignore file (default: `.lintignore`)
- `output_file`: Optional output file (if not provided, outputs to stdout)

**Returns:**

- `0`: Success
- `1`: Failure (cloc not available or command failed)

**Usage:**

```bash
# Output to stdout
run_cloc_analysis "." ".lintignore"

# Output to file
run_cloc_analysis "." ".lintignore" "cloc_results.txt"
```

#### `generate_cloc_for_readme(base_dir, lint_ignore_file)`

Generates CLOC output formatted for inclusion in README.md files with proper code block formatting.

**Parameters:**

- `base_dir`: Base directory to analyze (default: current directory)
- `lint_ignore_file`: Path to lintignore file (default: `.lintignore`)

**Usage:**

```bash
generate_cloc_for_readme "." ".lintignore" >> README.md
```

### Statistics Functions

#### `extract_cloc_stats(cloc_output_file)`

Extracts summary statistics from CLOC output file.

**Parameters:**

- `cloc_output_file`: Path to file containing CLOC output

**Returns:**

- Success: Outputs "files:N,lines:N" format
- Failure: Error message to stderr

**Usage:**

```bash
stats=$(extract_cloc_stats "cloc_output.txt")
echo "Statistics: $stats"
```

#### `run_cloc_with_stats(base_dir, lint_ignore_file)`

Runs CLOC analysis and returns summary statistics in a compact format.

**Parameters:**

- `base_dir`: Base directory to analyze (default: current directory)
- `lint_ignore_file`: Path to lintignore file (default: `.lintignore`)

**Returns:**

- Success: Outputs "files:N,lines:N" format
- Failure: Returns non-zero exit code

**Usage:**

```bash
stats=$(run_cloc_with_stats "." ".lintignore")
if [ $? -eq 0 ]; then
    echo "Code statistics: $stats"
fi
```

## Default Exclusion Patterns

The library includes default exclusion patterns for common build and temporary directories:

- `build/*`
- `build_debug/*`
- `build_perf/*`
- `build_release/*`
- `build_valgrind/*`
- `tests/logs/*`
- `tests/results/*`
- `tests/diagnostics/*`

## Integration with Lintignore

The library reads `.lintignore` files to determine which files and directories should be excluded from analysis. This ensures consistency between linting operations and code counting.

### Lintignore Format

- One pattern per line
- Lines starting with `#` are treated as comments
- Patterns ending with `/*` are treated as directory patterns
- Supports both exact file matches and directory-based exclusions

## Environment Requirements

- **cloc**: The `cloc` command-line tool must be installed and available in PATH
- **UTF-8 Support**: The library sets `LC_ALL=en_US.UTF_8` for proper character encoding

## Error Handling

The library includes comprehensive error handling:

- Checks for `cloc` availability before execution
- Validates required parameters
- Uses temporary files with proper cleanup
- Returns appropriate exit codes for success/failure

## Usage Examples

### Basic Code Analysis

```bash
source lib/cloc.sh

# Simple analysis with output to stdout
run_cloc_analysis

# Analysis with custom lintignore file
run_cloc_analysis "src/" "custom.lintignore"
```

### Statistics Collection

```bash
source lib/cloc.sh

# Get compact statistics
stats=$(run_cloc_with_stats)
echo "Project statistics: $stats"

# Extract specific values
files=$(echo "$stats" | cut -d',' -f1 | cut -d':' -f2)
lines=$(echo "$stats" | cut -d',' -f2 | cut -d':' -f2)
echo "Files: $files, Lines of Code: $lines"
```

### README Generation

```bash
source lib/cloc.sh

# Add CLOC output to README
echo "## Code Statistics" >> README.md
generate_cloc_for_readme >> README.md
```

## Related Documentation

- [Test Framework Library](framework.md) - Main test framework that may use CLOC functionality
- [File Utils Library](file_utils.md) - File manipulation utilities
- [LIBRARIES.md](LIBRARIES.md) - Complete library documentation index
