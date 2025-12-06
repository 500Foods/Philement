# GitHub Sitemap Library Documentation

## Overview

The GitHub Sitemap Library (`github-sitemap.sh`) is a comprehensive markdown link checker script designed to analyze and maintain the integrity of markdown documentation within a GitHub repository. It automates the process of checking links and mapping file relationships in markdown files using parallel processing for enhanced performance.

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
- **Exclusion Support** - Honors `.lintignore` patterns or custom ignore files for file exclusion
- **Parallel Processing** - Utilizes `xargs` for faster processing with configurable job counts
- **Performance Profiling** - Measures and reports execution times for different stages

## Script Information

- **Version**: 0.7.0
- **Usage**: `./github-sitemap.sh <markdown_file> [options]`
- **Description**: Checks local markdown links in a repository using parallel processing, reports missing links, and identifies orphaned markdown files

## Command Line Options

### Required Parameters

- `<markdown_file>` - Starting markdown file to analyze

### Optional Flags

- `--debug` - Enable debug logging with timestamps
- `--quiet` - Display only tables, suppress other output
- `--noreport` - Do not create a report file
- `--help` - Display help message and exit
- `--theme <Red|Blue>` - Set table theme (default: Red)
- `--profile` - Enable performance profiling
- `--jobs <N>` - Specify the number of parallel jobs (default: auto-detected based on CPU count)
- `--ignore <file>` - Specify a custom file with ignore patterns (overrides default `.lintignore`)

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

# Enable performance profiling
./github-sitemap.sh README.md --profile

# Specify number of parallel jobs
./github-sitemap.sh README.md --jobs 4

# Use a custom ignore file
./github-sitemap.sh README.md --ignore custom_ignore.txt

# Show help
./github-sitemap.sh --help
```

## How It Works

### Processing Pipeline

1. **Initialization** - Parse arguments, validate dependencies, and set up parallel processing parameters
2. **File Cache Building** - Build a cache of file existence for faster lookups
3. **Parallel Processing Setup** - Prepare temporary files and export cache for parallel jobs
4. **Link Extraction and Validation** - Extract markdown links and check if linked files/directories exist using `xargs` for parallel execution
5. **Iterative Discovery** - Process linked markdown files in multiple iterations to cover the full network of links
6. **Orphan Detection** - Find markdown files not linked by others
7. **Report Generation** - Create tables, performance summaries, and optional report file

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

The script uses `.lintignore` patterns by default or a custom ignore file specified via `--ignore` to exclude files:

```bash
# Example ignore patterns
*.tmp
build/
docs/generated/*
```

**Exclusion Logic:**

- Reads patterns from `.lintignore` in repository root or a specified ignore file
- Skips empty lines and comments (starting with `#`)
- Uses bash extended globbing for pattern matching
- Applies to both processing and orphan detection

## Global Variables

### Tracking Arrays and Associative Arrays

```bash
declare -A file_exists_cache  # Cache of file existence for performance
declare -A checked_files      # Track processed files
declare -A file_summary       # Store file statistics
declare -a missing_links      # Store missing link entries
declare -a orphaned_files     # Store orphaned file list
declare -A timing_data        # Store performance timing data
```

### Configuration Variables

```bash
DEBUG=false           # Debug logging flag
QUIET=false           # Quiet output mode
NOREPORT=false        # Skip report file creation
TABLE_THEME="Red"     # Table color theme
APPVERSION="0.7.0"    # Script version
PROFILE=false         # Performance profiling flag
JOBS=""               # Number of parallel jobs (auto-detected if not set)
IGNORE_FILE=""        # Custom ignore file path
```

## Core Functions

### detect_cpu_count

Detects the optimal number of parallel jobs based on CPU count.

**Returns:**

- Number of CPUs or a capped value (max 8) for reasonable performance

**Features:**

- Uses `nproc`, `/proc/cpuinfo`, or `sysctl` for CPU detection
- Caps at 8 jobs to balance performance and system load

### timing_start and timing_end

Records performance timing for different stages of execution.

**Parameters:**

- `name` - Name of the timing stage

**Features:**

- Stores start and end times in nanoseconds
- Calculates duration in milliseconds
- Outputs profiling data if `--profile` is enabled

### process_file_batch

Optimized function for processing a batch of files in parallel via `xargs`.

**Parameters:**

- `cache_file` - File containing the file existence cache
- `repo_root` - Repository root directory
- `original_dir` - Original working directory
- `results_file` - File to store processing results
- Reads file paths from stdin for processing

**Features:**

- Loads file cache for fast existence checks
- Extracts and validates links from markdown files
- Outputs structured results for summary, missing links, and linked files
- Handles ignore patterns within parallel processes

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

### build_file_cache

Builds a cache of file existence for performance optimization.

**Features:**

- Uses a single `find` command to populate the cache
- Applies ignore patterns during cache building
- Stores file and directory types for quick lookup

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
- Detailed statistics and performance metrics (if profiling is enabled)

## Dependencies

### Required Tools

- **bash** - Shell environment (version 4.0+)
- **jq** - JSON processing for data generation
- **find** - File discovery with exclusion support
- **grep** - Pattern matching for link extraction
- **realpath** - Path resolution and normalization
- **xargs** - Parallel processing of files

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

- **Parallel Processing with xargs** - Significantly speeds up processing of large repositories by utilizing multiple CPU cores
- **File Existence Cache** - Reduces disk I/O by caching file system lookups
- **Iterative Processing** - Efficiently handles linked file discovery in batches
- **Profiling Option** - Allows detailed performance analysis to identify bottlenecks

## Integration Patterns

### CI/CD Integration

```bash
# Check documentation in CI pipeline
./github-sitemap.sh README.md --quiet --noreport --jobs 4
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
# Weekly documentation check with profiling
./github-sitemap.sh README.md --theme Blue --profile > weekly_report.txt
```

## Version History

- **0.7.0** (2025-07-04) - Major performance overhaul with `xargs` parallel processing, added `--profile`, `--jobs`, and `--ignore` options
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
4. **Maintain ignore patterns** - Exclude generated or temporary files using `.lintignore` or custom ignore files
5. **Optimize parallel jobs** - Adjust `--jobs` based on system capabilities for optimal performance
6. **Regular checks** - Integrate into development workflow
7. **Review orphaned files** - Regularly check for unlinked documentation
8. **Profile performance** - Use `--profile` to analyze execution time on large repositories

## Troubleshooting

### Common Issues

**Script not finding tables.sh:**

- Ensure tables.sh is in the same directory
- Check file permissions

**No markdown files found:**

- Verify repository structure
- Check ignore file exclusions
- Enable debug mode for detailed logging

**Performance issues:**

- Adjust the number of jobs with `--jobs` if processing is too slow or overloading the system
- Use `--profile` to identify slow stages
- Consider excluding large directories in ignore patterns

**Path resolution errors:**

- Ensure proper file permissions
- Check for broken symlinks
- Verify repository structure
