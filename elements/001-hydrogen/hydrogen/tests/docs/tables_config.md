# Tables Config Library Documentation

## Overview

The Tables Config Library (`lib/tables_config.sh`) provides configuration parsing and validation functionality for the tables.sh system. This library handles the parsing of layout JSON files and manages column and sort configurations for table rendering.

## Purpose

- Parse and validate layout JSON configuration files
- Extract table structure information (columns, sorting, themes)
- Manage global configuration arrays for table rendering
- Validate column and sort configurations with error handling
- Support title and footer configuration with positioning

## Key Features

- **JSON Layout Parsing** - Extracts theme, columns, and sort information from JSON
- **Column Configuration** - Manages headers, data types, formatting, and display options
- **Sort Configuration** - Handles multi-column sorting with priorities
- **Title/Footer Support** - Configures table titles and footers with positioning
- **Validation** - Validates configuration parameters with warnings for invalid values
- **Global State Management** - Maintains configuration in global arrays for other modules

## Global Variables

### Title and Footer Configuration

- `TABLE_TITLE` - The table title text
- `TITLE_WIDTH` - Calculated width for title display
- `TITLE_POSITION` - Title position (left, right, center, full, none)
- `TABLE_FOOTER` - The table footer text
- `FOOTER_WIDTH` - Calculated width for footer display
- `FOOTER_POSITION` - Footer position (left, right, center, full, none)

### Column Configuration Arrays

- `HEADERS[]` - Column header text
- `KEYS[]` - JSON field names for data extraction
- `JUSTIFICATIONS[]` - Text alignment (left, right, center)
- `DATATYPES[]` - Data types (text, int, num, float, kcpu, kmem)
- `NULL_VALUES[]` - How to display null values (blank, 0, missing)
- `ZERO_VALUES[]` - How to display zero values (blank, 0, missing)
- `FORMATS[]` - Custom format strings
- `SUMMARIES[]` - Summary calculation types (sum, count, etc.)
- `BREAKS[]` - Whether to insert separators on value changes
- `STRING_LIMITS[]` - Maximum string lengths
- `WRAP_MODES[]` - Text wrapping modes (clip, wrap)
- `WRAP_CHARS[]` - Characters for text wrapping
- `PADDINGS[]` - Padding spaces for columns
- `WIDTHS[]` - Column widths
- `IS_WIDTH_SPECIFIED[]` - Whether width was explicitly set
- `VISIBLES[]` - Whether columns should be displayed

### Sort Configuration Arrays

- `SORT_KEYS[]` - Column keys to sort by
- `SORT_DIRECTIONS[]` - Sort directions (asc, desc)
- `SORT_PRIORITIES[]` - Sort priorities for multi-column sorting

## Functions

### validate_input_files

Validates that layout and data JSON files exist and are not empty.

**Parameters:**

- `layout_file` - Path to the layout JSON file
- `data_file` - Path to the data JSON file

**Returns:**

- `0` if files are valid
- `1` if files are missing or empty

**Example:**

```bash
if validate_input_files "$layout_file" "$data_file"; then
    echo "Files are valid"
else
    echo "Invalid or missing files"
    exit 1
fi
```

### parse_layout_file

Extracts theme, columns, and sort information from a layout JSON file.

**Parameters:**

- `layout_file` - Path to the layout JSON file

**Side Effects:**

- Updates global `THEME_NAME`, `COLUMN_COUNT`, and all configuration arrays
- Sets title and footer configuration variables

**Returns:**

- `0` on success
- `1` if no columns are defined

**Example:**

```bash
if parse_layout_file "$layout_file"; then
    echo "Layout parsed successfully"
    echo "Theme: $THEME_NAME"
    echo "Columns: $COLUMN_COUNT"
else
    echo "Failed to parse layout"
    exit 1
fi
```

### parse_column_config

Processes column configurations from the columns JSON array.

**Parameters:**

- `columns_json` - JSON array containing column configurations

**Side Effects:**

- Populates all column configuration arrays
- Sets initial column widths based on headers and padding
- Validates column configurations

**Features:**

- Automatic key generation from headers if not specified
- Default value assignment for missing configuration options
- Width calculation based on header length and padding
- Support for hidden columns via `visible` property

**Example:**

```bash
columns_json='[{"header":"Name","datatype":"text"},{"header":"Value","datatype":"num"}]'
parse_column_config "$columns_json"
echo "Parsed ${#HEADERS[@]} columns"
```

### validate_column_config

Validates individual column configuration parameters.

**Parameters:**

- `column_index` - Index of the column being validated
- `header` - Column header text
- `justification` - Text alignment setting
- `datatype` - Data type setting
- `summary` - Summary calculation type

**Side Effects:**

- Prints warnings for invalid configurations
- Corrects invalid values to defaults
- Updates configuration arrays with corrected values

**Validation Rules:**

- Headers cannot be empty
- Justification must be left, right, or center
- Data types must be supported by the datatype handlers
- Summary types must be compatible with the data type

**Example:**

```bash
validate_column_config 0 "Name" "left" "text" "count"
```

### parse_sort_config

Processes sort configurations from the sort JSON array.

**Parameters:**

- `sort_json` - JSON array containing sort configurations

**Side Effects:**

- Populates sort configuration arrays
- Validates sort parameters

**Features:**

- Multi-column sorting with priorities
- Direction validation (asc/desc)
- Warning messages for invalid configurations

**Example:**

```bash
sort_json='[{"key":"name","direction":"asc","priority":1}]'
parse_sort_config "$sort_json"
echo "Configured ${#SORT_KEYS[@]} sort keys"
```

## Layout JSON Structure

The layout JSON file defines the complete table configuration:

```json
{
  "theme": "Red",
  "title": "Table Title",
  "title_position": "center",
  "footer": "Table Footer",
  "footer_position": "right",
  "sort": [
    {
      "key": "column_key",
      "direction": "asc",
      "priority": 1
    }
  ],
  "columns": [
    {
      "header": "Column Name",
      "key": "json_field",
      "justification": "left",
      "datatype": "text",
      "null_value": "blank",
      "zero_value": "blank",
      "summary": "count",
      "break": false,
      "string_limit": 0,
      "wrap_mode": "clip",
      "wrap_char": "",
      "padding": 1,
      "width": 0,
      "format": "",
      "visible": true
    }
  ]
}
```

### Configuration Options

#### Theme Options

- `"Red"` - Red color scheme
- `"Blue"` - Blue color scheme

#### Title/Footer Positions

- `"left"` - Left-aligned
- `"right"` - Right-aligned
- `"center"` - Centered
- `"full"` - Full width
- `"none"` - No title/footer (default)

#### Column Properties

- `header` - Column header text (required)
- `key` - JSON field name (auto-generated from header if not specified)
- `justification` - Text alignment: left, right, center
- `datatype` - Data type: text, int, num, float, kcpu, kmem
- `null_value` - Display for null values: blank, 0, missing
- `zero_value` - Display for zero values: blank, 0, missing
- `summary` - Summary type: none, sum, count, min, max, avg, unique
- `break` - Insert separator on value changes: true, false
- `string_limit` - Maximum string length (0 = unlimited)
- `wrap_mode` - Text wrapping: clip, wrap
- `wrap_char` - Character for wrapping
- `padding` - Padding spaces (default: 1)
- `width` - Fixed column width (0 = auto)
- `format` - Custom format string
- `visible` - Display column: true, false

#### Sort Properties

- `key` - Column key to sort by
- `direction` - Sort direction: asc, desc
- `priority` - Sort priority (lower numbers = higher priority)

## Error Handling

The library provides comprehensive error handling:

- **File Validation** - Checks for missing or empty files
- **Configuration Validation** - Validates all configuration parameters
- **Default Values** - Provides sensible defaults for missing options
- **Warning Messages** - Alerts users to invalid configurations
- **Graceful Degradation** - Continues operation with corrected values

## Integration with Tables System

This library is a core component of the tables.sh system:

1. **tables.sh** calls `parse_layout_file()` to initialize configuration
2. **tables_data.sh** uses the configuration arrays to process data
3. **tables_render.sh** uses the configuration for display formatting
4. **tables_datatypes.sh** validates data against configured types
5. **tables_themes.sh** applies the selected theme

## Usage Patterns

### Basic Configuration Parsing

```bash
#!/bin/bash
source "lib/tables_config.sh"

# Parse layout file
if parse_layout_file "layout.json"; then
    echo "Configuration loaded successfully"
    echo "Theme: $THEME_NAME"
    echo "Columns: $COLUMN_COUNT"
    
    # Access column configuration
    for ((i=0; i<COLUMN_COUNT; i++)); do
        echo "Column $i: ${HEADERS[i]} (${DATATYPES[i]})"
    done
else
    echo "Failed to parse configuration"
    exit 1
fi
```

### Custom Validation

```bash
# Validate files before parsing
if validate_input_files "$layout_file" "$data_file"; then
    parse_layout_file "$layout_file"
    echo "Ready to process data"
else
    echo "Invalid input files"
    exit 1
fi
```

## Dependencies

- **jq** - JSON processing
- **tables_datatypes.sh** - Data type validation
- **tables_themes.sh** - Theme definitions

## Best Practices

1. **Always validate files** before parsing
2. **Check return codes** from parsing functions
3. **Use default values** for optional configuration
4. **Provide meaningful error messages** for invalid configurations
5. **Test with various JSON structures** to ensure robustness

## Related Documentation

- [Tables Library](tables.md) - Main tables system documentation
- [Tables Data Library](tables_data.md) - Data processing functions
- [Tables Render Library](tables_render.md) - Rendering functions
- [Tables Datatypes Library](tables_datatypes.md) - Data type handling
- [Tables Themes Library](tables_themes.md) - Theme definitions
