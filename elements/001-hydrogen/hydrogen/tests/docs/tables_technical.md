# Tables Technical Reference

This document serves as an in-depth technical reference for the `tables.sh` system, detailing the internal workings of configuration, data processing, data types, and rendering. It provides comprehensive insights into the framework's capabilities with code snippets and detailed feature breakdowns to aid in understanding the full feature set.

## Table of Contents

- [Configuration Library](#configuration-library)
- [Data Processing Library](#data-processing-library)
- [Datatypes Library](#datatypes-library)
- [Rendering Library](#rendering-library)

## Configuration Library

### Configuration Library Overview

The Tables Config Library (`lib/tables_config.sh`) is a core component of the `tables.sh` system, responsible for parsing and validating layout JSON files. It manages the configuration of columns, sorting, themes, and other table properties, establishing the foundation for data processing and rendering.

### Configuration Library Purpose

- Parse layout JSON files to define table structure, including columns, sorting rules, and visual themes.
- Validate configuration parameters to ensure correctness and consistency.
- Maintain global state in arrays for access by other system components.
- Support advanced table customization through title and footer positioning options.

### Configuration Library Key Features

- **JSON Parsing**: Extracts theme settings, column definitions, sort criteria, and title/footer configurations from layout JSON.
- **Column Configuration**: Defines headers, data types, formatting rules, visibility, and summary options for each column.
- **Sort Configuration**: Supports multi-column sorting with configurable priorities and directions (ascending/descending).
- **Validation**: Ensures configuration integrity by validating parameters, correcting invalid entries, and issuing warnings.
- **Title/Footer Support**: Allows customization of table titles and footers with multiple positioning options for enhanced presentation.

### Configuration Library Global Variables

- **Title and Footer Configuration**:
  - `TABLE_TITLE`: Stores the title text of the table.
  - `TITLE_WIDTH`: Calculated width for title display, based on content and positioning.
  - `TITLE_POSITION`: Defines title alignment; options are `left`, `right`, `center`, `full`, or `none`.
  - `TABLE_FOOTER`: Stores the footer text of the table.
  - `FOOTER_WIDTH`: Calculated width for footer display.
  - `FOOTER_POSITION`: Defines footer alignment; options mirror title positioning.
- **Column Configuration Arrays**:
  - `HEADERS[]`: Array of column header texts.
  - `KEYS[]`: JSON field names used for data extraction from input data.
  - `JUSTIFICATIONS[]`: Text alignment for each column (`left`, `right`, `center`).
  - `DATATYPES[]`: Data type for each column (`text`, `int`, `num`, `float`, `kcpu`, `kmem`).
  - `NULL_VALUES[]`: Display rule for null values (`blank`, `0`, `missing`).
  - `ZERO_VALUES[]`: Display rule for zero values (`blank`, `0`, `missing`).
  - `FORMATS[]`: Custom format strings for data display.
  - `SUMMARIES[]`: Summary calculation type for each column (`sum`, `count`, `min`, `max`, `avg`, `unique`).
  - `BREAKS[]`: Boolean flags to insert separators when column values change.
  - `STRING_LIMITS[]`: Maximum string lengths for text data (0 for unlimited).
  - `WRAP_MODES[]`: Text wrapping behavior (`clip` or `wrap`).
  - `WRAP_CHARS[]`: Characters used as wrap delimiters.
  - `PADDINGS[]`: Padding spaces added around column content.
  - `WIDTHS[]`: Column widths, either specified or dynamically calculated.
  - `IS_WIDTH_SPECIFIED[]`: Boolean flags indicating if width was explicitly set.
  - `VISIBLES[]`: Boolean flags controlling column visibility in rendering.
- **Sort Configuration Arrays**:
  - `SORT_KEYS[]`: Column keys used for sorting data.
  - `SORT_DIRECTIONS[]`: Sort directions for each key (`asc` or `desc`).
  - `SORT_PRIORITIES[]`: Priority levels for multi-column sorting (lower numbers = higher priority).

### Configuration Library Core Functions

- **validate_input_files**: Ensures layout and data JSON files exist and are non-empty. Returns 0 for valid files, 1 for invalid or missing files.

  ```bash
  if validate_input_files "$layout_file" "$data_file"; then
      echo "Input files are valid, proceeding with processing."
  else
      echo "Error: Invalid or missing input files."
      exit 1
  fi
  ```

- **parse_layout_file**: Parses the layout JSON file to extract theme, columns, sort information, and title/footer settings into global variables. Returns 0 on success, 1 if no columns are defined.

  ```bash
  if parse_layout_file "layout.json"; then
      echo "Layout configuration parsed. Theme: $THEME_NAME, Columns: $COLUMN_COUNT"
  else
      echo "Error: No columns defined in layout."
      exit 1
  fi
  ```

- **parse_column_config**: Processes column configurations from a JSON array into the respective global arrays, initializing widths based on header length and padding.
- **validate_column_config**: Validates settings for each column (header, justification, datatype, summary), correcting invalid values to defaults and printing warnings for issues like empty headers or unsupported data types.

  ```bash
  validate_column_config 0 "Name" "left" "text" "count"
  # Output warning if justification is invalid, e.g., "invalid_just", and correct to "left"
  ```

- **parse_sort_config**: Processes sort configurations from JSON, validating direction (`asc` or `desc`) and priority for multi-column sorting, ensuring proper data ordering setup.

### Configuration Library Layout JSON Structure

The layout JSON file defines the complete table configuration, serving as the blueprint for rendering. Below is a detailed example structure:

```json
{
  "theme": "Red",                       // Theme name for visual styling ("Red" or "Blue")
  "title": "Table Title",               // Optional table title text
  "title_position": "center",           // Title alignment: "left", "right", "center", "full", "none"
  "footer": "Table Footer",             // Optional table footer text
  "footer_position": "right",           // Footer alignment: same options as title
  "sort": [                             // Array of sort criteria for data ordering
    {
      "key": "column_key",               // Column key to sort by
      "direction": "asc",                // Sort direction: "asc" (ascending) or "desc" (descending)
      "priority": 1                      // Sort priority (lower number = higher priority)
    }
  ],
  "columns": [                          // Array of column definitions
    {
      "header": "Column Name",           // Required: Column header text
      "key": "json_field",               // Optional: JSON field for data; auto-generated from header if omitted
      "justification": "left",           // Text alignment: "left", "right", "center"
      "datatype": "text",                // Data type: "text", "int", "num", "float", "kcpu", "kmem"
      "null_value": "blank",             // Display for null: "blank", "0", "missing"
      "zero_value": "blank",             // Display for zero: "blank", "0", "missing"
      "summary": "count",                // Summary type: "none", "sum", "count", "min", "max", "avg", "unique"
      "break": false,                    // Insert separator on value change: true/false
      "string_limit": 0,                 // Max string length (0 = unlimited)
      "wrap_mode": "clip",               // Text wrapping: "clip" (truncate) or "wrap" (split lines)
      "wrap_char": "",                   // Character for wrapping (if wrap_mode is "wrap")
      "padding": 1,                      // Padding spaces around content
      "width": 0,                        // Fixed width (0 = auto-calculated)
      "format": "",                      // Custom format string for display
      "visible": true                    // Show/hide column in rendering: true/false
    }
  ]
}
```

### Configuration Library Error Handling

- **File Validation**: Checks for missing or empty input files before processing.
- **Configuration Validation**: Validates all parameters, such as ensuring headers are non-empty and data types are supported, with warnings for corrections.
- **Default Values**: Provides sensible defaults for missing or invalid configuration options to maintain system stability.
- **Graceful Degradation**: Continues operation with corrected values to prevent crashes due to misconfiguration.

### Configuration Library Integration with Tables System

This library is integral to the `tables.sh` workflow:

1. **tables.sh**: Invokes `parse_layout_file()` to initialize configuration.
2. **tables_data.sh**: Uses configuration arrays for data processing.
3. **tables_render.sh**: Relies on configuration for formatting and display.
4. **tables_datatypes.sh**: Validates data against configured types.
5. **tables_themes.sh**: Applies the selected theme based on configuration.

### Configuration Library Dependencies

- **jq**: Essential for JSON parsing and data extraction.
- **tables_datatypes.sh**: Provides data type validation logic.
- **tables_themes.sh**: Supplies theme definitions for visual styling.

## Data Processing Library

### Data Processing Library Overview

The Tables Data Library (`lib/tables_data.sh`) orchestrates the data processing pipeline for `tables.sh`, managing the preparation, sorting, width calculation, and summary computation of input data for table rendering.

### Data Processing Library Purpose

- Process and validate JSON data to prepare it for table display.
- Apply multi-column sorting based on user-defined configurations.
- Calculate dynamic column widths to fit content and layout constraints.
- Compute various summary statistics (sum, count, min, max, avg, unique) for columns.
- Handle advanced features like text wrapping, null/zero value display, and column visibility during processing.

### Data Processing Library Key Features

- **Data Pipeline**: Manages the complete workflow from raw JSON input to processed rows ready for rendering.
- **Multi-Column Sorting**: Supports sorting by multiple keys with configurable priorities and directions for precise data ordering.
- **Dynamic Width Calculation**: Adjusts column widths based on content length, respecting explicitly specified limits and accommodating summaries.
- **Summary Calculations**: Provides comprehensive statistical summaries tailored to data types, enhancing data analysis capabilities.
- **Text Wrapping and Visibility**: Processes text wrapping for multi-line content and respects column visibility settings to optimize data presentation.

### Data Processing Library Global Variables

- **Data Storage**:
  - `ROW_JSONS[]`: Array storing processed row data as JSON objects, ready for rendering.
- **Summary Storage**:
  - `SUM_SUMMARIES[]`: Accumulates sum totals for each column, supporting numeric and Kubernetes unit calculations.
  - `COUNT_SUMMARIES[]`: Tracks the count of non-null values per column for summary statistics.
  - `MIN_SUMMARIES[]`: Records minimum values per column for numeric data types.
  - `MAX_SUMMARIES[]`: Records maximum values per column for numeric data types.
  - `UNIQUE_VALUES[]`: Collects space-separated unique values per column for counting distinct entries.
  - `AVG_SUMMARIES[]`: Accumulates sums for average calculations per column.
  - `AVG_COUNTS[]`: Tracks counts of values for average calculations per column.
- **Processing Variables**:
  - `MAX_LINES`: Stores the maximum number of lines in any row, used for rendering multi-line wrapped text.
  - `IS_WIDTH_SPECIFIED[]`: Boolean flags indicating whether a column width was explicitly set in configuration.

### Data Processing Library Core Functions

- **initialize_summaries**: Resets all summary storage arrays to default values, ensuring a clean slate for new data processing cycles.

  ```bash
  initialize_summaries
  echo "Summary arrays reset for $COLUMN_COUNT columns."
  ```

- **prepare_data**: Reads JSON data from a file, validates its structure, and ensures it is ready for processing. Outputs a clean JSON data array.

  ```bash
  data_json=$(prepare_data "data.json")
  echo "Data prepared with $(jq '. | length' <<< "$data_json") rows."
  ```

- **sort_data**: Applies sorting to the data based on configured keys, directions, and priorities. Returns the sorted JSON data array, with a fallback to the original data if sorting fails.

  ```bash
  sorted_data=$(sort_data "$data_json")
  echo "Data sorted by ${SORT_KEYS[0]} (${SORT_DIRECTIONS[0]})."
  ```

- **process_data_rows**: The primary processing function; updates `MAX_LINES` for wrapped content, populates `ROW_JSONS[]` with processed data, adjusts `WIDTHS[]` dynamically, and calculates all summaries.

  ```bash
  process_data_rows "$sorted_data"
  echo "Processed ${#ROW_JSONS[@]} rows, max lines per row: $MAX_LINES."
  ```

- **update_summaries**: Updates summary calculations for a specific column and value, based on the data type and configured summary type (sum, min, max, count, unique, avg).

  ```bash
  update_summaries 0 "100" "int" "sum"
  echo "Updated sum for column 0: ${SUM_SUMMARIES[0]}"
  ```

### Data Processing Library Pipeline

The pipeline transforms raw data into a render-ready format through a structured sequence:

1. **Initialize Summaries**: Clears all summary storage arrays to prevent data carryover from previous operations.
2. **Prepare Data**: Reads and validates JSON data from the input file, ensuring structural integrity and correct format.
3. **Sort Data**: Applies sorting if configured, ordering data by specified columns with priority and direction considerations.
4. **Process Rows**: Iterates through each data row to:
   - Validate and format cell values using data type-specific handlers.
   - Apply null and zero value display rules as per configuration.
   - Calculate dynamic column widths unless explicitly specified, considering content length.
   - Update summary statistics for each column based on data type and summary type.
   - Track the maximum number of lines per row for text wrapping in rendering.
5. **Adjust for Summaries**: Ensures column widths are sufficient to display summary values, preventing truncation of important statistics.

### Data Processing Library Width Calculation Logic

- **Automatic Width Calculation**: For columns without specified widths, initializes width as header length plus padding, expands based on content length, accounts for wrapped text by calculating maximum line length, and adjusts for summary values if wider (applies only to visible columns).
- **Specified Width Handling**: For columns with explicit width settings, uses the specified value without content-based adjustments, applicable to both visible and hidden columns.
- **Text Wrapping Support**: When `wrap_mode` is set to "wrap" and a `wrap_char` is provided, splits content by the wrap character, calculates the maximum line length, updates row height (`MAX_LINES`), and adjusts column width to fit the longest line.

### Data Processing Library Summary Calculations

- **Numeric Summaries (sum, min, max, avg)**:
  - For `int`, `float`, `num`: Performs direct numeric operations using `awk` for precision.
  - For `kcpu`: Handles millicores (e.g., "100m"), accumulating values without the suffix.
  - For `kmem`: Manages memory units (K, M, G, Ki, Mi, Gi), converting as needed for consistent summation.
- **Count Summaries**: Counts non-null values per column, excluding empty or null entries to ensure accurate tallies.
- **Unique Summaries**: Collects all unique values per column as a space-separated list, with the final count calculated during rendering for efficiency.
- **Average Calculations**: Accumulates sum and count separately per column, computing the final average during rendering to respect data type formatting and precision.

### Data Processing Library Null and Zero Value Handling

- **Null Values**: Based on `null_value` configuration:
  - `"blank"`: Displays as an empty string.
  - `"0"`: Displays as "0".
  - `"missing"`: Displays as "Missing".
- **Zero Values**: Based on `zero_value` configuration:
  - `"blank"`: Displays as an empty string.
  - `"0"`: Displays as "0".
  - `"missing"`: Displays as "Missing".

### Data Processing Library Error Handling

- **JSON Validation**: Gracefully handles malformed JSON input by providing fallbacks or error messages to prevent crashes.
- **Sort Errors**: Falls back to original unsorted data if sorting fails due to invalid expressions or data mismatches.
- **Data Type Errors**: Uses validation functions to safely handle incorrect data formats, returning empty strings for invalid values.
- **File Operations**: Ensures proper cleanup of temporary files created during processing to avoid clutter or conflicts.
- **Numeric Operations**: Implements safe arithmetic with error checking to prevent issues like overflow or division by zero.

### Data Processing Library Integration with Tables System

This library is central to the `tables.sh` workflow:

1. **tables.sh**: Initiates the data processing pipeline.
2. **tables_config.sh**: Supplies configuration arrays for processing rules.
3. **tables_datatypes.sh**: Provides validation and formatting functions for data consistency.
4. **tables_render.sh**: Consumes processed data for final visual output.

### Data Processing Library Dependencies

- **jq**: For JSON processing and manipulation of input data.
- **awk**: For precise numeric calculations and formatting in summaries.
- **tables_config.sh**: Supplies configuration arrays and variables for processing logic.
- **tables_datatypes.sh**: Provides essential data validation and formatting functions.

## Datatypes Library

### Datatypes Library Overview

The Tables Datatypes Library (`lib/tables_datatypes.sh`) establishes a robust data type system for the `tables.sh` framework, managing validation, formatting, and summary compatibility for diverse data types used in table rendering.

### Datatypes Library Purpose

- Define and manage a set of supported data types for table columns to ensure consistent handling.
- Provide validation functions to maintain data integrity across processing stages.
- Format data for display with type-specific rules to enhance readability.
- Support specialized data types for Kubernetes CPU and memory units, addressing domain-specific needs.
- Enable data type-specific summary calculations for meaningful statistical insights.

### Datatypes Library Key Features

- **Data Type Registry**: Central mapping of data types to their respective handler functions for validation, formatting, and summary support.
- **Validation System**: Ensures input data matches expected formats, rejecting invalid entries to maintain data quality.
- **Formatting Pipeline**: Converts raw data into display-ready formats with type-specific styling and formatting rules.
- **Specialized Types**: Includes dedicated support for Kubernetes-style CPU (millicores) and memory units (K, M, G, etc.).
- **Summary Integration**: Defines compatible summary types per data type, enabling tailored statistical outputs.
- **Text Processing**: Offers advanced text handling with options for wrapping and truncation to fit table constraints.

### Datatypes Library Data Type Registry

The `DATATYPE_HANDLERS` associative array maps each data type to its handler functions for validation, formatting, and summary compatibility:

```bash
declare -A DATATYPE_HANDLERS=(
    [text_validate]="validate_text"       [text_format]="format_text"         [text_summary_types]="count unique"
    [int_validate]="validate_number"      [int_format]="format_number"        [int_summary_types]="sum min max avg count unique"
    [float_validate]="validate_number"    [float_format]="format_number"      [float_summary_types]="sum min max avg count unique"
    [num_validate]="validate_number"      [num_format]="format_num"           [num_summary_types]="sum min max avg count unique"
    [kcpu_validate]="validate_kcpu"       [kcpu_format]="format_kcpu"         [kcpu_summary_types]="sum min max avg count unique"
    [kmem_validate]="validate_kmem"       [kmem_format]="format_kmem"         [kmem_summary_types]="sum min max avg count unique"
)
```

### Datatypes Library Supported Data Types

- **text**: General-purpose text data with flexible formatting.
  - **Validation**: Accepts any non-null text value, returns an empty string for null values to prevent display issues.
  - **Formatting**: Supports string length limits (truncation if exceeded), text wrapping with custom delimiter characters, and justification for truncation (`left`, `right`, `center`).
  - **Summaries**: Supports `count` (total non-null entries) and `unique` (count of distinct values).
- **int**: Integer numeric values for whole number data.
  - **Validation**: Accepts whole numbers and zero, accepts null values, rejects non-numeric input.
  - **Formatting**: Displays raw numeric values, supports custom format strings for specialized output (e.g., padding with zeros).
  - **Summaries**: Supports `sum`, `min`, `max`, `avg`, `count`, `unique` for comprehensive statistical analysis.
- **float**: Floating-point numeric values for decimal data.
  - **Validation**: Accepts decimal numbers, whole numbers, zero, and null values.
  - **Formatting**: Displays raw numeric values, allows custom format strings for precision control.
  - **Summaries**: Supports `sum`, `min`, `max`, `avg`, `count`, `unique` for full statistical capabilities.
- **num**: Numeric values with thousands separator formatting for readability.
  - **Validation**: Matches `int` and `float` validation, accepting decimal and whole numbers.
  - **Formatting**: Automatically adds thousands separators (e.g., "1,234"), supports custom format strings.
  - **Summaries**: Supports `sum`, `min`, `max`, `avg`, `count`, `unique` for detailed analysis.
- **kcpu**: Kubernetes-style CPU values, typically in millicores.
  - **Validation**: Accepts values with 'm' suffix (e.g., "100m") or plain numeric values (interpreted as cores), supports zero and null.
  - **Formatting**: Always displays with 'm' suffix for millicores, adds thousands separators for large values, converts plain numbers to millicores (e.g., "1.5" to "1,500m").
  - **Summaries**: Supports `sum`, `min`, `max`, `avg`, `count`, `unique` for resource analysis.
- **kmem**: Kubernetes-style memory values with unit suffixes.
  - **Validation**: Accepts values with K, M, G suffixes or binary units (Ki, Mi, Gi), supports zero and null values.
  - **Formatting**: Normalizes binary units to decimal (e.g., Mi to M), adds thousands separators, preserves original unit type for clarity.
  - **Summaries**: Supports `sum`, `min`, `max`, `avg`, `count`, `unique` for memory usage statistics.

### Datatypes Library Validation Functions

- **validate_text**: Ensures text data is non-null, returning the original value if valid or an empty string if null.

  ```bash
  result=$(validate_text "Hello World")  # Returns: "Hello World"
  result=$(validate_text "null")         # Returns: ""
  ```

- **validate_number**: Validates numeric data for `int`, `float`, and `num` types, accepting whole or decimal numbers and returning empty for invalid input.

  ```bash
  result=$(validate_number "123.45")     # Returns: "123.45"
  result=$(validate_number "abc")        # Returns: ""
  ```

- **validate_kcpu**: Validates Kubernetes CPU values, accepting 'm'-suffixed or plain numeric values, returning empty for invalid formats.

  ```bash
  result=$(validate_kcpu "100m")         # Returns: "100m"
  result=$(validate_kcpu "1.5")          # Returns: "1.5"
  result=$(validate_kcpu "abc")          # Returns: ""
  ```

- **validate_kmem**: Validates Kubernetes memory values with unit suffixes (K, M, G, Ki, Mi, Gi), returning empty for invalid formats.

  ```bash
  result=$(validate_kmem "128M")         # Returns: "128M"
  result=$(validate_kmem "1Gi")          # Returns: "1Gi"
  result=$(validate_kmem "abc")          # Returns: ""
  ```

### Datatypes Library Formatting Functions

- **format_text**: Formats text data with options for length limits, wrapping mode (`clip` or `wrap`), custom wrap characters, and justification for truncation.

  ```bash
  result=$(format_text "Hello World" "" 5 "clip" "" "left")    # Returns: "Hello"
  result=$(format_text "Hello World" "" 5 "clip" "" "right")   # Returns: "World"
  ```

- **format_number**: Formats basic numeric values for `int` and `float`, supporting custom format strings for display customization.

  ```bash
  result=$(format_number "123" "")      # Returns: "123"
  result=$(format_number "null" "")     # Returns: ""
  ```

- **format_num**: Formats numeric values with thousands separators for the `num` type, enhancing readability of large numbers.

  ```bash
  result=$(format_num "1234" "")        # Returns: "1,234"
  result=$(format_num "1234567" "")     # Returns: "1,234,567"
  ```

- **format_kcpu**: Formats Kubernetes CPU values, ensuring 'm' suffix display and thousands separators, converting plain numbers to millicores.

  ```bash
  result=$(format_kcpu "100m" "")       # Returns: "100m"
  result=$(format_kcpu "1.5" "")        # Returns: "1,500m"
  result=$(format_kcpu "2000m" "")      # Returns: "2,000m"
  ```

- **format_kmem**: Formats Kubernetes memory values, normalizing binary units (e.g., Mi to M) and adding thousands separators while preserving unit type.

  ```bash
  result=$(format_kmem "128M" "")       # Returns: "128M"
  result=$(format_kmem "1Gi" "")        # Returns: "1G"
  result=$(format_kmem "2048Mi" "")     # Returns: "2,048M"
  ```

- **format_display_value**: A high-level function orchestrating the complete formatting pipeline, handling null/zero values, data type-specific formatting, and advanced text processing options.

  ```bash
  result=$(format_display_value "null" "missing" "0" "text" "" 0 "clip" "" "left")  # Returns: "Missing"
  result=$(format_display_value "1234" "blank" "blank" "num" "" 0 "clip" "" "left")  # Returns: "1,234"
  ```

### Datatypes Library Data Type Integration

The data type system integrates seamlessly with the broader `tables.sh` framework through configuration and processing pipelines:

- **Configuration Integration**: Accesses handler functions dynamically based on data type for validation, formatting, and summary compatibility.

  ```bash
  validate_fn="${DATATYPE_HANDLERS[${datatype}_validate]}"  # Retrieves validation function
  format_fn="${DATATYPE_HANDLERS[${datatype}_format]}"      # Retrieves formatting function
  summary_types="${DATATYPE_HANDLERS[${datatype}_summary_types]}"  # Retrieves supported summaries
  ```

- **Processing Pipeline**: Follows a structured flow:
  1. **Validation**: Ensures data matches the expected format for the specified type.
  2. **Formatting**: Applies display formatting rules specific to the data type.
  3. **Null/Zero Handling**: Adjusts display for special values based on configuration.
  4. **Summary Calculation**: Updates appropriate summary statistics compatible with the data type.

### Datatypes Library Summary Type Compatibility

Each data type supports specific summary calculations, ensuring meaningful statistical outputs:

| Data Type | sum | min | max | avg | count | unique |
|-----------|-----|-----|-----|-----|-------|--------|
| text      | ❌  | ❌  | ❌  | ❌  | ✅    | ✅     |
| int       | ✅  | ✅  | ✅  | ✅  | ✅    | ✅     |
| float     | ✅  | ✅  | ✅  | ✅  | ✅    | ✅     |
| num       | ✅  | ✅  | ✅  | ✅  | ✅    | ✅     |
| kcpu      | ✅  | ✅  | ✅  | ✅  | ✅    | ✅     |
| kmem      | ✅  | ✅  | ✅  | ✅  | ✅    | ✅     |

### Datatypes Library Advanced Text Processing

- **String Limiting**: Text data can be limited by length with configurable truncation strategies:
  - **Left Truncation**: Retains rightmost characters when trimming.
  - **Right Truncation**: Retains leftmost characters (default behavior).
  - **Center Truncation**: Retains middle characters, trimming from both ends.
- **Text Wrapping**: Supports splitting text into multiple lines using custom characters:
  - Splits content by the specified wrap character.
  - Displays each segment on a separate line during rendering.
  - Calculates appropriate column width to accommodate wrapped content.

### Datatypes Library Error Handling

- **Invalid Data**: Returns empty strings for data that fails validation, preventing display errors.
- **Null Values**: Handles nulls according to configuration settings (`blank`, `0`, `missing`).
- **Zero Values**: Similarly handles zero values per configuration for consistent output.
- **Format Errors**: Falls back to raw values if formatting fails, ensuring data visibility.

### Datatypes Library Performance Considerations

- **Validation Overhead**: Each value undergoes validation before formatting, adding minor overhead.
- **Regex Matching**: Complex pattern matching for validation (e.g., `kcpu`, `kmem`) may impact speed with large datasets.
- **String Operations**: Text processing like wrapping and truncation can be resource-intensive for extensive data.
- **Function Calls**: Dynamic resolution of handler functions introduces slight performance costs.

### Datatypes Library Dependencies

- **awk**: Used for numeric formatting and calculations, especially in summaries.
- **bash**: Leverages built-in pattern matching and string operations for validation.
- **printf**: Facilitates formatted output for custom display strings.

### Datatypes Library Integration with Tables System

This library underpins data handling across `tables.sh`:

1. **tables_config.sh**: Validates data types during configuration setup.
2. **tables_data.sh**: Uses validation and formatting functions for data processing.
3. **tables_render.sh**: Relies on formatted values for accurate display.
4. **tables.sh**: Coordinates the pipeline, integrating data type logic throughout.

## Rendering Library

### Rendering Library Overview

The Tables Render Library (`lib/tables_render.sh`) is the visual output engine of the `tables.sh` framework, responsible for rendering all aspects of table display, including borders, headers, data rows, summaries, titles, and footers, with precise formatting and styling.

### Rendering Library Purpose

- Render complete table structures into a visually coherent output for terminal display.
- Manage complex border rendering, seamlessly integrating titles and footers into the table layout.
- Support text wrapping and multi-line content for flexible data presentation.
- Handle column visibility to show or hide columns without disrupting layout integrity.
- Provide consistent visual styling across all table elements using theme configurations.
- Implement break separators and summary rows for enhanced data organization and insight.

### Rendering Library Key Features

- **Complete Table Rendering**: Executes a full rendering pipeline from processed data to final visual output.
- **Border Management**: Handles intricate border rendering, integrating titles and footers into top and bottom borders.
- **Text Wrapping**: Supports multi-line content with configurable wrapping modes for proper alignment and readability.
- **Column Visibility**: Allows hiding or showing columns without affecting overall table layout or data processing.
- **Break Separators**: Inserts visual separators between rows when values in break-configured columns change, aiding data grouping.
- **Summary Rendering**: Displays formatted summary rows with calculated statistics for quick data analysis.
- **Title/Footer Support**: Offers flexible positioning options (left, right, center, full) for titles and footers to customize table appearance.

### Rendering Library Core Functions

- **render_table_title**: Renders the table title section if specified, respecting the configured positioning and width for alignment within the table structure.

  ```bash
  render_table_title 50  # Renders title for a table width of 50 characters
  ```

- **render_table_border**: A shared utility for rendering complex top or bottom borders, integrating title/footer elements with appropriate junction characters.

  ```bash
  render_table_border "top" 50 0 50 50 "center" false  # Renders top border with title integration
  ```

- **render_table_top_border**: Renders the top border of the table, calculating total width from visible columns and integrating title positioning.

  ```bash
  render_table_top_border  # Renders top border with title if configured
  ```

- **render_table_bottom_border**: Renders the bottom border, similarly integrating footer positioning and width for a cohesive design.

  ```bash
  render_table_bottom_border  # Renders bottom border with footer if configured
  ```

- **render_table_headers**: Renders the header row for visible columns, applying justification and clipping text to fit width constraints, styled with theme colors.

  ```bash
  render_table_headers  # Renders headers for all visible columns
  ```

- **render_table_separator**: Renders horizontal separator lines within the table (e.g., after headers or before summaries), using appropriate junction characters for the type ("middle" or "bottom").

  ```bash
  render_table_separator "middle"  # Renders separator after headers
  ```

- **render_data_rows**: The main function for rendering data rows, processing break separators, handling text wrapping modes, supporting multi-line content, and applying data type formatting for visible columns.

  ```bash
  render_data_rows "$MAX_LINES"  # Renders data rows respecting max lines per row
  ```

- **render_table_footer**: Renders the table footer section if specified, evaluating shell commands in text and handling width and positioning for alignment.

  ```bash
  render_table_footer 50  # Renders footer for a table width of 50 characters
  ```

- **render_summaries_row**: Renders the summary row if any summaries are defined, calculating and formatting values based on type and data type, styled with theme colors.

  ```bash
  if render_summaries_row; then
      echo "Summaries rendered successfully."
  else
      echo "No summaries configured to render."
  fi
  ```

### Rendering Library Rendering Pipeline

The rendering process follows a structured sequence to build the table visually:

1. **Title Rendering**: If a title is specified, renders it with the configured positioning (left, right, center, full).
2. **Top Border**: Renders the top border, integrating the title if present, using appropriate junction characters.
3. **Headers**: Renders column headers for visible columns with formatting and theme styling.
4. **Header Separator**: Renders a separator line after headers to distinguish them from data rows.
5. **Data Rows**: Renders all data rows, inserting break separators where configured, handling text wrapping and multi-line content.
6. **Summary Separator**: If summaries exist, renders a separator line before the summary row for visual distinction.
7. **Summary Row**: Renders calculated summaries with appropriate formatting and styling.
8. **Bottom Border**: Renders the bottom border, integrating the footer if present, mirroring top border logic.
9. **Footer Rendering**: If a footer is specified, renders it with the configured positioning.

### Rendering Library Text Processing Features

- **Multi-line Content Support**: Accommodates content spanning multiple lines through different strategies:
  - **Character-based Wrapping**: Splits content by a specified `wrap_char` (e.g., "|") to create separate lines.

    ```json
    "wrap_mode": "wrap", "wrap_char": "|"  # Splits "Line1|Line2" into two lines
    ```

  - **Word Wrapping**: Breaks content on word boundaries if no wrap character is specified, preserving readability.

    ```json
    "wrap_mode": "wrap", "wrap_char": ""  # Wraps long text at word boundaries
    ```

  - **Content Clipping**: Truncates content to fit column width if wrapping is disabled (`wrap_mode`: "clip").

    ```json
    "wrap_mode": "clip"  # Clips "Very long text" to fit column width
    ```

- **Justification and Clipping**: Supports three alignment modes for text content:
  - **Left**: Aligns left, clips from the right.
  - **Right**: Aligns right, clips from the left.
  - **Center**: Centers content, clips from both sides.
