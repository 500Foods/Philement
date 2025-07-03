# GitHub Sitemap Library Documentation

## Overview

The GitHub Sitemap Library (`lib/github-sitemap.sh`) is a comprehensive markdown link checker script designed to analyze and maintain the integrity of markdown documentation within a GitHub repository. It automates the process of checking links and mapping file relationships in markdown files.

## Purpose

- Check local markdown links in repositories for validity
- Report missing or broken links with detailed information
- Identify orphaned markdown files that aren't linked by other files
- Generate structured reports and visual tables
- Support repository documentation maintenance workflows

## Key Features

- **Markdown Link Checking** - Validates local links and GitHub URLs
- **Orphaned File Detection** - Identifies unlinked markdown files
- **Detailed Reporting** - Generates comprehensive reports and tables
- **Customizable Output** - Multiple output options and themes
- **Exclusion Support** - Honors `.lintignore` patterns for file exclusion
- **Queue-based Processing** - Efficiently processes linked files

## Script Information

- **Version**: 0.4.1
- **Usage**: `./github-sitemap.sh <markdown_file> [options]`
- **Description**: Checks local markdown links in a repository, reports missing links, and identifies orphaned markdown files

## Command Line Options

### Required Parameters

- `<markdown_file>` - Starting markdown file to analyze

### Optional Flags

- `--debug` - Enable debug logging with timestamps
- `--quiet` - Display only tables, suppress other output
- `--noreport` - Do not create a report file
- `--help` - Display help message and exit
- `--theme <Red|Blue>` - Set table theme (default: Red)

### Usage Examples

```bash
# Basic usage
./github-sitemap.sh README.md

# With debug output
./github-sitemap.sh README.md --debug

# Quiet mode with blue theme
./github-sitemap.sh README.md --quiet --theme Blue

# Skip report file generation
./github-sitemap.sh README.md --noreport

# Show help
./github-sitemap.sh --help
```

## How It Works

### Processing Pipeline

1. **Initialization** - Parse arguments and validate dependencies
2. **File Processing** - Start with input file and process queue
3. **Link Extraction** - Extract markdown links using regex patterns
4. **Link Validation** - Check if linked files/directories exist
5. **Queue Management** - Add linked markdown files to processing queue
6. **Orphan Detection** - Find markdown files not linked by others
7. **Report Generation** - Create tables and optional report file

### Link Types Supported

#### GitHub URLs

- Pattern: `https://github.com/[user]/[repo]/blob/main/[path]`
- Extracts relative path for local validation
- Only `.md` files are traversed for further processing

#### Local Relative Links

- Relative paths from current file location
- Both files and directories are validated
- Only `.md` files are added to processing queue

#### Excluded Links

- Fragment links (starting with `#`)
- External URLs (`http`, `ftp`, `mailto`)
- These are ignored during processing

### File Discovery and Exclusion

The script uses `.lintignore` patterns to exclude files:

```bash
# Example .lintignore patterns
*.tmp
build/
docs/generated/*
```

**Exclusion Logic:**

- Reads patterns from `.lintignore` in repository root
- Skips empty lines and comments (starting with `#`)
- Uses bash extended globbing for pattern matching
- Applies to both processing and orphan detection

## Global Variables

### Tracking Arrays

```bash
declare -A checked_files    # Track processed files
declare -A linked_files     # Track files that have been linked to
declare -A file_summary     # Store file statistics
declare -a missing_links    # Store missing link entries
declare -a orphaned_files   # Store orphaned file list
declare -a queue           # Processing queue
```

### Configuration Variables

```bash
DEBUG=false           # Debug logging flag
QUIET=false          # Quiet output mode
NOREPORT=false       # Skip report file creation
TABLE_THEME="Red"    # Table color theme
APPVERSION="0.4.1"   # Script version
```

## Core Functions

### extract_links

Extracts local markdown links from a file using regex patterns.

**Parameters:**

- `file` - Path to the markdown file to analyze

**Returns:**

- Outputs valid links to stdout, one per line
- Prefixes non-markdown links with "non_md:"

**Features:**

- Handles GitHub URLs and extracts relative paths
- Processes local relative links
- Filters out external URLs and fragments
- Only includes `.md` files for traversal

**Example:**

```bash
links=$(extract_links "README.md")
```

### resolve_path

Converts relative paths to absolute paths from a base directory.

**Parameters:**

- `base_dir` - Base directory for resolution
- `rel_path` - Relative path to resolve

**Returns:**

- Absolute path via stdout

**Example:**

```bash
abs_path=$(resolve_path "/repo/docs" "../README.md")
```

### relative_to_root

Converts absolute paths to paths relative to repository root.

**Parameters:**

- `abs_path` - Absolute path to convert

**Returns:**

- Relative path via stdout

**Example:**

```bash
rel_path=$(relative_to_root "/repo/docs/guide.md")
# Returns: docs/guide.md
```

### process_file

Main function to process a single markdown file.

**Parameters:**

- `file` - Path to the markdown file to process

**Side Effects:**

- Updates `checked_files`, `linked_files`, `file_summary`
- Adds entries to `missing_links` for broken links
- Adds linked markdown files to processing queue

**Features:**

- Checks `.lintignore` exclusion patterns
- Validates all extracted links
- Tracks statistics (total, checked, missing links)
- Handles both files and directories as valid targets

### find_all_md_files

Discovers all markdown files in the repository.

**Returns:**

- Outputs relative file paths to stdout

**Features:**

- Uses `find` with timeout and depth limits
- Excludes `.git` directories and symlinks
- Honors `.lintignore` exclusion patterns
- Provides error handling and logging

### generate_*_json Functions

Generate JSON data and layout files for table rendering:

- `generate_reviewed_files_json` - Creates data for reviewed files table
- `generate_missing_links_json` - Creates data for missing links table
- `generate_orphaned_files_json` - Creates data for orphaned files table
- `generate_*_layout_json` - Creates layout configurations for tables

**Features:**

- Sorts data using modified path logic (files before folders)
- Includes summary calculations and break grouping
- Supports different table themes

## Output Tables

### Reviewed Files Summary

Shows statistics for all processed files:

| Column | Description |
|--------|-------------|
| File | Relative path to the markdown file |
| Folder | Directory path (hidden, used for grouping) |
| Total Links | Total number of links found |
| Found Links | Number of valid links |
| Missing Links | Number of broken links |

**Features:**

- Groups by folder with break separators
- Includes summary totals for all columns
- Counts all processed files

### Missing Links

Lists all broken or missing links:

| Column | Description |
|--------|-------------|
| File | File containing the broken link |
| Missing Link | The broken link path |

**Features:**

- Shows only files with missing links
- Includes count summary

### Orphaned Markdown Files

Lists markdown files not linked by any other file:

| Column | Description |
|--------|-------------|
| File | Relative path to orphaned file |

**Features:**

- Shows files that exist but aren't referenced
- Includes count summary

## Report File

When `--noreport` is not used, creates `sitemap_report.txt` with:

- Link check summary for each file
- List of missing links with file locations
- List of orphaned markdown files
- Detailed statistics and warnings

## Dependencies

### Required Tools

- **bash** - Shell environment (version 4.0+)
- **jq** - JSON processing for data generation
- **find** - File discovery with exclusion support
- **grep** - Pattern matching for link extraction
- **realpath** - Path resolution and normalization

### Optional Tools

- **timeout** - Command timeout support (warns if missing)

### Custom Dependencies

- **tables.sh** - Custom table rendering library
- Must be in the same directory as the script
- Used for generating formatted output tables

## Error Handling

### File Validation

- Checks for readable input files
- Validates script dependencies
- Handles missing or inaccessible files gracefully

### Path Resolution

- Robust path resolution with error checking
- Handles both absolute and relative paths
- Manages repository root detection

### Processing Errors

- Continues processing on individual file errors
- Logs warnings for missing files
- Provides detailed debug information

## Performance Considerations

- **Queue-based Processing** - Avoids duplicate file processing
- **Timeout Support** - Prevents hanging on large repositories
- **Depth Limits** - Controls find command scope
- **Pattern Exclusion** - Reduces processing overhead

## Integration Patterns

### CI/CD Integration

```bash
# Check documentation in CI pipeline
./github-sitemap.sh README.md --quiet --noreport
exit_code=$?
if [[ $exit_code -gt 0 ]]; then
    echo "Documentation issues found: $exit_code"
    exit 1
fi
```

### Pre-commit Hook

```bash
#!/bin/bash
# Pre-commit hook to check documentation
./scripts/github-sitemap.sh README.md --quiet
```

### Regular Maintenance

```bash
# Weekly documentation check
./github-sitemap.sh README.md --theme Blue > weekly_report.txt
```

## Version History

- **0.4.1** (2025-06-30) - Enhanced .lintignore exclusion handling, fixed shellcheck warning
- **0.4.0** (2025-06-19) - Updated link checking to recognize existing folders as valid links
- **0.3.9** (2025-06-15) - Fixed repo root detection, improved path resolution
- **0.3.8** (2025-06-15) - Fixed find_all_md_files path resolution, enhanced debug logging
- **0.3.7** (2025-06-15) - Optimized find_all_md_files with .git pruning
- **0.3.6** (2025-06-15) - Fixed orphaned files table, added theme option
- **0.3.5** (2025-06-15) - Fixed tables.sh invocation, improved debug output
- **0.3.4** (2025-06-15) - Fixed debug flag handling for tables.sh
- **0.3.3** (2025-06-15) - Enhanced find_all_md_files with symlink avoidance
- **0.3.2** (2025-06-15) - Improved find_all_md_files with timeout and error handling
- **0.3.1** (2025-06-15) - Changed to execute tables.sh instead of sourcing
- **0.3.0** (2025-06-15) - Added debug flag, improved robustness
- **0.2.0** (2025-06-15) - Added table output using tables.sh
- **0.1.0** (2025-06-15) - Initial version with basic link checking

## Best Practices

1. **Start with main documentation** - Use README.md or main index file
2. **Use appropriate themes** - Choose Red or Blue based on preference
3. **Enable debug for troubleshooting** - Use `--debug` for detailed logging
4. **Maintain .lintignore** - Exclude generated or temporary files
5. **Regular checks** - Integrate into development workflow
6. **Review orphaned files** - Regularly check for unlinked documentation

## Troubleshooting

### Common Issues

**Script not finding tables.sh:**

- Ensure tables.sh is in the same directory
- Check file permissions

**No markdown files found:**

- Verify repository structure
- Check .lintignore exclusions
- Enable debug mode for detailed logging

**Timeout errors:**

- Large repositories may need timeout adjustments
- Consider excluding large directories

**Path resolution errors:**

- Ensure proper file permissions
- Check for broken symlinks
- Verify repository structure

## Related Documentation

- [Tables Library](tables.md) - Table rendering system used for output
- [Framework Library](framework.md) - Testing framework integration
- [LIBRARIES.md](LIBRARIES.md) - Complete library documentation index
