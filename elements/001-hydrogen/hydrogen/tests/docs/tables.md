# Tables Library Documentation

## Overview

The `tables.sh` script is a reusable library for rendering JSON data as ANSI tables in bash scripts. It provides a flexible and modular approach to table creation, supporting various data types, themes, and rendering options. This library is designed to enhance the visual output of scripts by presenting data in a structured, readable format.

## Usage

The Tables library can be used either as a sourced library within another script or directly from the command line:

### As a Library in Scripts

```bash
source /path/to/tables.sh
draw_table "layout.json" "data.json" [--debug]
```

### Command Line Execution

```bash
./tables.sh layout.json data.json [--debug] [--version] [--help]
```

### Arguments and Options

- `layout_json_file`: Path to a JSON file containing the table layout configuration.
- `data_json_file`: Path to a JSON file containing the data to be rendered in the table.
- `--debug`: Enable debug logging to stderr for troubleshooting.
- `--version`: Display the version information of the Tables library.
- `--help`: Show usage information and available options.

## Functionality

- **Modular Design**: The library is split into multiple modules (`tables_themes.sh`, `tables_datatypes.sh`, `tables_render.sh`, `tables_config.sh`, `tables_data.sh`) for maintainability and clarity.
- **Theme Support**: Supports customizable themes for table appearance, with predefined options like "Red" and "Blue".
- **Data Type Handling**: Supports various data types with specific formatting and justification options.
- **Dynamic Rendering**: Automatically adjusts column widths based on content and supports multi-line content within cells.
- **Summaries**: Provides functionality to calculate and display summaries (e.g., count, sum) for columns.

## Integration

This library is used by other scripts like `github-sitemap.sh` to render output tables for summaries and reports. Ensure all dependent modules are in the same directory as `tables.sh` for proper functionality.

## Examples

Render a table from JSON files with debug output enabled:

```bash
./tables.sh layout.json data.json --debug
```

Display version information:

```bash
./tables.sh --version
```

Show help information:

```bash
./tables.sh --help
```

## Version History

- **1.0.4**: Fixed border rendering for titles and footers wider than the table.
- **1.0.3**: Visible: false columns are now excluded from width calculations.
- **1.0.2**: Added help functionality and version history section.
- **1.0.1**: Fixed shellcheck issues (SC2004, SC2155).
- **1.0.0**: Initial release with table rendering functionality.

## Related Documentation

- [GitHub Sitemap Library](github-sitemap.md) - Documentation for a script that utilizes this library for table output.
- [LIBRARIES.md](LIBRARIES.md) - Central Table of Contents for all test suite library documentation.
- [Script](../lib/tables.sh) - Direct link to the script source.
