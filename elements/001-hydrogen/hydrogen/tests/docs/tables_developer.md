# Tables Developer Documentation

This document provides detailed technical information about the internal workings of the `tables.sh` script and instructions for extending its functionality.

## Architecture Overview

The tables.sh script is organized into several functional components:

1. **Main Script** (`tables.sh`): Entry point, argument parsing, and main execution flow
2. **Configuration System** (`lib/tables_config.sh`): Parses and validates layout and data JSON files
3. **Theme System** (`lib/tables_themes.sh`): Manages visual appearance with customizable themes
4. **Datatype System** (`lib/tables_datatypes.sh`): Validates and formats different types of data
5. **Data Processing Pipeline** (`lib/tables_data.sh`): Prepares, sorts, and processes data rows
6. **Rendering System** (`lib/tables_render.sh`): Draws the table with borders, content, and summaries

The script follows a modular design with clearly defined functions for each component, making it extensible without requiring significant changes to the core code.

## File Structure

```directory
tables/
├── tables.sh                    # Main script and entry point
├── lib/
│   ├── tables_config.sh         # Configuration parsing and validation
│   ├── tables_data.sh           # Data processing pipeline
│   ├── tables_datatypes.sh      # Data type system
│   ├── tables_render.sh         # Table rendering system
│   └── tables_themes.sh         # Theme definitions
├── tst/                         # Test files
│   ├── tables_test_01_basic.sh
│   ├── tables_test_03_titles.sh
│   ├── tables_test_05_footers.sh
│   └── ...
├── tables.md                    # User documentation
└── tables_developer.md          # This file
```

## Core Components and Flow

The main execution flow follows these steps:

1. Parse arguments and validate input files
2. Parse layout JSON and set theme
3. Initialize summaries storage
4. Read and prepare data
5. Sort data if specified
6. Process data rows, update column widths, calculate summaries
7. Render the table (title, borders, headers, data rows, summaries, footer)

Here's a visualization of the data flow:

```diagram
Input JSON Files
       ↓
Configuration Parsing
       ↓
  Theme Setup
       ↓
 Data Processing ←→ Datatype System
       ↓
 Width Calculation
       ↓
  Table Rendering
       ↓
    Output
```

## Global Variables and Arrays

The script uses several global variables and arrays to maintain state:

### Configuration Arrays

- `HEADERS[]`: Column header text
- `KEYS[]`: JSON field names for data extraction
- `JUSTIFICATIONS[]`: Text alignment (left, right, center)
- `DATATYPES[]`: Data type for each column
- `NULL_VALUES[]`: How to display null values
- `ZERO_VALUES[]`: How to display zero values
- `FORMATS[]`: Custom format strings
- `SUMMARIES[]`: Summary type for each column
- `BREAKS[]`: Whether to insert separators on value changes
- `STRING_LIMITS[]`: Maximum string lengths
- `WRAP_MODES[]`: Text wrapping behavior
- `WRAP_CHARS[]`: Characters used for wrapping
- `PADDINGS[]`: Padding spaces for each column
- `WIDTHS[]`: Calculated or specified column widths
- `VISIBLES[]`: Whether to display the column (true/false for visibility)

### Sort Configuration

- `SORT_KEYS[]`: Keys to sort by
- `SORT_DIRECTIONS[]`: Sort directions (asc/desc)
- `SORT_PRIORITIES[]`: Sort priorities

### Data Storage

- `ROW_JSONS[]`: Processed row data
- `SUM_SUMMARIES{}`: Sum calculations by column
- `COUNT_SUMMARIES{}`: Count calculations by column
- `MIN_SUMMARIES{}`: Minimum values by column
- `MAX_SUMMARIES{}`: Maximum values by column
- `UNIQUE_VALUES{}`: Unique value tracking by column

### Title and Footer Support

- `TABLE_TITLE`: Title text
- `TITLE_WIDTH`: Calculated title width
- `TITLE_POSITION`: Title positioning (left, right, center, full, none)
- `TABLE_FOOTER`: Footer text
- `FOOTER_WIDTH`: Calculated footer width
- `FOOTER_POSITION`: Footer positioning (left, right, center, full, none)

## Theme System

The theme system defines the visual appearance of tables, including colors and border characters.

### Theme Structure

Each theme is stored as an associative array with the following keys:

```bash
declare -A THEME_NAME=(
    [border_color]='ANSI color code'    # Table borders and separators
    [caption_color]='ANSI color code'   # Column headers
    [header_color]='ANSI color code'    # Table title
    [footer_color]='ANSI color code'    # Table footer
    [summary_color]='ANSI color code'   # Summary/totals row
    [text_color]='ANSI color code'      # Regular data content
    [tl_corner]='character'             # Top-left corner
    [tr_corner]='character'             # Top-right corner
    [bl_corner]='character'             # Bottom-left corner
    [br_corner]='character'             # Bottom-right corner
    [h_line]='character'                # Horizontal line
    [v_line]='character'                # Vertical line
    [t_junct]='character'               # Top junction
    [b_junct]='character'               # Bottom junction
    [l_junct]='character'               # Left junction
    [r_junct]='character'               # Right junction
    [cross]='character'                 # Cross junction
)
```

### Current Themes

**Red Theme:**

- Border: Red (`\033[0;31m`)
- Caption: Green (`\033[0;32m`)
- Header: Bright White (`\033[1;37m`)
- Footer: Cyan (`\033[0;36m`)
- Summary: Bright White (`\033[1;37m`)
- Text: Default (`\033[0m`)

**Blue Theme:**

- Border: Blue (`\033[0;34m`)
- Caption: Blue (`\033[0;34m`)
- Header: Bright White (`\033[1;37m`)
- Footer: Cyan (`\033[0;36m`)
- Summary: Bright White (`\033[1;37m`)
- Text: Default (`\033[0m`)

### Adding a New Theme

To add a new theme, follow these steps:

1. Define a new associative array in `lib/tables_themes.sh`:

```bash
declare -A GREEN_THEME=(
    [border_color]='\033[0;32m'  # Green border color
    [caption_color]='\033[0;32m' # Green caption color
    [header_color]='\033[1;37m'  # White header color
    [footer_color]='\033[0;36m'  # Cyan footer color
    [summary_color]='\033[1;37m' # White summary color
    [text_color]='\033[0m'       # Default text color
    [tl_corner]='╭'
    [tr_corner]='╮'
    [bl_corner]='╰'
    [br_corner]='╯'
    [h_line]='─'
    [v_line]='│'
    [t_junct]='┬'
    [b_junct]='┴'
    [l_junct]='├'
    [r_junct]='┤'
    [cross]='┼'
)
```

1. Update the `get_theme` function to support your new theme:

```bash
get_theme() {
    local theme_name="$1"
    # Clear existing THEME entries
    unset THEME
    declare -g -A THEME

    # Set new theme
    case "${theme_name,,}" in
        red)
            for key in "${!RED_THEME[@]}"; do
                THEME[$key]="${RED_THEME[$key]}"
            done
            ;;
        blue)
            for key in "${!BLUE_THEME[@]}"; do
                THEME[$key]="${BLUE_THEME[$key]}"
            done
            ;;
        green)  # Add your new theme case here
            for key in "${!GREEN_THEME[@]}"; do
                THEME[$key]="${GREEN_THEME[$key]}"
            done
            ;;
        *)
            for key in "${!RED_THEME[@]}"; do
                THEME[$key]="${RED_THEME[$key]}"
            done
            echo -e "${THEME[border_color]}Warning: Unknown theme '$theme_name', using Red${THEME[text_color]}" >&2
            ;;
    esac
}
```

## Datatype System

The datatype system manages how different types of data are validated, formatted, and summarized.

### Datatype Registry

The script uses a registry pattern to map datatypes to their respective functions:

```bash
declare -A DATATYPE_HANDLERS=(
    [text_validate]="validate_text"
    [text_format]="format_text"
    [text_summary_types]="count unique"
    [int_validate]="validate_number"
    [int_format]="format_number"
    [int_summary_types]="sum min max count unique"
    [num_validate]="validate_number"
    [num_format]="format_num"
    [num_summary_types]="sum min max count unique"
    [float_validate]="validate_number"
    [float_format]="format_number"
    [float_summary_types]="sum min max count unique"
    [kcpu_validate]="validate_kcpu"
    [kcpu_format]="format_kcpu"
    [kcpu_summary_types]="sum count"
    [kmem_validate]="validate_kmem"
    [kmem_format]="format_kmem"
    [kmem_summary_types]="sum count"
)
```

For each datatype, three entries are registered:

- `datatype_validate`: Function to validate input values
- `datatype_format`: Function to format values for display
- `datatype_summary_types`: Space-separated list of supported summary types

### Current Data Types

1. **text**: Plain text with optional wrapping
2. **int**: Integer numbers
3. **float**: Floating-point numbers
4. **num**: Numbers with thousands separator formatting
5. **kcpu**: Kubernetes CPU values (e.g., "100m")
6. **kmem**: Kubernetes memory values (e.g., "128M")

### Adding a New Datatype

To add a new datatype, follow these steps:

1. Define validation and formatting functions in `lib/tables_datatypes.sh`:

```bash
# Example: Adding a "percentage" datatype

# Validation function
validate_percentage() {
    local value="$1"
    if [[ "$value" =~ ^[0-9]+(\.[0-9]+)?%$ ]] || [[ "$value" == "0%" ]] || [[ "$value" == "null" ]]; then
        echo "$value"
    else
        echo ""
    fi
}

# Formatting function
format_percentage() {
    local value="$1" format="$2"
    [[ -z "$value" || "$value" == "null" || "$value" == "0%" ]] && { echo ""; return; }
    if [[ -n "$format" && "$value" =~ ^[0-9]+(\.[0-9]+)?%$ ]]; then
        local num_value=${value%\%}
        printf "$format" "$num_value"
    else
        echo "$value"
    fi
}
```

1. Update the datatype registry:

```bash
declare -A DATATYPE_HANDLERS=(
    # Existing handlers...
    [percentage_validate]="validate_percentage"
    [percentage_format]="format_percentage"
    [percentage_summary_types]="sum min max count unique"
)
```

1. If needed, update the `update_summaries` function in `lib/tables_data.sh` for custom summary handling.

## Data Processing Pipeline

The data processing pipeline handles preparing, sorting, and processing data for display.

### Key Functions

1. **initialize_summaries**: Initialize summary storage arrays
2. **prepare_data**: Read and validate data from JSON file
3. **sort_data**: Apply sorting based on configuration
4. **process_data_rows**: Process rows, update column widths, calculate summaries
5. **update_summaries**: Update running summaries for columns

### Summary Calculations

The system supports several summary types:

- **sum**: Arithmetic sum of numeric values
- **min**: Minimum value
- **max**: Maximum value
- **avg**: Average value of numeric data
- **count**: Count of non-null values
- **unique**: Count of unique values

Summary calculations are handled differently based on data type:

```bash
update_summaries() {
    local j="$1" value="$2" datatype="$3" summary_type="$4"
    
    case "$summary_type" in
        sum)
            if [[ "$datatype" == "kcpu" && "$value" =~ ^[0-9]+m$ ]]; then
                SUM_SUMMARIES[$j]=$(( ${SUM_SUMMARIES[$j]:-0} + ${value%m} ))
            elif [[ "$datatype" == "kmem" ]]; then
                # Handle different memory units...
            elif [[ "$datatype" == "int" || "$datatype" == "float" || "$datatype" == "num" ]]; then
                if [[ "$value" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
                    SUM_SUMMARIES[$j]=$(awk "BEGIN {print (${SUM_SUMMARIES[$j]:-0} + $value)}")
                fi
            fi
            ;;
        # Other summary types...
    esac
}
```

## Rendering System

The rendering system draws the table with borders, headers, data rows, and summaries.

### Key Components

1. **render_table_title**: Renders the title section with positioning
2. **render_table_border**: Renders table borders (top/bottom) with element integration
3. **render_table_top_border**: Renders the top border
4. **render_table_headers**: Renders the header row
5. **render_table_separator**: Renders horizontal separators
6. **render_data_rows**: Renders the data rows with proper formatting
7. **render_summaries_row**: Renders the summaries row if needed
8. **render_table_bottom_border**: Renders the bottom border
9. **render_table_footer**: Renders the footer section with positioning

### Title and Footer Rendering

The system supports flexible positioning of titles and footers:

```bash
render_table_title() {
    local total_table_width="$1"
    
    if [[ -n "$TABLE_TITLE" ]]; then
        calculate_title_width "$TABLE_TITLE" "$total_table_width"
        
        local offset=0
        case "$TITLE_POSITION" in
            left) offset=0 ;;
            right) offset=$((total_table_width - TITLE_WIDTH)) ;;
            center) offset=$(((total_table_width - TITLE_WIDTH) / 2)) ;;
            full) offset=0 ;;
            *) offset=0 ;;
        esac
        
        # Render title with appropriate positioning...
    fi
}
```

### Border Integration

The border rendering system intelligently handles integration between table borders and title/footer elements:

```bash
render_table_border() {
    local border_type="$1"
    local total_table_width="$2"
    local element_offset="$3"
    local element_right_edge="$4"
    local element_width="$5"
    
    # Calculate maximum width and column positions
    # Render border character by character with appropriate junctions
}
```

## Configuration System

The configuration system parses and validates layout JSON files.

### Key Configuration Functions

1. **validate_input_files**: Check if layout and data files exist and are valid
2. **parse_layout_file**: Extract theme, title, footer, columns, and sort information
3. **parse_column_config**: Process column configurations
4. **validate_column_config**: Validate column settings and print warnings
5. **parse_sort_config**: Process sort configurations

### Column Configuration Processing

```bash
parse_column_config() {
    local columns_json="$1"
    
    # Clear all arrays first
    HEADERS=()
    KEYS=()
    # ... other arrays
    
    local column_count
    column_count=$(jq '. | length' <<<"$columns_json")
    COLUMN_COUNT=$column_count
    
    for ((i=0; i<column_count; i++)); do
        local col_json
        col_json=$(jq -c ".[$i]" <<<"$columns_json")
        HEADERS[i]=$(jq -r '.header // ""' <<<"$col_json")
        KEYS[i]=$(jq -r '.key // (.header | ascii_downcase | gsub("[^a-z0-9]"; "_"))' <<<"$col_json")
        # ... process other column properties including visibility
        
        validate_column_config "$i" "${HEADERS[$i]}" "${JUSTIFICATIONS[$i]}" "${DATATYPES[$i]}" "${SUMMARIES[$i]}"
    done
}
```

#### Column Visibility Feature

The configuration system supports a `visible` property for each column, stored in the `VISIBLES[]` array. Setting `"visible": false` in a column's JSON configuration excludes it from the rendered table output without affecting data processing. This feature is useful for including data needed for sorting or other operations while keeping it hidden from the final display. The rendering system ensures that hidden columns do not impact table layout or border calculations.

## Extension Examples

### Adding Column Calculations

To add calculated columns that derive values from other columns:

1. Extend the column configuration to support calculations:

```json
{
  "columns": [
    {
      "header": "TOTAL",
      "key": "calculated_total",
      "datatype": "float",
      "calculation": "price * quantity",
      "format": "%.2f"
    }
  ]
}
```

1. Add a calculation processor during data preparation:

```bash
process_calculations() {
    local row_json="$1"
    local result="$row_json"
    
    for ((j=0; j<COLUMN_COUNT; j++)); do
        local calculation="${CALCULATIONS[$j]}"
        if [[ -n "$calculation" ]]; then
            # Replace column keys with their values
            local calc_expr="$calculation"
            for ((k=0; k<COLUMN_COUNT; k++)); do
                local key="${KEYS[$k]}"
                local value=$(jq -r ".${key} // \"0\"" <<<"$row_json")
                if [[ "$value" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
                    calc_expr=${calc_expr//$key/$value}
                else
                    calc_expr=${calc_expr//$key/0}
                fi
            done
            
            # Evaluate the expression and store the result
            local calc_result=$(awk "BEGIN {print ($calc_expr)}")
            result=$(jq --arg key "${KEYS[$j]}" --arg val "$calc_result" '. + {($key): $val}' <<<"$result")
        fi
    done
    
    echo "$result"
}
```

### Adding Output Formats

To support different output formats (HTML, CSV, etc.):

1. Add an output format parameter to the configuration:

```json
{
  "theme": "Red",
  "output_format": "ascii",  // or "html", "csv", etc.
  "columns": [...]
}
```

1. Create new rendering functions for each format:

```bash
render_table_html() {
    echo "<table class='tables-output'>"
    echo "<thead><tr>"
    for ((i=0; i<COLUMN_COUNT; i++)); do
        echo "<th>${HEADERS[$i]}</th>"
    done
    echo "</tr></thead>"
    echo "<tbody>"
    # ... render data rows
    echo "</tbody>"
    echo "</table>"
}

render_table_csv() {
    # Print headers
    printf "%s" "${HEADERS[0]}"
    for ((i=1; i<COLUMN_COUNT; i++)); do
        printf ",%s" "${HEADERS[$i]}"
    done
    printf "\n"
    
    # Print data rows
    for row_json in "${ROW_JSONS[@]}"; do
        # ... format and print CSV row
    done
}
```

## Performance Optimization

For large datasets, consider these optimizations:

1. **Batched Processing**: Process data in batches instead of all at once
2. **Lazy Evaluation**: Only calculate values when needed
3. **Caching**: Cache calculated values and intermediate results
4. **Streaming**: Process data as a stream rather than loading everything into memory

Example of batched processing:

```bash
process_data_batched() {
    local data_file="$1" batch_size=100
    local row_count=$(jq '. | length' "$data_file")
    
    for ((i=0; i<row_count; i+=batch_size)); do
        local end=$((i+batch_size))
        [[ $end -gt $row_count ]] && end=$row_count
        
        jq -c ".[$i:$end]" "$data_file" | process_batch
    done
}
```

## Debugging and Testing

### Debug Mode

Use the `--debug` flag to enable detailed logging:

```bash
./tables.sh layout.json data.json --debug
```

Debug messages are sent to stderr and include:

- Configuration parsing details
- Data processing steps
- Width calculations
- Rendering decisions

### Test Framework

The `tst/` directory contains an extensive suite of test files that demonstrate various features and ensure reliability:

- [`tables_test_01_basic.sh`](../lib/tables-tests/tables_test_01_basic.sh): Basic functionality tests
- [`tables_test_02_summary.sh`](../lib/tables-tests/tables_test_02_summary.sh): Summary calculation tests
- [`tables_test_03_wrapping.sh`](../lib/tables-tests/tables_test_03_wrapping.sh): Text wrapping functionality tests
- [`tables_test_04_complex.sh`](../lib/tables-tests/tables_test_04_complex.sh): Complex table rendering tests
- [`tables_test_05_titles.sh`](../lib/tables-tests/tables_test_05_titles.sh): Title positioning tests
- [`tables_test_06_title_positions.sh`](../lib/tables-tests/tables_test_06_title_positions.sh): Additional title positioning scenarios
- [`tables_test_07_footers.sh`](../lib/tables-tests/tables_test_07_footers.sh): Footer functionality tests
- [`tables_test_08_footer_positions.sh`](../lib/tables-tests/tables_test_08_footer_positions.sh): Additional footer positioning scenarios
- [`tables_test_09_showcase.sh`](../lib/tables-tests/tables_test_09_showcase.sh): Comprehensive feature showcase

### Creating Test Cases

```bash
test_new_feature() {
    cat > test_layout.json << 'EOF'
    {
      "theme": "Red",
      "columns": [
        {
          "header": "TEST",
          "key": "test_value",
          "datatype": "your_new_datatype"
        }
      ]
    }
    EOF
    
    cat > test_data.json << 'EOF'
    [
      {"test_value": "sample_data"}
    ]
    EOF
    
    ./tables.sh test_layout.json test_data.json --debug
}
```

## Best Practices for Extensions

1. **Maintain Compatibility**: Ensure backward compatibility with existing features
2. **Follow Naming Conventions**: Use consistent naming patterns for functions and variables
3. **Add Documentation**: Document new features and update usage examples
4. **Error Handling**: Include proper validation and error messages
5. **Performance**: Consider the impact on performance, especially for large datasets
6. **Testing**: Create test cases to verify functionality
7. **Modular Design**: Keep extensions in appropriate modules (lib/ files)

## Common Pitfalls

1. **Shell Limitations**: Remember that Bash has limited mathematical capabilities; use `awk` for complex math
2. **Quoting**: Be careful with variable quoting to handle spaces and special characters
3. **Escaping**: Properly escape characters in regular expressions and JSON
4. **Global Variables**: Be careful with global variable scope and initialization
5. **Array Indexing**: Bash arrays are zero-indexed; ensure consistent indexing
6. **Error Propagation**: Ensure errors are properly reported up the call stack
7. **ANSI Sequences**: Remember that ANSI color codes affect string length calculations

## Version History

- **1.1.0**: Added Avg summary type, enhanced wrap functionality, expanded datatype support for summaries, increased test coverage, and passed shellcheck with no errors or warnings
- **1.0.2**: Added help functionality and version history section
- **1.0.1**: Fixed shellcheck issues (SC2004, SC2155)
- **1.0.0**: Initial release with table rendering functionality

The current implementation includes title/footer support, enhanced theming, the `num` datatype with thousands separators, comprehensive summary calculations including averages, advanced text wrapping options, and support for column visibility to hide specific columns from display.
