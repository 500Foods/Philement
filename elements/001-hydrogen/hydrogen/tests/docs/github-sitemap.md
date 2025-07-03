# GitHub Sitemap Script

## Overview

**github-sitemap.sh** is a Bash script designed to analyze and maintain the integrity of markdown documentation within a GitHub repository. It automates the process of checking links and mapping file relationships in markdown files, helping repository maintainers ensure their documentation is accurate and well-connected.

## What It Does

The script provides a detailed analysis of markdown files in a repository with the following features:

- **Markdown Link Checking**: Scans markdown files to identify local links and verifies if they point to existing files or directories. It flags broken or missing links for correction.
- **Orphaned File Detection**: Identifies markdown files that are not linked to by any other markdown file in the repository, helping to uncover disconnected or overlooked documentation.
- **Detailed Reporting**: Generates structured reports and tables summarizing the status of links and listing orphaned files. These outputs provide clear insights into the documentation structure.
- **Customizable Output**: Supports options to adjust the level of detail and presentation, including `--debug` for verbose logging, `--quiet` to minimize output, `--noreport` to skip report file creation, and `--theme` to select table color schemes (Red or Blue).

## How It Works

The script operates by recursively scanning a specified directory (or the current directory if none is provided) for markdown files. It performs the following steps:

1. **File Discovery**: Uses `find` to locate all markdown files (`.md`) while excluding irrelevant directories like `.git`. It builds a list of files to analyze.
2. **Link Extraction and Validation**: Reads each markdown file, extracts local links using regular expressions, and checks if the linked paths exist as files or directories within the repository.
3. **Relationship Mapping**: Tracks which files link to others and identifies files that have no incoming links (orphaned files).
4. **Report Generation**: Outputs results to the console with optional tables for visual clarity. If enabled, it saves a detailed report to a file (e.g., `sitemap_report.txt`) for later reference.

The script handles both relative and absolute links by resolving paths based on the location of the markdown file containing the link. It also includes error handling and dependency checks to ensure reliable operation.

## How to Use It in Your Repository

To use `github-sitemap.sh` in your own GitHub repository, follow these steps:

1. **Download or Clone the Script**: Obtain the script by downloading it from this repository or cloning the entire repository to your local machine.
2. **Place the Script in Your Repository**: Copy `github-sitemap.sh` to a suitable location within your repository, such as a `scripts/` or `tools/` directory.
3. **Make It Executable**: Run `chmod +x github-sitemap.sh` to ensure the script can be executed on Unix-like systems (Linux, macOS).
4. **Run the Script**: Execute the script from the command line. You can specify a directory to analyze or let it default to the current directory:

   ```bash
   ./github-sitemap.sh [directory]
   ```

   - Replace `[directory]` with the path to the directory containing your markdown files if you want to analyze a specific subset of your repository.
5. **Review the Output**: The script will display a summary of its findings, including counts of broken links and orphaned files. If tables are enabled, it will show detailed lists. Check the optional report file (e.g., `sitemap_report.txt`) if you did not use the `--noreport` option.
6. **Customize Behavior with Options**: Use command-line flags to tailor the script's operation to your needs:
   - `--debug`: Enable detailed logging for troubleshooting.
   - `--quiet`: Suppress non-essential output for a cleaner display.
   - `--noreport`: Prevent the creation of a report file.
   - `--theme Red` or `--theme Blue`: Choose a color scheme for table output.
   Example:

   ```bash
   ./github-sitemap.sh --theme Blue --quiet
   ```

7. **Integrate into Workflows**: Consider adding the script to your CI/CD pipeline or a cron job to regularly check documentation integrity. For example, you could create a GitHub Action to run the script on every push or pull request to catch issues early.

## Requirements

- **Bash**: The script requires a Bash environment, available on Linux, macOS, and Windows (via WSL or Git Bash).
- **Dependencies**: The script checks for necessary tools like `find`, `grep`, and `jq` (for JSON processing). Ensure these are installed on your system.
- **Custom Library**: The script uses a custom `tables.sh` library for rendering output tables. Ensure this library is accessible or bundled with the script if you move it to another repository.

## Limitations

- The script focuses on local links within markdown files and does not validate external URLs.
- It assumes a standard GitHub repository structure and may require adjustments for non-standard setups.
- Performance may vary with very large repositories due to the recursive nature of file discovery.

## Output Example

When run, the script might produce output like this (simplified for brevity):

```bash
Analyzing markdown files in ./docs...
Total Files: 25
Broken Links: 3
Orphaned Files: 5

Missing Links:
| File                 | Broken Link          |
|----------------------|----------------------|
| ./docs/setup.md      | ./docs/missing.md    |
| ./docs/guide.md      | ./docs/old.md        |

Orphaned Files:
| File                 |
|----------------------|
| ./docs/unlinked.md   |
| ./docs/hidden.md     |
```

A more detailed report may be saved to a file if the `--noreport` option is not used.

## Use Cases

- **Documentation Maintenance**: Regularly run the script to ensure all links in your documentation are valid and no files are orphaned.
- **Repository Cleanup**: Use the orphaned file detection to identify and remove or reorganize outdated documentation.
- **Onboarding Contributors**: Provide new contributors with a clear map of documentation structure by sharing the script's reports.
- **Pre-Release Checks**: Include the script in pre-release checklists to verify documentation integrity before a new version is published.

By incorporating `github-sitemap.sh` into your workflow, you can maintain a high-quality, navigable documentation structure for your GitHub repository.
