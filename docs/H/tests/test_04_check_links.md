# Test 06: Markdown Links Check Script Documentation

## Overview

The `test_06_check_links.sh` script runs github-sitemap.sh to check markdown links and evaluates results with subtests.

## Purpose

This script ensures markdown documentation maintains proper link integrity by validating internal links, identifying missing files, and detecting orphaned markdown files.

## Script Details

- **Script Name**: `test_06_check_links.sh`
- **Test Name**: Markdown Links Check
- **Version**: 2.0.3 (Fixed table detection and extraction logic)
- **Dependencies**: Uses modular libraries from `lib/` directory

## Key Features

- Validates sitemap script and README existence
- Executes link check with github-sitemap.sh
- Parses and validates missing links count
- Parses and validates orphaned files count

## Technical Implementation

### Library Dependencies

- `log_output.sh` - Formatted output and logging
- `file_utils.sh` - File operations and path management
- `framework.sh` - Test framework and result tracking

### Test Sequence (4 Subtests)

1. Validate GitHub Sitemap Script
2. Execute Markdown Link Check
3. Validate Missing Links Count
4. Validate Orphaned Files Count

### Output Parsing Logic

- Extracts issues from output tables

## Usage

To run this test as part of the full suite:

```bash
./test_00_all.sh all
```

To run this test individually:

```bash
./test_06_check_links.sh
```

## Expected Results

- GitHub sitemap script and README.md found
- Link check executes without errors
- Zero missing links detected
- Zero orphaned markdown files found

## Troubleshooting

- Ensure `tests/lib/github-sitemap.sh` exists
- Verify `README.md` exists in project root

## Version History

- **2.0.3** (2025-07-07): Fixed table detection
- **2.0.2** (2025-07-07): Fixed extraction logic
- **2.0.1** (2025-07-06): Added shellcheck justifications
- **2.0.0** (2025-07-02): Migrated to lib/ scripts
- **1.0.0** (2025-06-20): Initial version

## Related Documentation

- [test_00_all.md](test_00_all.md) - Main test suite documentation
- [LIBRARIES.md](LIBRARIES.md) - Modular library documentation
