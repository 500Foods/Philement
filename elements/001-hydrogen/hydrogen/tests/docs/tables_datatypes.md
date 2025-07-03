# Tables Datatypes Library Documentation

## Overview

The Tables Datatypes Library (`lib/tables_datatypes.sh`) provides a comprehensive data type system for the tables.sh framework. This library handles validation, formatting, and summary functions for different data types used in table rendering.

## Purpose

- Define and manage supported data types for table columns
- Provide validation functions to ensure data integrity
- Handle formatting for display purposes
- Support specialized data types (Kubernetes CPU/memory units)
- Enable data type-specific summary calculations
- Maintain a registry of data type handlers

## Key Features

- **Data Type Registry** - Centralized mapping of data types to handler functions
- **Validation System** - Ensures data matches expected formats
- **Formatting Pipeline** - Converts data for display with proper formatting
- **Specialized Types** - Support for Kubernetes-style CPU and memory units
- **Summary Integration** - Defines which summary types are valid for each data type
- **Text Processing** - Advanced text handling with wrapping and truncation

## Data Type Registry

The `DATATYPE_HANDLERS` associative array maps data types to their handler functions:

```bash
declare -A DATATYPE_HANDLERS=(
    [text_validate]="validate_text"
    [text_format]="format_text"
    [text_summary_types]="count unique"
    [int_validate]="validate_number"
    [int_format]="format_number"
    [int_summary_types]="sum min max avg count unique"
    [num_validate]="validate_number"
    [num_format]="format_num"
    [num_summary_types]="sum min max avg count unique"
    [float_validate]="validate_number"
    [float_format]="format_number"
    [float_summary_types]="sum min max avg count unique"
    [kcpu_validate]="validate_kcpu"
    [kcpu_format]="format_kcpu"
    [kcpu_summary_types]="sum min max avg count unique"
    [kmem_validate]="validate_kmem"
    [kmem_format]="format_kmem"
    [kmem_summary_types]="sum min max avg count unique"
)
```

## Supported Data Types

### text

General text data with advanced formatting options.

**Validation:**

- Accepts any non-null text value
- Returns empty string for null values

**Formatting:**

- Supports string length limits
- Handles text wrapping with custom characters
- Supports text truncation with justification (left, right, center)

**Summary Types:** `count`, `unique`

### int

Integer numeric values.

**Validation:**

- Accepts whole numbers and zero
- Accepts null values
- Rejects non-numeric input

**Formatting:**

- Raw numeric display
- Custom format string support

**Summary Types:** `sum`, `min`, `max`, `avg`, `count`, `unique`

### float

Floating-point numeric values.

**Validation:**

- Accepts decimal numbers
- Accepts whole numbers and zero
- Accepts null values

**Formatting:**

- Raw numeric display
- Custom format string support

**Summary Types:** `sum`, `min`, `max`, `avg`, `count`, `unique`

### num

Numeric values with thousands separator formatting.

**Validation:**

- Same as int/float validation
- Accepts decimal and whole numbers

**Formatting:**

- Automatic thousands separator (e.g., "1,234")
- Custom format string support

**Summary Types:** `sum`, `min`, `max`, `avg`, `count`, `unique`

### kcpu

Kubernetes-style CPU values (millicores).

**Validation:**

- Accepts values with 'm' suffix (e.g., "100m")
- Accepts plain numeric values (converted to millicores)
- Accepts zero and null values

**Formatting:**

- Always displays with 'm' suffix
- Thousands separator for large values
- Converts plain numbers to millicores

**Summary Types:** `sum`, `min`, `max`, `avg`, `count`, `unique`

### kmem

Kubernetes-style memory values.

**Validation:**

- Accepts values with K, M, G suffixes
- Accepts values with Ki, Mi, Gi suffixes (binary units)
- Accepts zero and null values

**Formatting:**

- Normalizes binary units to decimal (Mi → M, Gi → G, Ki → K)
- Thousands separator for large values
- Preserves original unit type

**Summary Types:** `sum`, `min`, `max`, `avg`, `count`, `unique`

## Validation Functions

### validate_text

Validates text data, accepting any non-null value.

**Parameters:**

- `value` - The value to validate

**Returns:**

- The original value if not null
- Empty string if null

**Example:**

```bash
result=$(validate_text "Hello World")  # Returns: "Hello World"
result=$(validate_text "null")         # Returns: ""
```

### validate_number

Validates numeric data (int, float, num types).

**Parameters:**

- `value` - The value to validate

**Returns:**

- The original value if numeric
- Empty string if invalid

**Example:**

```bash
result=$(validate_number "123")     # Returns: "123"
result=$(validate_number "123.45") # Returns: "123.45"
result=$(validate_number "abc")     # Returns: ""
```

### validate_kcpu

Validates Kubernetes CPU values.

**Parameters:**

- `value` - The value to validate

**Returns:**

- The original value if valid kcpu format
- Empty string if invalid

**Example:**

```bash
result=$(validate_kcpu "100m")  # Returns: "100m"
result=$(validate_kcpu "1.5")   # Returns: "1.5"
result=$(validate_kcpu "abc")   # Returns: ""
```

### validate_kmem

Validates Kubernetes memory values.

**Parameters:**

- `value` - The value to validate

**Returns:**

- The original value if valid kmem format
- Empty string if invalid

**Example:**

```bash
result=$(validate_kmem "128M")   # Returns: "128M"
result=$(validate_kmem "1Gi")    # Returns: "1Gi"
result=$(validate_kmem "abc")    # Returns: ""
```

## Formatting Functions

### format_text

Formats text data with advanced options.

**Parameters:**

- `value` - The text value to format
- `format` - Custom format string (unused for text)
- `string_limit` - Maximum string length (0 = unlimited)
- `wrap_mode` - Text wrapping mode ("clip" or "wrap")
- `wrap_char` - Character to use for wrapping
- `justification` - Text alignment for truncation

**Returns:**

- Formatted text value

**Features:**

- String length limiting with justification
- Text wrapping with custom characters
- Truncation with left/right/center alignment

**Example:**

```bash
result=$(format_text "Hello World" "" 5 "clip" "" "left")    # Returns: "Hello"
result=$(format_text "Hello World" "" 5 "clip" "" "right")   # Returns: "World"
```

### format_number

Formats basic numeric values.

**Parameters:**

- `value` - The numeric value to format
- `format` - Custom format string

**Returns:**

- Formatted numeric value

**Example:**

```bash
result=$(format_number "123" "")      # Returns: "123"
result=$(format_number "null" "")     # Returns: ""
```

### format_num

Formats numeric values with thousands separators.

**Parameters:**

- `value` - The numeric value to format
- `format` - Custom format string

**Returns:**

- Formatted numeric value with thousands separators

**Example:**

```bash
result=$(format_num "1234" "")        # Returns: "1,234"
result=$(format_num "1234567" "")     # Returns: "1,234,567"
```

### format_kcpu

Formats Kubernetes CPU values.

**Parameters:**

- `value` - The CPU value to format
- `format` - Custom format string (unused)

**Returns:**

- Formatted CPU value with 'm' suffix and thousands separators

**Features:**

- Converts plain numbers to millicores
- Adds thousands separators for readability
- Always includes 'm' suffix

**Example:**

```bash
result=$(format_kcpu "100m" "")       # Returns: "100m"
result=$(format_kcpu "1.5" "")        # Returns: "1,500m"
result=$(format_kcpu "2000m" "")      # Returns: "2,000m"
```

### format_kmem

Formats Kubernetes memory values.

**Parameters:**

- `value` - The memory value to format
- `format` - Custom format string (unused)

**Returns:**

- Formatted memory value with normalized units and thousands separators

**Features:**

- Normalizes binary units (Mi → M, Gi → G, Ki → K)
- Adds thousands separators for readability
- Preserves unit type

**Example:**

```bash
result=$(format_kmem "128M" "")       # Returns: "128M"
result=$(format_kmem "1Gi" "")        # Returns: "1G"
result=$(format_kmem "2048Mi" "")     # Returns: "2,048M"
```

### format_display_value

High-level formatting function that handles the complete formatting pipeline.

**Parameters:**

- `value` - The value to format
- `null_value` - How to display null values
- `zero_value` - How to display zero values
- `datatype` - The data type
- `format` - Custom format string
- `string_limit` - String length limit
- `wrap_mode` - Text wrapping mode
- `wrap_char` - Wrapping character
- `justification` - Text alignment

**Returns:**

- Fully formatted display value

**Features:**

- Complete validation and formatting pipeline
- Null and zero value handling
- Data type-specific formatting
- Advanced text processing

**Example:**

```bash
result=$(format_display_value "null" "missing" "0" "text" "" 0 "clip" "" "left")
# Returns: "Missing"

result=$(format_display_value "1234" "blank" "blank" "num" "" 0 "clip" "" "left")
# Returns: "1,234"
```

## Data Type Integration

The data type system integrates with the tables framework through:

### Configuration Integration

```bash
# Access validation function
validate_fn="${DATATYPE_HANDLERS[${datatype}_validate]}"

# Access formatting function
format_fn="${DATATYPE_HANDLERS[${datatype}_format]}"

# Access summary types
summary_types="${DATATYPE_HANDLERS[${datatype}_summary_types]}"
```

### Processing Pipeline

1. **Validation** - Ensure data matches expected format
2. **Formatting** - Apply display formatting
3. **Null/Zero Handling** - Apply special value handling
4. **Summary Calculation** - Update appropriate summaries

## Summary Type Compatibility

Each data type supports specific summary calculations:

| Data Type | sum | min | max | avg | count | unique |
|-----------|-----|-----|-----|-----|-------|--------|
| text      | ❌  | ❌  | ❌  | ❌  | ✅    | ✅     |
| int       | ✅  | ✅  | ✅  | ✅  | ✅    | ✅     |
| float     | ✅  | ✅  | ✅  | ✅  | ✅    | ✅     |
| num       | ✅  | ✅  | ✅  | ✅  | ✅    | ✅     |
| kcpu      | ✅  | ✅  | ✅  | ✅  | ✅    | ✅     |
| kmem      | ✅  | ✅  | ✅  | ✅  | ✅    | ✅     |

## Advanced Text Processing

### String Limiting

Text can be limited by length with different truncation strategies:

- **Left truncation** - Keep rightmost characters
- **Right truncation** - Keep leftmost characters (default)
- **Center truncation** - Keep middle characters

### Text Wrapping

Text can be wrapped using custom characters:

- Split text by wrap character
- Display each part on separate lines
- Calculate proper column width for wrapped content

## Error Handling

The data type system provides robust error handling:

- **Invalid Data** - Returns empty strings for invalid input
- **Null Values** - Handles null values according to configuration
- **Zero Values** - Handles zero values according to configuration
- **Format Errors** - Falls back to raw values on format failures

## Usage Patterns

### Basic Validation

```bash
#!/bin/bash
source "lib/tables_datatypes.sh"

# Validate different data types
text_result=$(validate_text "Hello")
num_result=$(validate_number "123")
cpu_result=$(validate_kcpu "100m")

echo "Text: $text_result"
echo "Number: $num_result"
echo "CPU: $cpu_result"
```

### Formatting Pipeline

```bash
# Complete formatting with all options
display_value=$(format_display_value \
    "$raw_value" \
    "missing" \
    "0" \
    "num" \
    "" \
    20 \
    "clip" \
    "" \
    "left")

echo "Formatted: $display_value"
```

### Dynamic Handler Access

```bash
# Access handlers dynamically
datatype="kcpu"
validate_fn="${DATATYPE_HANDLERS[${datatype}_validate]}"
format_fn="${DATATYPE_HANDLERS[${datatype}_format]}"

# Use handlers
validated_value=$("$validate_fn" "$raw_value")
formatted_value=$("$format_fn" "$validated_value" "")
```

## Performance Considerations

- **Validation Overhead** - Each value is validated before formatting
- **Regex Matching** - Complex patterns may impact performance
- **String Operations** - Text processing can be expensive for large datasets
- **Function Calls** - Dynamic function resolution adds overhead

## Dependencies

- **awk** - Numeric formatting and calculations
- **bash** - Pattern matching and string operations
- **printf** - Formatted output

## Integration with Tables System

This library is a foundational component of the tables.sh system:

1. **tables_config.sh** validates data types during configuration
2. **tables_data.sh** uses validation and formatting functions
3. **tables_render.sh** relies on formatted values for display
4. **tables.sh** coordinates the entire pipeline

## Best Practices

1. **Always validate** data before formatting
2. **Use appropriate data types** for your data
3. **Test edge cases** like null and zero values
4. **Consider performance** for large datasets
5. **Extend carefully** when adding new data types

## Related Documentation

- [Tables Library](tables.md) - Main tables system documentation
- [Tables Config Library](tables_config.md) - Configuration parsing
- [Tables Data Library](tables_data.md) - Data processing functions
- [Tables Render Library](tables_render.md) - Rendering functions
- [Tables Themes Library](tables_themes.md) - Theme definitions
