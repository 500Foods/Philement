# Tables Data Library Documentation

## Overview

The Tables Data Library (`lib/tables_data.sh`) provides data processing pipeline functionality for the tables.sh system. This library handles data preparation, sorting, row processing, and summary calculations for table rendering.

## Purpose

- Process and validate JSON data for table display
- Apply sorting configurations to data
- Calculate column widths based on content
- Manage summary calculations (sum, count, min, max, avg, unique)
- Handle data formatting and validation
- Support text wrapping and column visibility

## Key Features

- **Data Pipeline Processing** - Complete data preparation workflow
- **Multi-column Sorting** - Sort data by multiple keys with priorities
- **Dynamic Width Calculation** - Adjust column widths based on content
- **Summary Calculations** - Support for various summary types
- **Text Wrapping** - Handle wrapped text with proper width calculations
- **Null/Zero Handling** - Configurable display for null and zero values
- **Column Visibility** - Support for hidden columns

## Global Variables

### Data Storage Arrays

- `ROW_JSONS[]` - Array storing processed row JSON objects

### Summary Storage

- `SUM_SUMMARIES[]` - Sum totals for each column
- `COUNT_SUMMARIES[]` - Count of non-null values for each column
- `MIN_SUMMARIES[]` - Minimum values for each column
- `MAX_SUMMARIES[]` - Maximum values for each column
- `UNIQUE_VALUES[]` - Space-separated unique values for each column
- `AVG_SUMMARIES[]` - Sum totals for average calculations
- `AVG_COUNTS[]` - Count of values for average calculations

### Processing Variables

- `MAX_LINES` - Maximum number of lines in any row (for wrapped text)
- `IS_WIDTH_SPECIFIED[]` - Whether column width was explicitly set

## Functions

### initialize_summaries

Initializes all summary storage arrays to default values.

**Parameters:** None

**Side Effects:**

- Clears all summary associative arrays
- Sets default values for all columns

**Example:**

```bash
initialize_summaries
echo "Summaries initialized for $COLUMN_COUNT columns"
```

### prepare_data

Reads and validates data from a JSON file, ensuring proper format.

**Parameters:**

- `data_file` - Path to the JSON data file

**Returns:**

- Valid JSON data array to stdout

**Features:**

- Validates JSON structure
- Handles malformed data gracefully
- Creates temporary files for processing
- Returns clean JSON array

**Example:**

```bash
data_json=$(prepare_data "data.json")
echo "Prepared data: $data_json"
```

### sort_data

Applies sorting configuration to data based on sort keys and directions.

**Parameters:**

- `data_json` - JSON data array to sort

**Returns:**

- Sorted JSON data array to stdout

**Features:**

- Multi-column sorting with priorities
- Ascending and descending sort directions
- Error handling for invalid sort expressions
- Falls back to original data on sort failure

**Example:**

```bash
sorted_data=$(sort_data "$data_json")
echo "Data sorted successfully"
```

### process_data_rows

Main data processing function that handles row processing, width calculations, and summary updates.

**Parameters:**

- `data_json` - JSON data array to process

**Side Effects:**

- Updates global `MAX_LINES` variable
- Populates `ROW_JSONS[]` array
- Updates column widths in `WIDTHS[]` array
- Calculates all summary values

**Features:**

- Processes each row and column
- Applies data type validation and formatting
- Handles null and zero value display
- Calculates dynamic column widths
- Supports text wrapping with proper width calculation
- Respects explicitly specified column widths
- Updates summary calculations
- Adjusts widths for summary values

**Example:**

```bash
process_data_rows "$sorted_data"
echo "Processed ${#ROW_JSONS[@]} rows"
echo "Maximum lines per row: $MAX_LINES"
```

### update_summaries

Updates summary calculations for a specific column and value.

**Parameters:**

- `column_index` - Index of the column (0-based)
- `value` - The value to include in summaries
- `datatype` - Data type of the column
- `summary_type` - Type of summary to calculate

**Side Effects:**

- Updates appropriate summary arrays based on summary type

**Supported Summary Types:**

- `sum` - Adds numeric values, handles kcpu/kmem units
- `min` - Tracks minimum numeric value
- `max` - Tracks maximum numeric value
- `count` - Counts non-null values
- `unique` - Collects unique values for counting
- `avg` - Accumulates values and counts for average calculation

**Example:**

```bash
update_summaries 0 "100" "int" "sum"
update_summaries 1 "test" "text" "count"
```

## Data Processing Pipeline

The data processing follows this sequence:

1. **Initialize Summaries** - Clear all summary storage
2. **Prepare Data** - Read and validate JSON data
3. **Sort Data** - Apply sorting configuration
4. **Process Rows** - Main processing loop:
   - Validate and format each cell value
   - Handle null/zero value display
   - Calculate column widths (respecting specified widths)
   - Update summary calculations
   - Track maximum lines for wrapped text
5. **Adjust for Summaries** - Ensure columns are wide enough for summary values

## Width Calculation Logic

The library uses sophisticated width calculation:

### Automatic Width Calculation

For columns without specified widths:

- Start with header length + padding
- Expand based on content length
- Handle wrapped text by finding maximum line length
- Adjust for summary values if needed
- Only applies to visible columns

### Specified Width Handling

For columns with explicit width settings:

- Respects the specified width exactly
- Does not adjust based on content
- Applies to both visible and hidden columns

### Text Wrapping Support

When `wrap_mode` is "wrap" and `wrap_char` is specified:

- Splits content by wrap character
- Calculates maximum line length
- Updates line count for row height
- Adjusts column width to fit longest line

## Summary Calculations

### Numeric Summaries (sum, min, max, avg)

- **int/float/num**: Direct numeric operations
- **kcpu**: Handles millicores (e.g., "100m")
- **kmem**: Handles memory units (K, M, G, Ki, Mi, Gi)

### Count Summaries

- Counts non-null values
- Excludes null and empty values

### Unique Summaries

- Collects all unique values
- Final count calculated during rendering

### Average Calculations

- Accumulates sum and count separately
- Calculates average during summary rendering
- Respects data type formatting

## Data Type Integration

The library integrates with the datatypes system:

```bash
# Validate value using datatype handler
value=$("$validate_fn" "$value")

# Format value for display
display_value=$("$format_fn" "$value" "$format" "$string_limit" "$wrap_mode" "$wrap_char")
```

## Null and Zero Value Handling

### Null Values

Based on `null_value` configuration:

- `"blank"` - Display as empty string
- `"0"` - Display as "0"
- `"missing"` - Display as "Missing"

### Zero Values

Based on `zero_value` configuration:

- `"blank"` - Display as empty string
- `"0"` - Display as "0"
- `"missing"` - Display as "Missing"

## Error Handling

The library provides robust error handling:

- **JSON Validation** - Handles malformed JSON gracefully
- **Sort Errors** - Falls back to original data on sort failure
- **Data Type Errors** - Uses validation functions for safety
- **File Operations** - Proper cleanup of temporary files
- **Numeric Operations** - Safe arithmetic with error checking

## Usage Patterns

### Basic Data Processing

```bash
#!/bin/bash
source "lib/tables_config.sh"
source "lib/tables_data.sh"
source "lib/tables_datatypes.sh"

# Initialize
initialize_summaries

# Process data
data_json=$(prepare_data "data.json")
sorted_data=$(sort_data "$data_json")
process_data_rows "$sorted_data"

echo "Processed ${#ROW_JSONS[@]} rows with $MAX_LINES max lines"
```

### Custom Summary Processing

```bash
# Process data and check summaries
process_data_rows "$data_json"

# Access summary values
for ((i=0; i<COLUMN_COUNT; i++)); do
    if [[ "${SUMMARIES[i]}" == "sum" ]]; then
        echo "Column $i sum: ${SUM_SUMMARIES[i]}"
    fi
done
```

### Width Calculation Monitoring

```bash
# Before processing
echo "Initial widths: ${WIDTHS[*]}"

# Process data
process_data_rows "$data_json"

# After processing
echo "Final widths: ${WIDTHS[*]}"
echo "Width specified flags: ${IS_WIDTH_SPECIFIED[*]}"
```

## Performance Considerations

- **Large Datasets** - Processing time increases with row count
- **Complex Sorting** - Multiple sort keys add overhead
- **Text Wrapping** - Wrapped text requires additional processing
- **Summary Calculations** - All summary types are calculated simultaneously

## Dependencies

- **jq** - JSON processing and manipulation
- **awk** - Numeric calculations and formatting
- **tables_config.sh** - Configuration arrays and variables
- **tables_datatypes.sh** - Data validation and formatting functions

## Integration with Tables System

This library is a core component of the tables.sh system:

1. **tables.sh** calls the data processing pipeline
2. **tables_config.sh** provides configuration arrays
3. **tables_datatypes.sh** provides validation and formatting
4. **tables_render.sh** uses the processed data for display

## Best Practices

1. **Initialize summaries** before processing data
2. **Validate data** using prepare_data function
3. **Handle sort errors** gracefully
4. **Monitor width calculations** for layout issues
5. **Test with various data types** and edge cases

## Related Documentation

- [Tables Library](tables.md) - Main tables system documentation
- [Tables Config Library](tables_config.md) - Configuration parsing
- [Tables Datatypes Library](tables_datatypes.md) - Data type handling
- [Tables Render Library](tables_render.md) - Rendering functions
- [Tables Themes Library](tables_themes.md) - Theme definitions
