# Tables

A flexible utility for rendering JSON data as ANSI tables in terminal output. Tables.sh provides a powerful way to visualize structured data with customizable formatting, data processing, and display options.

## Overview

Tables.sh converts JSON data into beautifully formatted ANSI tables with the following features:

- Multiple visual themes with colored borders and distinct element colors
- Support for various data types (text, numbers, Kubernetes CPU/memory values)
- Customizable column configurations (headers, alignment, formatting, width)
- Data processing capabilities (sorting, validation, summaries calculation)
- Text wrapping and custom display options for null/zero values
- Title and footer support with flexible positioning
- Thousands separator formatting for numeric data
- Option to hide specific columns using visibility settings

## Usage

```bash
./tables.sh <layout_json_file> <data_json_file> [OPTIONS]
```

### Parameters

- `layout_json_file`: JSON file defining table structure and formatting
- `data_json_file`: JSON file containing the data to display

### Options

- `--debug`: Enable debug output to stderr
- `--version`: Display version information
- `--help`, `-h`: Show help message

### Examples

```bash
# Basic table rendering
./tables.sh layout.json data.json

# With debug output
./tables.sh layout.json data.json --debug

# Show version
./tables.sh --version

# Show help
./tables.sh --help
```

## Layout JSON Structure

The layout file defines how the table should be structured and formatted:

```json
{
  "theme": "Red",
  "title": "Table Title",
  "title_position": "center",
  "footer": "Table Footer", 
  "footer_position": "center",
  "sort": [
    {"key": "column_key", "direction": "asc", "priority": 1}
  ],
  "columns": [
    {
      "header": "COLUMN NAME",
      "key": "json_field_name",
      "justification": "left",
      "datatype": "text",
      "null_value": "blank",
      "zero_value": "blank",
      "summary": "none",
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

### Theme Options

The `theme` field defines the visual appearance of the table:

- `"Red"`: Red borders with distinct colors for different table elements
- `"Blue"`: Blue borders with distinct colors for different table elements

Each theme includes specific colors for:

- **Border**: Table borders and separators
- **Header**: Table title text
- **Caption**: Column header text
- **Summary**: Summary/totals row text
- **Footer**: Footer text
- **Text**: Regular data content

### Title and Footer Support

Tables can include optional titles and footers with flexible positioning:

#### Title Configuration

- `title`: The title text to display above the table
- `title_position`: Position of the title relative to the table
  - `"left"`: Left-aligned title
  - `"right"`: Right-aligned title
  - `"center"`: Centered title
  - `"full"`: Title spans the full table width
  - `"none"`: No title (default)

#### Footer Configuration

- `footer`: The footer text to display below the table
- `footer_position`: Position of the footer relative to the table
  - `"left"`: Left-aligned footer
  - `"right"`: Right-aligned footer
  - `"center"`: Centered footer
  - `"full"`: Footer spans the full table width
  - `"none"`: No footer (default)

### Sort Configuration

The `sort` array allows sorting data by one or more columns:

- `key`: The column key to sort by
- `direction`: Either `"asc"` (ascending) or `"desc"` (descending)
- `priority`: Sort priority when multiple sort keys are defined (lower numbers have higher priority)

### Column Configuration Options

Each column in the `columns` array can have the following properties:

| Property | Description | Default | Options |
|----------|-------------|---------|---------|
| `header` | Column header text | (required) | Any string |
| `key` | JSON field name in the data | Derived from header | Any string |
| `justification` | Text alignment | `"left"` | `"left"`, `"right"`, `"center"` |
| `datatype` | Data type for validation and formatting | `"text"` | `"text"`, `"int"`, `"num"`, `"float"`, `"kcpu"`, `"kmem"` |
| `null_value` | How to display null values | `"blank"` | `"blank"`, `"0"`, `"missing"` |
| `zero_value` | How to display zero values | `"blank"` | `"blank"`, `"0"`, `"missing"` |
| `summary` | Type of summary to calculate | `"none"` | See "Summary Types" section |
| `break` | Insert separator when value changes | `false` | `true`, `false` |
| `string_limit` | Maximum string length | `0` (unlimited) | Any integer |
| `wrap_mode` | How to handle text exceeding limit | `"clip"` | `"clip"`, `"wrap"` |
| `wrap_char` | Character to use for wrapping | `""` | Any character |
| `padding` | Padding spaces on each side | `1` | Any integer |
| `width` | Fixed column width | `0` (auto) | Any integer |
| `format` | Custom format string | `""` | Format string |
| `visible` | Whether to display the column | `true` | `true`, `false` |

#### Visibility Option

The `visible` property allows you to hide specific columns from the table output without removing them from the data processing. Setting `"visible": false` in a column's configuration will exclude that column from the rendered table, ensuring that it does not affect the layout or border calculations. This is useful for including data in the JSON that you might need for sorting or other processing but do not wish to display. In particular, this is useful for adding a custom break value, where the break value itself doesn't need to be visible. For example, in a file listing, a folder column could be present, but not shown, such that extra separator lines could be drawn between folders.

## Supported Data Types

Tables.sh supports the following data types:

### text

Text data with optional wrapping and length limits.

- **Validation**: Any non-null text value
- **Formatting**: Raw text with enhanced clipping and wrapping options
- **Summary Types**: `count`, `unique`

### int / float

Integer or floating-point numbers.

- **Validation**: Any valid number
- **Formatting**: Raw number or custom format string
- **Summary Types**: `sum`, `min`, `max`, `count`, `unique`

### num

Numeric values with thousands separator formatting.

- **Validation**: Any valid number
- **Formatting**: Numbers formatted with thousands separators (e.g., "1,234")
- **Summary Types**: `sum`, `min`, `max`, `count`, `unique`

### kcpu

Kubernetes-style CPU values (e.g., `100m` for 100 millicores).

- **Validation**: Values with `m` suffix or numeric values
- **Formatting**: Always with `m` suffix and thousands separators
- **Summary Types**: `sum`, `count`

### kmem

Kubernetes-style memory values (e.g., `128M`, `1G`, `512Ki`).

- **Validation**: Values with `K`, `M`, `G`, `Ki`, `Mi`, `Gi` suffixes
- **Formatting**: Normalized to `K`, `M`, or `G` format with thousands separators
- **Summary Types**: `sum`, `count`

## Summary Types

Depending on the data type, the following summary calculations are available:

- `sum`: Sum of all values (numeric types, kcpu, kmem)
- `min`: Minimum value (numeric types)
- `max`: Maximum value (numeric types)
- `avg`: Average value (numeric types)
- `count`: Count of non-null values (all types)
- `unique`: Count of unique values (all types)
- `none`: No summary (default)

## Example Tables

### Basic Example

This example renders a table of Kubernetes pod information:

**Layout JSON (layout.json):**

```json
{
  "theme": "Red",
  "columns": [
    {
      "header": "POD",
      "key": "pod",
      "justification": "left",
      "datatype": "text"
    },
    {
      "header": "NAMESPACE",
      "key": "namespace",
      "justification": "center",
      "datatype": "text"
    },
    {
      "header": "CPU USE",
      "key": "cpu_use",
      "justification": "right",
      "datatype": "kcpu",
      "summary": "sum"
    }
  ]
}
```

**Data JSON (data.json):**

```json
[
  {
    "pod": "pod-a",
    "namespace": "default",
    "cpu_use": "100m"
  },
  {
    "pod": "pod-b",
    "namespace": "kube-system",
    "cpu_use": "50m"
  }
]
```

**Command:**

```bash
./tables.sh layout.json data.json
```

**Output:**

```table
╭───────────┬────────────┬─────────╮
│POD        │ NAMESPACE  │  CPU USE│
├───────────┼────────────┼─────────┤
│pod-a      │  default   │     100m│
│pod-b      │kube-system │      50m│
├───────────┼────────────┼─────────┤
│           │            │     150m│
╰───────────┴────────────┴─────────╯
```

### Advanced Example with Title and Footer

This example demonstrates more features, including titles, footers, sorting, and summaries:

**Layout JSON:**

```json
{
  "theme": "Blue",
  "title": "Pod Resource Usage Report",
  "title_position": "center",
  "footer": "Generated by tables.sh",
  "footer_position": "right",
  "sort": [
    {"key": "namespace", "direction": "asc", "priority": 1},
    {"key": "pod", "direction": "asc", "priority": 2}
  ],
  "columns": [
    {
      "header": "POD",
      "key": "pod",
      "justification": "left",
      "datatype": "text",
      "summary": "count"
    },
    {
      "header": "NAMESPACE",
      "key": "namespace",
      "justification": "center",
      "datatype": "text",
      "break": true
    },
    {
      "header": "CPU USE",
      "key": "cpu_use",
      "justification": "right",
      "datatype": "kcpu",
      "null_value": "missing",
      "summary": "sum"
    },
    {
      "header": "MEM USE",
      "key": "mem_use",
      "justification": "right",
      "datatype": "kmem",
      "zero_value": "0",
      "summary": "sum"
    },
    {
      "header": "REQUESTS",
      "key": "requests",
      "justification": "right",
      "datatype": "num",
      "summary": "sum"
    }
  ]
}
```

**Data JSON:**

```json
[
  {
    "pod": "pod-a",
    "namespace": "ns1",
    "cpu_use": "100m",
    "mem_use": "128M",
    "requests": 1500
  },
  {
    "pod": "pod-b",
    "namespace": "ns1",
    "cpu_use": "50m",
    "mem_use": "64M",
    "requests": 750
  },
  {
    "pod": "pod-c",
    "namespace": "ns2",
    "cpu_use": null,
    "mem_use": "256M",
    "requests": 2000
  },
  {
    "pod": "pod-d",
    "namespace": "ns2",
    "cpu_use": "200m",
    "mem_use": null,
    "requests": 1200
  }
]
```

### Title and Footer Positioning Examples

**Short Title (shorter than table width):**

```json
{
  "title": "Pod Stats",
  "title_position": "left"
}
```

**Wide Title (wider than table width):**

```json
{
  "title": "Kubernetes Cluster Pod Resource Utilization Report - Production Environment",
  "title_position": "center"
}
```

**Full Width Title:**

```json
{
  "title": "System Report",
  "title_position": "full"
}
```

### Example with Hidden Column

This example shows how to use the `visible: false` property to hide a column from display while still processing its data:

**Layout JSON:**

```json
{
  "theme": "Red",
  "columns": [
    {
      "header": "ID",
      "key": "id",
      "justification": "right",
      "datatype": "int"
    },
    {
      "header": "Hidden Data",
      "key": "hidden_data",
      "justification": "left",
      "datatype": "text",
      "visible": false
    },
    {
      "header": "Server Name",
      "key": "name",
      "justification": "left",
      "datatype": "text"
    },
    {
      "header": "Status",
      "key": "status",
      "justification": "center",
      "datatype": "text"
    }
  ]
}
```

**Data JSON:**

```json
[
  {
    "id": 1,
    "hidden_data": "secret-info-1",
    "name": "web-server-01",
    "status": "Running"
  },
  {
    "id": 2,
    "hidden_data": "secret-info-2",
    "name": "db-server-01",
    "status": "Running"
  }
]
```

**Output:**

```table
╭────┬───────────────┬──────────╮
│ ID │ Server Name   │  Status  │
├────┼───────────────┼──────────┤
│  1 │ web-server-01 │ Running  │
│  2 │ db-server-01  │ Running  │
╰────┴───────────────┴──────────╯
```

In this example, the "Hidden Data" column is not displayed in the table output, even though the data is present in the JSON file.

## Using in Scripts

You can source tables.sh in your own scripts to use its functions:

```bash
#!/usr/bin/env bash

# Source table library
source ./tables.sh

# Create layout and data files
cat > layout.json << 'EOF'
{
  "theme": "Red",
  "title": "Sample Report",
  "title_position": "center",
  "columns": [
    {
      "header": "NAME",
      "key": "name",
      "datatype": "text"
    },
    {
      "header": "VALUE",
      "key": "value",
      "datatype": "num",
      "summary": "sum"
    }
  ]
}
EOF

cat > data.json << 'EOF'
[
  {"name": "Item A", "value": 1000},
  {"name": "Item B", "value": 2500}
]
EOF

# Draw the table
draw_table layout.json data.json
```

## Tips and Best Practices

1. **Column Width Management**:
   - The script automatically determines column widths based on content
   - Use `width` property to set fixed column widths
   - Use `string_limit` and `wrap_mode` for wide columns

2. **Data Sorting**:
   - Complex sorting can be achieved with multiple sort keys and priorities
   - Use `break: true` to visually group data by important fields

3. **Null and Zero Handling**:
   - Choose appropriate `null_value` and `zero_value` settings for each column
   - Options include showing blank space, "0", or "Missing"

4. **Title and Footer Design**:
   - Use `title_position` and `footer_position` to control alignment
   - Consider table width when designing titles and footers
   - Use `"full"` position for titles/footers that should span the entire table width

5. **Column Visibility**:
   - Use `visible: false` to hide columns that contain data needed for processing (like sorting) but should not be displayed
   - Hidden columns do not affect the table layout or border rendering

6. **Performance**:
   - Very large datasets may cause performance issues
   - Consider limiting data or pre-filtering for large datasets

7. **Testing**:
   - The project includes an extensive suite of test scripts to ensure reliability
   - These tests cover various scenarios and edge cases for robust functionality

8. **Color Compatibility**:
   - The colored output uses ANSI escape sequences which work in most terminals
   - For environments without color support, consider piping through `cat -A` to see escape sequences

## Dependencies

- `jq` (JSON processor)
- `bash` 4.0+
- `awk`
- `sed`

## Version Information

Use `./tables.sh --version` to see the current version and `./tables.sh --help` for usage information.
