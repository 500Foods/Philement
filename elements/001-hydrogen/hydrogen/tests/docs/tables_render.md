# Tables Render Library Documentation

## Overview

The Tables Render Library (`lib/tables_render.sh`) provides the complete rendering system for the tables.sh framework. This library handles all visual aspects of table display including borders, headers, data rows, summaries, titles, and footers.

## Purpose

- Render complete table structures with proper formatting
- Handle complex border rendering with titles and footers
- Support text wrapping and multi-line content
- Manage column visibility and dynamic width calculations
- Provide consistent visual styling across all table elements
- Support break separators and summary rows

## Key Features

- **Complete Table Rendering** - Full table display pipeline
- **Border Management** - Complex border rendering with title/footer integration
- **Text Wrapping** - Multi-line content support with proper alignment
- **Column Visibility** - Hide/show columns without affecting layout
- **Break Separators** - Visual grouping with automatic separators
- **Summary Rendering** - Formatted summary rows with calculations
- **Title/Footer Support** - Positioned titles and footers with various alignments

## Core Rendering Functions

### render_table_title

Renders the table title section if a title is specified.

**Parameters:**

- `total_table_width` - The calculated width of the table content

**Features:**

- Evaluates shell commands in title text
- Supports multiple positioning options (left, right, center, full)
- Handles title width calculation and text clipping
- Renders title with proper borders and alignment

**Example:**

```bash
render_table_title 50
```

### render_table_border

Shared function for rendering complex table borders (top or bottom).

**Parameters:**

- `border_type` - Either "top" or "bottom"
- `total_table_width` - Width of the table content
- `element_offset` - Offset position for title/footer
- `element_right_edge` - Right edge position of title/footer
- `element_width` - Width of the title/footer element
- `element_position` - Position type of the element
- `is_element_full` - Whether element spans full width

**Features:**

- Calculates column separator positions
- Handles title/footer integration with borders
- Uses appropriate junction characters for complex layouts
- Supports elements wider than table content

**Border Logic:**

1. Determines maximum width (table or element)
2. Calculates column separator positions
3. Renders character-by-character with proper junctions
4. Handles element boundaries and overlaps

### render_table_top_border

Renders the top border of the table with title integration.

**Features:**

- Calculates total table width from visible columns
- Integrates title positioning and width
- Calls render_table_border with appropriate parameters

**Example:**

```bash
render_table_top_border
```

### render_table_bottom_border

Renders the bottom border of the table with footer integration.

**Features:**

- Calculates total table width from visible columns
- Integrates footer positioning and width
- Calls render_table_border with appropriate parameters

**Example:**

```bash
render_table_bottom_border
```

### render_table_headers

Renders the table headers row with proper formatting.

**Features:**

- Renders only visible columns
- Applies column-specific justification
- Handles header text clipping for width constraints
- Uses theme colors for headers

**Text Clipping:**

- **Left justification** - Clips from the right
- **Right justification** - Clips from the left
- **Center justification** - Clips from both sides

**Example:**

```bash
render_table_headers
```

### render_table_separator

Renders separator lines within the table.

**Parameters:**

- `type` - Type of separator ("middle" or "bottom")

**Features:**

- Uses appropriate junction characters
- Respects column visibility
- Handles variable column widths

**Example:**

```bash
render_table_separator "middle"
```

### render_data_rows

Main function for rendering all data rows with advanced formatting.

**Parameters:**

- `max_lines` - Maximum number of lines per row (for wrapped content)

**Features:**

- Processes break separators between rows
- Handles text wrapping with multiple modes
- Supports multi-line content rendering
- Applies data type-specific formatting
- Respects column visibility and width constraints

**Text Wrapping Modes:**

- **Character-based wrapping** - Uses wrap_char to split content
- **Word wrapping** - Breaks on word boundaries
- **No wrapping** - Clips content to fit

**Break Logic:**

- Compares current row values with previous values
- Inserts separators when break column values change
- Maintains state across all rows

**Example:**

```bash
render_data_rows "$MAX_LINES"
```

### render_table_footer

Renders the table footer section if a footer is specified.

**Parameters:**

- `total_table_width` - The calculated width of the table content

**Features:**

- Evaluates shell commands in footer text
- Supports multiple positioning options
- Handles footer width calculation and text clipping
- Renders footer with proper borders

**Example:**

```bash
render_table_footer 50
```

### render_summaries_row

Renders the summaries row if any summaries are defined.

**Features:**

- Checks for any defined summaries across columns
- Calculates summary values based on type
- Formats summaries according to data types
- Uses theme colors for summary display
- Handles summary text clipping

**Summary Types Supported:**

- **sum** - Formatted totals with proper units
- **min/max** - Minimum/maximum values
- **count** - Count of non-null values
- **unique** - Count of unique values
- **avg** - Calculated averages with proper precision

**Example:**

```bash
if render_summaries_row; then
    echo "Summaries rendered"
else
    echo "No summaries to render"
fi
```

## Rendering Pipeline

The complete table rendering follows this sequence:

1. **Title Rendering** - If title is specified
2. **Top Border** - With title integration
3. **Headers** - Column headers with formatting
4. **Header Separator** - Separator after headers
5. **Data Rows** - All data with breaks and wrapping
6. **Summary Separator** - If summaries exist
7. **Summary Row** - Calculated summaries
8. **Bottom Border** - With footer integration
9. **Footer Rendering** - If footer is specified

## Text Processing Features

### Multi-line Content Support

The rendering system supports multiple approaches to multi-line content:

#### Character-based Wrapping

```bash
# Configuration
"wrap_mode": "wrap"
"wrap_char": "|"

# Input: "Line1|Line2|Line3"
# Output: Three separate lines
```

#### Word Wrapping

```bash
# Configuration
"wrap_mode": "wrap"
"wrap_char": ""

# Input: "This is a long sentence"
# Output: Wrapped at word boundaries
```

#### Content Clipping

```bash
# Configuration
"wrap_mode": "clip"

# Input: "Very long content that exceeds width"
# Output: Clipped based on justification
```

### Justification and Clipping

All text content supports three justification modes:

- **Left** - Content aligned left, clipped from right
- **Right** - Content aligned right, clipped from left  
- **Center** - Content centered, clipped from both sides

## Column Visibility System

The rendering system respects column visibility settings:

```bash
# Hidden columns are completely excluded from rendering
"visible": false

# Visible columns participate in all rendering
"visible": true
```

**Impact on Rendering:**

- Hidden columns don't affect table width calculations
- Border rendering excludes hidden column separators
- Data processing skips hidden columns entirely

## Break Separator System

Break separators provide visual grouping:

```bash
# Configuration
"break": true

# Behavior
# - Compares current row value with previous row
# - Inserts separator line when value changes
# - Maintains state across all break columns
```

## Theme Integration

The rendering system uses theme colors throughout:

- **Border Color** - All border elements
- **Header Color** - Table title text
- **Caption Color** - Column headers
- **Summary Color** - Summary row values
- **Footer Color** - Footer text
- **Text Color** - Regular data content

## Width Calculation Integration

The rendering system works with the width calculation system:

- **Specified Widths** - Respects explicit width settings
- **Dynamic Widths** - Uses calculated widths from content
- **Minimum Widths** - Ensures headers fit properly
- **Summary Widths** - Adjusts for summary content

## Error Handling

The rendering system provides robust error handling:

- **Missing Data** - Handles null and empty values gracefully
- **Width Overflow** - Clips content appropriately
- **Theme Errors** - Falls back to default characters
- **Calculation Errors** - Uses safe defaults

## Performance Considerations

- **Large Tables** - Rendering time increases with row count
- **Complex Wrapping** - Multi-line content adds overhead
- **Many Columns** - Border calculations become more complex
- **Summary Calculations** - All summaries calculated during rendering

## Usage Patterns

### Basic Table Rendering

```bash
#!/bin/bash
source "lib/tables_render.sh"
source "lib/tables_themes.sh"

# Set theme
set_theme "Red"

# Render complete table
render_table_title "$total_width"
render_table_top_border
render_table_headers
render_table_separator "middle"
render_data_rows "$MAX_LINES"
if render_summaries_row; then
    render_table_bottom_border
else
    render_table_separator "bottom"
fi
render_table_footer "$total_width"
```

### Custom Rendering

```bash
# Render only specific parts
render_table_headers
render_data_rows 1  # Single line only
```

## Dependencies

- **tables_config.sh** - Configuration arrays and variables
- **tables_data.sh** - Processed data and summaries
- **tables_datatypes.sh** - Data formatting functions
- **tables_themes.sh** - Theme definitions and colors
- **jq** - JSON processing for data extraction

## Integration with Tables System

This library is the final component of the tables.sh system:

1. **tables_config.sh** provides configuration
2. **tables_data.sh** processes and prepares data
3. **tables_datatypes.sh** formats individual values
4. **tables_themes.sh** provides visual styling
5. **tables_render.sh** creates the final visual output

## Best Practices

1. **Set themes** before rendering
2. **Process data** before calling render functions
3. **Handle large datasets** with appropriate limits
4. **Test wrapping** with various content types
5. **Validate output** in different terminal environments

## Related Documentation

- [Tables Library](tables.md) - Main tables system documentation
- [Tables Config Library](tables_config.md) - Configuration parsing
- [Tables Data Library](tables_data.md) - Data processing functions
- [Tables Datatypes Library](tables_datatypes.md) - Data type handling
- [Tables Themes Library](tables_themes.md) - Theme definitions
