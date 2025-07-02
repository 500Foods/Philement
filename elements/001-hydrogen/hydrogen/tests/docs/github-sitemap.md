# GitHub Sitemap Library Documentation

## Overview

The `github-sitemap.sh` script is a utility for checking local markdown links within a repository. It identifies missing links, reports on orphaned markdown files (files not linked to by any other markdown file), and generates a detailed sitemap report. This tool is essential for maintaining documentation integrity by ensuring all links are valid and all files are accessible through the documentation structure.

## Usage

To use the GitHub Sitemap script, run it with a markdown file as the starting point for link checking:

```bash
./github-sitemap.sh <markdown_file> [--debug] [--quiet] [--noreport] [--help] [--theme <Red|Blue>]
```

### Options

- `--debug`: Enable debug logging for detailed output during execution.
- `--quiet`: Suppress non-table output, displaying only the summary tables.
- `--noreport`: Prevent the creation of a report file, useful for quick checks.
- `--help`: Display usage instructions and options.
- `--theme <Red|Blue>`: Set the table theme for output rendering (default is Red).

## Functionality

- **Link Checking**: Recursively checks all local markdown links starting from the specified file, verifying their existence.
- **Missing Links Report**: Lists all links that point to non-existent files or directories.
- **Orphaned Files Detection**: Identifies markdown files within the repository that are not linked to by any other markdown file.
- **Report Generation**: Outputs a detailed report to `sitemap_report.txt` in the working directory, unless `--noreport` is specified.
- **Table Output**: Utilizes `tables.sh` to render summary tables for reviewed files, missing links, and orphaned files.

## Integration

This script integrates with `tables.sh` for rendering output tables, ensuring a visually appealing and organized presentation of results. Ensure `tables.sh` is present in the same directory as `github-sitemap.sh` for proper functionality.

## Examples

Check links starting from a specific markdown file with debug output:

```bash
./github-sitemap.sh README.md --debug
```

Generate a report with a Blue theme for tables:

```bash
./github-sitemap.sh README.md --theme Blue
```

## Version History

- **0.4.1** (2025-06-30): Enhanced .lintignore exclusion handling, fixed shellcheck warning for command execution.
- **0.4.0** (2025-06-19): Updated link checking to recognize existing folders as valid links.
- **0.3.9** (2025-06-15): Fixed repo root detection to use input file's directory, improved path resolution.
- **0.3.8** (2025-06-15): Fixed find_all_md_files path resolution, reordered -maxdepth, enhanced debug logging.
- **0.3.7** (2025-06-15): Optimized find_all_md_files with -prune for .git, fixed orphaned files detection.
- **0.3.6** (2025-06-15): Fixed orphaned files table, empty link counts, added --theme option, added orphaned files to report.
- **0.3.5** (2025-06-15): Fixed tables.sh invocation to avoid empty argument, improved debug output separation.
- **0.3.4** (2025-06-15): Fixed debug flag handling to pass --debug to tables.sh only when DEBUG=true.
- **0.3.3** (2025-06-15): Enhanced find_all_md_files to avoid symlinks, log errors, and use shorter timeout.
- **0.3.2** (2025-06-15): Improved find_all_md_files with depth limit, timeout, and error handling.
- **0.3.1** (2025-06-15): Changed to execute tables.sh instead of sourcing, per domain_info.sh.
- **0.3.0** (2025-06-15): Added --debug flag for verbose logging, improved robustness.
- **0.2.0** (2025-06-15): Added table output using tables.sh, version display, issue count, missing links table, and orphaned files table.
- **0.1.0** (2025-06-15): Initial version with basic link checking and summary output.

## Related Documentation

- [Tables Library](tables.md) - Documentation for the table rendering library used by this script.
- [LIBRARIES.md](LIBRARIES.md) - Central Table of Contents for all test suite library documentation.
- [Script](../lib/github-sitemap.sh) - Direct link to the script source.
