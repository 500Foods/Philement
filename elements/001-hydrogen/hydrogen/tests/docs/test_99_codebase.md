# Test 99: Codebase Analysis Script Documentation

## Overview

The `test_99_codebase.sh` script is a comprehensive static analysis tool within the Hydrogen test suite, focused on evaluating the codebase's structure, quality, and compliance through extensive metrics, linting processes, and code analysis across multiple languages and file types.

## Purpose

This script performs an in-depth analysis of the Hydrogen codebase, including build system cleaning, source code line counting, large file detection, multi-language linting validation, and comprehensive code metrics. It is essential for maintaining code quality, identifying potential issues, ensuring adherence to coding standards, and providing detailed insights into codebase health across the entire project.

## Script Details

- **Script Name**: `test_99_codebase.sh`
- **Test Name**: Static Codebase Analysis
- **Version**: 3.0.0 (Migrated to use lib/ scripts following established patterns)
- **Dependencies**: Uses modular libraries from `lib/` directory

## Key Features

### Core Functionality

- **CMake Build Cleaning**: Comprehensive build artifact cleanup using CMake cleanish target
- **Source Code Analysis**: Detailed line counting and file size categorization
- **Large File Detection**: Identifies oversized files that may impact repository performance
- **Multi-Language Linting**: Validates code quality across C/C++, Markdown, shell scripts, and JSON
- **Code Metrics Analysis**: Uses cloc for comprehensive code line counting and statistics

### Advanced Analysis Capabilities

- **File Size Distribution**: Categorizes files by line count ranges (0-99, 100-199, etc.)
- **Top File Analysis**: Identifies largest files by type (Markdown, C, Header, Shell)
- **Exclusion Pattern Support**: Respects .lintignore files for targeted analysis
- **Build Artifact Management**: Removes object files and other build artifacts
- **Comprehensive Reporting**: Generates detailed analysis reports with timestamps

### Linting Integration

- **cppcheck**: C/C++ static analysis with configurable rules
- **markdownlint**: Markdown formatting and style validation
- **shellcheck**: Shell script analysis with cross-referencing support
- **jq**: JSON syntax validation with intentional error handling
- **Custom Filtering**: Excludes expected warnings and noise from output

## Technical Implementation

### Library Dependencies

- `log_output.sh` - Formatted output and logging
- `file_utils.sh` - File operations and path management
- `framework.sh` - Test framework and result tracking
- `cloc.sh` - Code line counting and analysis

### Configuration Constants

- **MAX_SOURCE_LINES**: 1000 (threshold for oversized files)
- **LARGE_FILE_THRESHOLD**: 25k (size threshold for large files)
- **LINT_OUTPUT_LIMIT**: 100 (maximum lines of linting output)

### Exclusion Pattern System

- **Primary**: `.lintignore` - Main exclusion patterns
- **Language-Specific**: `.lintignore-c`, `.lintignore-markdown`, `.lintignore-bash`
- **Default Excludes**: Build directories, logs, results, diagnostics
- **Dynamic Filtering**: Runtime exclusion checking for each file

### Test Sequence (10 Subtests)

1. **CMake Clean Build Artifacts**: Removes build artifacts using CMake cleanish target
2. **Source Code File Analysis**: Analyzes file distribution and identifies oversized files
3. **Large Non-Source File Detection**: Finds large files that may impact repository
4. **C/C++ Code Linting**: Runs cppcheck with configurable rules and exclusions
5. **Markdown Linting**: Validates markdown files with markdownlint
6. **Shell Script Linting**: Analyzes shell scripts with shellcheck cross-referencing
7. **JSON Linting**: Validates JSON syntax with jq, handling intentional errors
8. **Code Line Count Analysis**: Comprehensive code metrics using cloc
9. **File Count Summary**: Provides distribution summary by file type
10. **Save Analysis Results**: Preserves detailed analysis results for review

## Usage

To run this test as part of the full suite:

```bash
./test_00_all.sh all
```

To run this test individually:

```bash
./test_99_codebase.sh
```

## Expected Results

### Successful Test Criteria

- Build artifacts cleaned successfully
- No source files exceed 1000 lines
- Large files identified and documented
- Linting tools find no unexpected issues
- Code metrics generated successfully
- Analysis results saved for review

### Analysis Outputs

- **File Distribution**: Categorized by line count ranges
- **Top Files**: Largest files by type (Markdown, C, Header, Shell)
- **Large Files**: Non-source files exceeding size threshold
- **Linting Results**: Issues found by each linting tool
- **Code Metrics**: Comprehensive statistics from cloc analysis

## Troubleshooting

### Common Issues

- **Missing Linting Tools**: Install cppcheck, markdownlint, shellcheck, jq as needed
- **Build Directory Issues**: Ensure CMake build directory exists and is accessible
- **Exclusion Pattern Problems**: Check .lintignore files for correct patterns
- **Large File Warnings**: Review identified large files for optimization opportunities
- **Linting Failures**: Review specific linting output for code quality issues

### Diagnostic Information

- **Results Directory**: `tests/results/`
- **Line Count Analysis**: Detailed file-by-file line counts
- **Large File Reports**: Complete list of oversized files
- **Linting Output**: Filtered and formatted linting results
- **Timestamped Results**: All analysis results include timestamp for tracking

## Linting Tool Integration

### cppcheck (C/C++ Analysis)

- **Configuration**: Uses `.lintignore-c` for custom rules
- **Features**: Static analysis, null pointer detection, code quality checks
- **Expected Issues**: Handles known false positives (e.g., intentional null pointer tests)
- **Parallel Processing**: Uses 24 threads for faster analysis

### markdownlint (Markdown Validation)

- **Configuration**: Uses `.lintignore-markdown` for style rules
- **Filtering**: Removes Node.js deprecation warnings from output
- **Standards**: Enforces consistent markdown formatting
- **Output Limiting**: Truncates excessive output for readability

### shellcheck (Shell Script Analysis)

- **Cross-Referencing**: Uses `-x` flag for sourced script analysis
- **Path Resolution**: Handles test scripts from tests directory context
- **Output Formatting**: Provides clean relative paths in results
- **Comprehensive Coverage**: Analyzes all shell scripts in project

### jq (JSON Validation)

- **Syntax Checking**: Validates JSON syntax across all configuration files
- **Intentional Errors**: Handles test files with deliberate JSON errors
- **Unity Framework**: Excludes Unity framework JSON files from analysis
- **Test Configuration**: Includes test configuration files in validation

## Code Metrics and Analysis

### File Size Analysis

- **Distribution Bins**: 0-99, 100-199, 200-299, ..., 900-999, 1000+ lines
- **Oversized Detection**: Identifies files exceeding 1000 lines
- **Type-Specific Analysis**: Separate analysis for different file types
- **Refactoring Guidance**: Highlights files that may need splitting

### cloc Integration

- **Comprehensive Metrics**: Lines of code, comments, blank lines
- **Language Detection**: Automatic language classification
- **Exclusion Support**: Respects .lintignore patterns
- **Summary Statistics**: Total files and code lines across project

### Large File Detection

- **Size Threshold**: 25KB for non-source files
- **Repository Impact**: Identifies files that may slow repository operations
- **Exclusion Logic**: Skips source code, documentation, and test files
- **Optimization Opportunities**: Highlights files for potential optimization

## Version History

- **3.0.0** (2025-07-02): Migrated to use lib/ scripts following established test patterns
- **2.0.1** (2025-07-01): Updated to use CMake cleanish target instead of Makefile discovery
- **2.0.0** (2025-06-17): Major refactoring with improved modularity and enhanced comments
- **1.0.1** (2025-06-16): Added comprehensive linting support for multiple languages
- **1.0.0** (2025-06-15): Initial version with basic codebase analysis

## Related Documentation

- [test_00_all.md](test_00_all.md) - Main test suite documentation
- [LIBRARIES.md](LIBRARIES.md) - Modular library documentation
- [testing.md](../../docs/testing.md) - Overall testing strategy
- [coding_guidelines.md](../../docs/coding_guidelines.md) - Code quality standards
- [developer_onboarding.md](../../docs/developer_onboarding.md) - Development environment setup
