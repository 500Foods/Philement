# Test 98: Check Links Script Documentation

## Overview

The `test_98_check_links.sh` script is a comprehensive markdown link validation tool within the Hydrogen test suite, focused on checking markdown links and identifying documentation issues using the GitHub sitemap functionality.

## Purpose

This script ensures that all markdown documentation maintains proper link integrity by validating internal links, identifying missing files, and detecting orphaned markdown files. It is essential for maintaining documentation quality and ensuring that all documentation remains accessible and properly interconnected.

## Script Details

- **Script Name**: `test_98_check_links.sh`
- **Test Name**: Markdown Links Check
- **Version**: 2.0.0 (Migrated to use lib/ scripts following established patterns)
- **Dependencies**: Uses modular libraries from `lib/` directory

## Key Features

### Core Functionality

- **GitHub Sitemap Integration**: Uses `github-sitemap.sh` for comprehensive link checking
- **Missing Link Detection**: Identifies broken internal links in markdown files
- **Orphaned File Detection**: Finds markdown files not referenced by any other files
- **Automated Validation**: Runs link checking with minimal output for automated testing
- **Detailed Reporting**: Provides comprehensive analysis of documentation link health

### Advanced Testing Capabilities

- **Prerequisites Validation**: Ensures sitemap script and target README exist
- **Output Parsing**: Extracts specific metrics from sitemap output tables
- **Issue Categorization**: Separates missing links from orphaned files
- **Summary Generation**: Creates detailed summaries of documentation issues
- **Result Preservation**: Saves detailed reports for further analysis

### Link Analysis Features

- **Internal Link Validation**: Checks all internal markdown links
- **File Existence Verification**: Ensures linked files actually exist
- **Cross-Reference Analysis**: Identifies files not linked from anywhere
- **Table-Based Reporting**: Structured output with clear metrics
- **Issue Counting**: Precise counting of different types of issues

## Technical Implementation

### Library Dependencies

- `log_output.sh` - Formatted output and logging
- `file_utils.sh` - File operations and path management
- `framework.sh` - Test framework and result tracking

### GitHub Sitemap Integration

- **Script Location**: `tests/lib/github-sitemap.sh`
- **Target File**: `README.md` (project root)
- **Execution Flags**: `--noreport --quiet` for minimal output
- **Output Processing**: Parses structured table output for metrics

### Test Sequence (4 Subtests)

1. **Script Validation**: Verifies GitHub sitemap script and target README exist
2. **Link Check Execution**: Runs markdown link check with sitemap script
3. **Missing Links Analysis**: Parses output to count missing links
4. **Orphaned Files Analysis**: Identifies and counts orphaned markdown files

### Output Parsing Logic

- **Issues Count**: Extracts total issues from "Issues found:" line
- **Missing Links**: Parses table data to extract missing links count
- **Orphaned Files**: Counts entries in orphaned files table
- **Summary Row**: Analyzes table summary for comprehensive metrics

## Usage

To run this test as part of the full suite:

```bash
./test_00_all.sh all
```

To run this test individually:

```bash
./test_98_check_links.sh
```

## Expected Results

### Successful Test Criteria

- GitHub sitemap script and README.md found
- Link check executes without errors
- Zero missing links detected
- Zero orphaned markdown files found
- Comprehensive link analysis report generated

### Link Health Metrics

- **Total Issues**: Overall count of documentation problems
- **Missing Links**: Count of broken internal links
- **Orphaned Files**: Count of unreferenced markdown files
- **Link Integrity**: Overall health of documentation cross-references

## Troubleshooting

### Common Issues

- **Missing Sitemap Script**: Ensure `tests/lib/github-sitemap.sh` exists
- **Missing README**: Verify `README.md` exists in project root
- **Link Check Failures**: Review detailed output for specific broken links
- **Parsing Errors**: Check sitemap output format for table structure changes

### Diagnostic Information

- **Results Directory**: `tests/results/`
- **Detailed Reports**: Complete sitemap output saved for analysis
- **Link Analysis**: Specific broken links and orphaned files listed
- **Timestamped Output**: All results include timestamp for tracking

## GitHub Sitemap Integration Notes

### Sitemap Script Features

- **Comprehensive Analysis**: Scans entire project for markdown files
- **Link Validation**: Checks all internal markdown links
- **Cross-Reference Mapping**: Builds complete link dependency graph
- **Structured Output**: Provides table-based reporting format

### Execution Parameters

- **Target**: `README.md` as starting point for link analysis
- **No Report**: `--noreport` flag prevents full report generation
- **Quiet Mode**: `--quiet` flag minimizes verbose output
- **Exit Codes**: Returns appropriate exit codes for automation

## Documentation Quality Assurance

### Link Integrity Standards

- **Internal Links**: All internal markdown links must resolve to existing files
- **Cross-References**: Documentation files should be properly interconnected
- **Orphan Prevention**: All markdown files should be referenced from somewhere
- **Consistency**: Link formats should follow project conventions

### Automated Validation

- **Continuous Checking**: Regular validation of documentation links
- **Integration Testing**: Link checking as part of test suite
- **Quality Gates**: Prevents documentation degradation
- **Maintenance Support**: Identifies documentation maintenance needs

## Version History

- **2.0.0** (2025-07-02): Migrated to use lib/ scripts following established test patterns
- **1.0.0** (2025-06-20): Initial version with subtests for markdown link checking

## Related Documentation

- [test_00_all.md](test_00_all.md) - Main test suite documentation
- [test_99_codebase.md](test_99_codebase.md) - Codebase analysis testing
- [LIBRARIES.md](LIBRARIES.md) - Modular library documentation
- [testing.md](../../docs/testing.md) - Overall testing strategy
- [developer_onboarding.md](../../docs/developer_onboarding.md) - Documentation standards
- [README.md](../../README.md) - Project root documentation
