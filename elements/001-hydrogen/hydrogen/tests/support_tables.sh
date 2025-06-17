#!/usr/bin/env bash

# tables.sh: Reusable table-drawing library for rendering JSON data as ASCII tables
# Functions: draw_table
# Usage: draw_table <layout_json_file> <data_json_file> [--debug] [--version]
# Dependencies: jq, bash 4.0+, awk, sed

# Version information
declare -r TABLES_VERSION="1.0.1"

# Debug flag - can be set by parent script
DEBUG_FLAG=false

# Global variables
declare -g COLUMN_COUNT=0
declare -g MAX_LINES=1
declare -g THEME_NAME="Red"
declare -g DEFAULT_PADDING=1

# Themes for table styling
# Each theme defines colors and ASCII characters for table borders
declare -A RED_THEME=(
    [border_color]='\033[0;31m' # Red border color
    [caption_color]='\033[0;32m' # Green caption color (column headers)
    [header_color]='\033[1;37m'  # White header color (bright, table header)
    [footer_color]='\033[0;36m'  # Cyan footer color (non-bold)
    [summary_color]='\033[1;37m' # White summary color (bright, summary row)
    [text_color]='\033[0m'       # Default text color (terminal default)
    [tl_corner]='╭'             # Top-left corner
    [tr_corner]='╮'             # Top-right corner
    [bl_corner]='╰'             # Bottom-left corner
    [br_corner]='╯'             # Bottom-right corner
    [h_line]='─'                # Horizontal line
    [v_line]='│'                # Vertical line
    [t_junct]='┬'              # Top junction
    [b_junct]='┴'              # Bottom junction
    [l_junct]='├'              # Left junction
    [r_junct]='┤'              # Right junction
    [cross]='┼'                 # Cross junction
)

declare -A BLUE_THEME=(
    [border_color]='\033[0;34m' # Blue border color
    [caption_color]='\033[0;34m' # Blue caption color (column headers)
    [header_color]='\033[1;37m'  # White header color (bright, table header)
    [footer_color]='\033[0;36m'  # Cyan footer color (non-bold)
    [summary_color]='\033[1;37m' # White summary color (bright, summary row)
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

# Current theme, always initialized to RED_THEME
declare -A THEME
# Explicitly copy RED_THEME to THEME
for key in "${!RED_THEME[@]}"; do
    THEME[$key]="${RED_THEME[$key]}"
done

# Debug logger function
debug_log() {
    [[ "$DEBUG_FLAG" == "true" ]] && echo "[DEBUG] $*" >&2
}

# get_theme: Updates the active theme based on name
# Args: theme_name (string, e.g., "Red", "Blue")
# Side effect: Updates global THEME array
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
        *)
            for key in "${!RED_THEME[@]}"; do
                THEME[$key]="${RED_THEME[$key]}"
            done
            echo -e "${THEME[border_color]}Warning: Unknown theme '$theme_name', using Red${THEME[text_color]}" >&2
            ;;
    esac
}

# Datatype registry: Maps datatypes to validation, formatting, and summary functions
# Format: [datatype_function]="function_name"
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

# Validation functions: Ensure data matches expected format
# Each returns the validated value or empty string if invalid
validate_text() {
    local value="$1"
    [[ "$value" != "null" ]] && echo "$value" || echo ""
}

validate_number() {
    local value="$1"
    if [[ "$value" =~ ^[0-9]+(\.[0-9]+)?$ ]] || [[ "$value" == "0" ]] || [[ "$value" == "null" ]]; then
        echo "$value"
    else
        echo ""
    fi
}

validate_kcpu() {
    local value="$1"
    if [[ "$value" =~ ^[0-9]+m$ ]] || [[ "$value" == "0" ]] || [[ "$value" == "0m" ]] || [[ "$value" == "null" ]] || [[ "$value" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
        echo "$value"
    else
        echo ""
    fi
}

validate_kmem() {
    local value="$1"
    if [[ "$value" =~ ^[0-9]+[KMG]$ ]] || [[ "$value" =~ ^[0-9]+Mi$ ]] || [[ "$value" =~ ^[0-9]+Gi$ ]] || [[ "$value" =~ ^[0-9]+Ki$ ]] || [[ "$value" == "0" ]] || [[ "$value" == "null" ]]; then
        echo "$value"
    else
        echo ""
    fi
}

# Formatting functions: Convert validated data into display format
format_text() {
    local value="$1" format="$2" string_limit="$3" wrap_mode="$4" wrap_char="$5"
    [[ -z "$value" || "$value" == "null" ]] && { echo ""; return; }
    if [[ "$string_limit" -gt 0 && ${#value} -gt $string_limit ]]; then
        if [[ "$wrap_mode" == "clip" ]]; then
            echo "${value:0:$string_limit}"
        elif [[ -n "$wrap_char" ]]; then
            local wrapped=""
            local IFS="$wrap_char"
            read -ra parts <<< "$value"
            for part in "${parts[@]}"; do
                wrapped+="$part\n"
            done
            echo -e "$wrapped" | head -n $((string_limit / ${#wrap_char}))
        else
            echo "${value:0:$string_limit}"
        fi
    else
        echo "$value"
    fi
}

format_number() {
    local value="$1" format="$2"
    [[ -z "$value" || "$value" == "null" || "$value" == "0" ]] && { echo ""; return; }
    if [[ -n "$format" && "$value" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
        printf "$format" "$value"
    else
        echo "$value"
    fi
}

format_num() {
    local value="$1" format="$2"
    [[ -z "$value" || "$value" == "null" || "$value" == "0" ]] && { echo ""; return; }
    if [[ "$value" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
        # Format with thousands separator
        if [[ -n "$format" ]]; then
            # If a format is specified, use it
            printf "$format" "$value"
        else
            # Otherwise, use awk to format with thousands separator
            printf "%s" "$(echo "$value" | awk '{ printf "%\047d", $0 }')"
        fi
    else
        echo "$value"
    fi
}

format_kcpu() {
    local value="$1" format="$2"
    [[ -z "$value" || "$value" == "null" || "$value" == "0" || "$value" == "0m" ]] && { echo ""; return; }
    if [[ "$value" =~ ^[0-9]+m$ ]]; then
        # Format with thousands separator
        local num_part="${value%m}"
        local formatted_num=$(echo "$num_part" | awk '{ printf "%\047d", $0 }')
        echo "${formatted_num}m"
    elif [[ "$value" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
        local num_value=$(awk "BEGIN {print $value * 1000}")
        printf "%sm" "$(echo "$num_value" | awk '{ printf "%\047d", $0 }')"
    else
        echo ""
    fi
}

format_kmem() {
    local value="$1" format="$2"
    [[ -z "$value" || "$value" == "null" || "$value" =~ ^0[MKG]$ || "$value" == "0Mi" || "$value" == "0Gi" || "$value" == "0Ki" ]] && { echo ""; return; }
    if [[ "$value" =~ ^[0-9]+[KMG]$ ]]; then
        # Format with thousands separator
        local num_part="${value%[KMG]}"
        local unit="${value: -1}"
        local formatted_num=$(echo "$num_part" | awk '{ printf "%\047d", $0 }')
        echo "${formatted_num}${unit}"
    elif [[ "$value" =~ ^[0-9]+Mi$ ]]; then
        local num_part="${value%Mi}"
        local formatted_num=$(echo "$num_part" | awk '{ printf "%\047d", $0 }')
        echo "${formatted_num}M"
    elif [[ "$value" =~ ^[0-9]+Gi$ ]]; then
        local num_part="${value%Gi}"
        local formatted_num=$(echo "$num_part" | awk '{ printf "%\047d", $0 }')
        echo "${formatted_num}G"
    elif [[ "$value" =~ ^[0-9]+Ki$ ]]; then
        local num_part="${value%Ki}"
        local formatted_num=$(echo "$num_part" | awk '{ printf "%\047d", $0 }')
        echo "${formatted_num}K"
    else
        echo ""
    fi
}

# Global arrays to store table configuration and data
declare -a HEADERS=()
declare -a KEYS=()
declare -a JUSTIFICATIONS=()
declare -a DATATYPES=()
declare -a NULL_VALUES=()
declare -a ZERO_VALUES=()
declare -a FORMATS=()
declare -a SUMMARIES=()
declare -a BREAKS=()
declare -a STRING_LIMITS=()
declare -a WRAP_MODES=()
declare -a WRAP_CHARS=()
declare -a PADDINGS=()
declare -a WIDTHS=()
declare -a SORT_KEYS=()
declare -a SORT_DIRECTIONS=()
declare -a SORT_PRIORITIES=()
declare -a ROW_JSONS=()
declare -A SUM_SUMMARIES=()
declare -A COUNT_SUMMARIES=()
declare -A MIN_SUMMARIES=()
declare -A MAX_SUMMARIES=()
declare -A UNIQUE_VALUES=()

# validate_input_files: Check if layout and data files exist and are valid
# Args: layout_file, data_file
# Returns: 0 if valid, 1 if invalid
validate_input_files() {
    local layout_file="$1" data_file="$2"
    debug_log "Validating input files: $layout_file, $data_file"
    
    if [[ ! -s "$layout_file" || ! -s "$data_file" ]]; then
        echo -e "${THEME[border_color]}Error: Layout or data JSON file is empty or missing${THEME[text_color]}" >&2
        return 1
    fi
    return 0
}

# Global variables for title and footer support
declare -g TABLE_TITLE=""
declare -g TITLE_WIDTH=0
declare -g TITLE_POSITION="none"
declare -g TABLE_FOOTER=""
declare -g FOOTER_WIDTH=0
declare -g FOOTER_POSITION="none"

# parse_layout_file: Extract theme, columns, and sort information from layout JSON
# Args: layout_file
# Side effect: Updates global THEME_NAME, COLUMN_COUNT, HEADERS, KEYS, etc.
parse_layout_file() {
    local layout_file="$1"
    debug_log "Parsing layout file: $layout_file"
    
    local columns_json sort_json
    THEME_NAME=$(jq -r '.theme // "Red"' "$layout_file")
    TABLE_TITLE=$(jq -r '.title // ""' "$layout_file")
    TITLE_POSITION=$(jq -r '.title_position // "none"' "$layout_file" | tr '[:upper:]' '[:lower:]')
    TABLE_FOOTER=$(jq -r '.footer // ""' "$layout_file")
    FOOTER_POSITION=$(jq -r '.footer_position // "none"' "$layout_file" | tr '[:upper:]' '[:lower:]')
    columns_json=$(jq -c '.columns // []' "$layout_file")
    sort_json=$(jq -c '.sort // []' "$layout_file")
    
    # Validate title position
    case "$TITLE_POSITION" in
        left|right|center|full|none) ;;
        *) 
            echo -e "${THEME[border_color]}Warning: Invalid title position '$TITLE_POSITION', using 'none'${THEME[text_color]}" >&2
            TITLE_POSITION="none"
            ;;
    esac
    
    # Validate footer position
    case "$FOOTER_POSITION" in
        left|right|center|full|none) ;;
        *) 
            echo -e "${THEME[border_color]}Warning: Invalid footer position '$FOOTER_POSITION', using 'none'${THEME[text_color]}" >&2
            FOOTER_POSITION="none"
            ;;
    esac
    
    debug_log "Theme: $THEME_NAME"
    debug_log "Title: $TABLE_TITLE"
    debug_log "Title Position: $TITLE_POSITION"
    debug_log "Footer: $TABLE_FOOTER"
    debug_log "Footer Position: $FOOTER_POSITION"
    debug_log "Columns JSON: $columns_json"
    debug_log "Sort JSON: $sort_json"
    
    if [[ -z "$columns_json" || "$columns_json" == "[]" ]]; then
        echo -e "${THEME[border_color]}Error: No columns defined in layout JSON${THEME[text_color]}" >&2
        return 1
    fi
    
    parse_column_config "$columns_json"
    parse_sort_config "$sort_json"
}

# parse_column_config: Process column configurations
# Args: columns_json
# Sets global arrays for column configuration
parse_column_config() {
    local columns_json="$1"
    debug_log "Parsing column configuration"
    
    # Clear all arrays first to ensure we start fresh
    HEADERS=()
    KEYS=()
    JUSTIFICATIONS=()
    DATATYPES=()
    NULL_VALUES=()
    ZERO_VALUES=()
    FORMATS=()
    SUMMARIES=()
    BREAKS=()
    STRING_LIMITS=()
    WRAP_MODES=()
    WRAP_CHARS=()
    WIDTHS=()
    
    local column_count=$(jq '. | length' <<<"$columns_json")
    debug_log "Column count: $column_count"
    
    # Store column count globally
    COLUMN_COUNT=$column_count
    
    for ((i=0; i<column_count; i++)); do
        local col_json
        col_json=$(jq -c ".[$i]" <<<"$columns_json")
        HEADERS[$i]=$(jq -r '.header // ""' <<<"$col_json")
        KEYS[$i]=$(jq -r '.key // (.header | ascii_downcase | gsub("[^a-z0-9]"; "_"))' <<<"$col_json")
        JUSTIFICATIONS[$i]=$(jq -r '.justification // "left"' <<<"$col_json" | tr '[:upper:]' '[:lower:]')
        DATATYPES[$i]=$(jq -r '.datatype // "text"' <<<"$col_json" | tr '[:upper:]' '[:lower:]')
        NULL_VALUES[$i]=$(jq -r '.null_value // "blank"' <<<"$col_json" | tr '[:upper:]' '[:lower:]')
        ZERO_VALUES[$i]=$(jq -r '.zero_value // "blank"' <<<"$col_json" | tr '[:upper:]' '[:lower:]')
        FORMATS[$i]=$(jq -r '.format // ""' <<<"$col_json")
        SUMMARIES[$i]=$(jq -r '.summary // "none"' <<<"$col_json" | tr '[:upper:]' '[:lower:]')
        BREAKS[$i]=$(jq -r '.break // false' <<<"$col_json")
        STRING_LIMITS[$i]=$(jq -r '.string_limit // 0' <<<"$col_json")
        WRAP_MODES[$i]=$(jq -r '.wrap_mode // "clip"' <<<"$col_json" | tr '[:upper:]' '[:lower:]')
        WRAP_CHARS[$i]=$(jq -r '.wrap_char // ""' <<<"$col_json")
        PADDINGS[$i]=$(jq -r '.padding // '$DEFAULT_PADDING <<<"$col_json")
        
        debug_log "Column $i: Header=${HEADERS[$i]}, Key=${KEYS[$i]}, Datatype=${DATATYPES[$i]}"
        
        validate_column_config "$i" "${HEADERS[$i]}" "${JUSTIFICATIONS[$i]}" "${DATATYPES[$i]}" "${SUMMARIES[$i]}"
    done
    
    # Initialize column widths based on header lengths plus padding or specified width
    for ((i=0; i<COLUMN_COUNT; i++)); do
        local col_json
        col_json=$(jq -c ".[$i]" <<<"$columns_json")
        local specified_width
        specified_width=$(jq -r '.width // 0' <<<"$col_json")
        
        if [[ $specified_width -gt 0 ]]; then
            WIDTHS[$i]=$specified_width
        else
            WIDTHS[$i]=$((${#HEADERS[$i]} + (2 * ${PADDINGS[$i]})))
        fi
        debug_log "Initial width for column $i (${HEADERS[$i]}): ${WIDTHS[$i]} (including padding ${PADDINGS[$i]})"
    done
    
    # Debug verification of array contents
    debug_log "After parse_column_config - Number of columns: $COLUMN_COUNT"
    debug_log "After parse_column_config - Headers: ${HEADERS[*]}"
    debug_log "After parse_column_config - Keys: ${KEYS[*]}"
}

# validate_column_config: Validate column configuration and print warnings
# Args: column_index, header, justification, datatype, summary
validate_column_config() {
    local i="$1" header="$2" justification="$3" datatype="$4" summary="$5"
    
    if [[ -z "$header" ]]; then
        echo -e "${THEME[border_color]}Error: Column $i has no header${THEME[text_color]}" >&2
        return 1
    fi
    
    if [[ "$justification" != "left" && "$justification" != "right" && "$justification" != "center" ]]; then
        echo -e "${THEME[border_color]}Warning: Invalid justification '$justification' for column $header, using 'left'${THEME[text_color]}" >&2
        JUSTIFICATIONS[$i]="left"
    fi
    
    if [[ -z "${DATATYPE_HANDLERS[${datatype}_validate]}" ]]; then
        echo -e "${THEME[border_color]}Warning: Invalid datatype '$datatype' for column $header, using 'text'${THEME[text_color]}" >&2
        DATATYPES[$i]="text"
    fi
    
    local valid_summaries="${DATATYPE_HANDLERS[${DATATYPES[$i]}_summary_types]}"
    if [[ "$summary" != "none" && ! " $valid_summaries " =~ " $summary " ]]; then
        echo -e "${THEME[border_color]}Warning: Summary '$summary' not supported for datatype '${DATATYPES[$i]}' in column $header, using 'none'${THEME[text_color]}" >&2
        SUMMARIES[$i]="none"
    fi
}

# parse_sort_config: Process sort configurations
# Args: sort_json
# Sets global arrays for sort configuration
parse_sort_config() {
    local sort_json="$1"
    debug_log "Parsing sort configuration"
    
    # Clear sort arrays
    SORT_KEYS=()
    SORT_DIRECTIONS=()
    SORT_PRIORITIES=()
    
    local sort_count=$(jq '. | length' <<<"$sort_json")
    debug_log "Sort count: $sort_count"
    
    for ((i=0; i<sort_count; i++)); do
        local sort_item
        sort_item=$(jq -c ".[$i]" <<<"$sort_json")
        SORT_KEYS[$i]=$(jq -r '.key // ""' <<<"$sort_item")
        SORT_DIRECTIONS[$i]=$(jq -r '.direction // "asc"' <<<"$sort_item" | tr '[:upper:]' '[:lower:]')
        SORT_PRIORITIES[$i]=$(jq -r '.priority // 0' <<<"$sort_item")
        
        debug_log "Sort $i: Key=${SORT_KEYS[$i]}, Direction=${SORT_DIRECTIONS[$i]}"
        
        if [[ -z "${SORT_KEYS[$i]}" ]]; then
            echo -e "${THEME[border_color]}Warning: Sort item $i has no key, ignoring${THEME[text_color]}" >&2
            continue
        fi
        
        if [[ "${SORT_DIRECTIONS[$i]}" != "asc" && "${SORT_DIRECTIONS[$i]}" != "desc" ]]; then
            echo -e "${THEME[border_color]}Warning: Invalid sort direction '${SORT_DIRECTIONS[$i]}' for key ${SORT_KEYS[$i]}, using 'asc'${THEME[text_color]}" >&2
            SORT_DIRECTIONS[$i]="asc"
        fi
    done
}

# initialize_summaries: Initialize summaries storage
initialize_summaries() {
    debug_log "Initializing summaries storage"
    
    # Clear summary associative arrays
    SUM_SUMMARIES=()
    COUNT_SUMMARIES=()
    MIN_SUMMARIES=()
    MAX_SUMMARIES=()
    UNIQUE_VALUES=()
    
    for ((i=0; i<COLUMN_COUNT; i++)); do
        SUM_SUMMARIES[$i]=0
        COUNT_SUMMARIES[$i]=0
        MIN_SUMMARIES[$i]=""
        MAX_SUMMARIES[$i]=""
        UNIQUE_VALUES[$i]=""
    done
}

# prepare_data: Read and validate data from JSON file
# Args: data_file
# Returns: valid data JSON
prepare_data() {
    local data_file="$1"
    debug_log "Preparing data from file: $data_file"
    
    local data_json
    data_json=$(jq -c '. // []' "$data_file")
    
    local temp_data_file=$(mktemp)
    echo "[" > "$temp_data_file"
    
    local row_count=$(jq '. | length' <<<"$data_json")
    local valid_rows=0
    debug_log "Data row count: $row_count"
    
    for ((i=0; i<row_count; i++)); do
        local row_json
        row_json=$(jq -c ".[$i]" <<<"$data_json")
        debug_log "Row $i: $row_json"
        
        if [[ $i -gt 0 && $valid_rows -gt 0 ]]; then
            echo "," >> "$temp_data_file"
        fi
        echo "$row_json" >> "$temp_data_file"
        ((valid_rows++))
    done
    
    echo "]" >> "$temp_data_file"
    debug_log "Valid rows count: $valid_rows"
    
    cat "$temp_data_file"
    rm -f "$temp_data_file"
}

# sort_data: Apply sorting to data
# Args: data_json
# Returns: sorted data JSON
sort_data() {
    local data_json="$1"
    debug_log "Sorting data"
    
    if [[ ${#SORT_KEYS[@]} -eq 0 ]]; then
        debug_log "No sort keys defined, skipping sort"
        echo "$data_json"
        return
    fi
    
    local jq_sort=""
    for i in "${!SORT_KEYS[@]}"; do
        local key="${SORT_KEYS[$i]}" dir="${SORT_DIRECTIONS[$i]}"
        
        if [[ "$dir" == "desc" ]]; then
            jq_sort+=".${key} | reverse,"
        else
            jq_sort+=".${key},"
        fi
    done
    
    jq_sort=${jq_sort%,}
    debug_log "JQ sort expression: $jq_sort"
    
    if [[ -n "$jq_sort" ]]; then
        local temp_data_file=$(mktemp)
        echo "$data_json" > "$temp_data_file"
        
        local sorted_data=$(jq -c "sort_by($jq_sort)" "$temp_data_file" 2>/tmp/jq_stderr.$$)
        local jq_exit=$?
        
        rm -f "$temp_data_file"
        
        if [[ $jq_exit -ne 0 ]]; then
            echo -e "${THEME[border_color]}Error: Sorting failed${THEME[text_color]}" >&2
            cat /tmp/jq_stderr.$$ >&2
            rm -f /tmp/jq_stderr.$$
            echo "$data_json"  # Return original data on error
        else
            debug_log "Data sorted successfully"
            echo "$sorted_data"
        fi
    else
        echo "$data_json"
    fi
}

# process_data_rows: Process data rows, update widths and calculate summaries
# Args: data_json
# Side effect: Updates global MAX_LINES
process_data_rows() {
    local data_json="$1"
    
    local row_count
    MAX_LINES=1
    row_count=$(jq '. | length' <<<"$data_json")
    
    debug_log "================ DATA PROCESSING ================"
    debug_log "Processing $row_count rows of data"
    debug_log "Number of columns: $COLUMN_COUNT"
    debug_log "Column headers: ${HEADERS[*]}"
    debug_log "Column keys: ${KEYS[*]}"
    debug_log "Initial widths: ${WIDTHS[*]}"
    
    # Clear previous row data
    ROW_JSONS=()
    
    for ((i=0; i<row_count; i++)); do
        local row_json line_count=1
        row_json=$(jq -c ".[$i]" <<<"$data_json")
        ROW_JSONS+=("$row_json")
        debug_log "Processing row $i: $row_json"
        
        for ((j=0; j<COLUMN_COUNT; j++)); do
            local key="${KEYS[$j]}" 
            local datatype="${DATATYPES[$j]}" 
            local format="${FORMATS[$j]}" 
            local string_limit="${STRING_LIMITS[$j]}" 
            local wrap_mode="${WRAP_MODES[$j]}" 
            local wrap_char="${WRAP_CHARS[$j]}"
            local validate_fn="${DATATYPE_HANDLERS[${datatype}_validate]}" 
            local format_fn="${DATATYPE_HANDLERS[${datatype}_format]}"
            local value
            
            value=$(jq -r ".${key} // null" <<<"$row_json")
            value=$("$validate_fn" "$value")
            local display_value=$("$format_fn" "$value" "$format" "$string_limit" "$wrap_mode" "$wrap_char")
            debug_log "Column $j (${HEADERS[$j]}): Raw value='$value', Formatted value='$display_value'"
            
            if [[ "$value" == "null" ]]; then
                case "${NULL_VALUES[$j]}" in
                    0) display_value="0" ;;
                    missing) display_value="Missing" ;;
                    *) display_value="" ;;
                esac
                debug_log "Null value handling: '$value' -> '$display_value'"
            elif [[ "$value" == "0" || "$value" == "0m" || "$value" == "0M" || "$value" == "0G" || "$value" == "0K" ]]; then
                case "${ZERO_VALUES[$j]}" in
                    0) display_value="0" ;;
                    missing) display_value="Missing" ;;
                    *) display_value="" ;;
                esac
                debug_log "Zero value handling: '$value' -> '$display_value'"
            fi
            
            # Update column width
            if [[ -n "$wrap_char" && "$wrap_mode" == "wrap" && -n "$display_value" && "$value" != "null" ]]; then
                local max_len=0
                local IFS="$wrap_char"
                read -ra parts <<<"$display_value"
                for part in "${parts[@]}"; do
                    local len=$(echo -n "$part" | sed 's/\x1B\[[0-9;]*m//g' | wc -c)
                    # Don't decrease length - we need the actual character count
                    [[ $len -gt $max_len ]] && max_len=$len
                done
                local padded_width=$((max_len + (2 * ${PADDINGS[$j]})))
                [[ $padded_width -gt ${WIDTHS[$j]} ]] && WIDTHS[$j]=$padded_width
                [[ ${#parts[@]} -gt $line_count ]] && line_count=${#parts[@]}
                debug_log "Wrapped value: parts=${#parts[@]}, max_len=$max_len, new width=${WIDTHS[$j]}"
            else
                local len=$(echo -n "$display_value" | sed 's/\x1B\[[0-9;]*m//g' | wc -c)
                # Don't decrease length - we need the actual character count
                local padded_width=$((len + (2 * ${PADDINGS[$j]})))
                [[ $padded_width -gt ${WIDTHS[$j]} ]] && WIDTHS[$j]=$padded_width
                debug_log "Plain value: len=$len, new width=${WIDTHS[$j]}"
            fi
            
            # Update summaries
            update_summaries "$j" "$value" "${DATATYPES[$j]}" "${SUMMARIES[$j]}"
        done
        
        [[ $line_count -gt $MAX_LINES ]] && MAX_LINES=$line_count
    done
    
    # After processing all rows, check if summaries need wider columns
    for ((j=0; j<COLUMN_COUNT; j++)); do
        if [[ "${SUMMARIES[$j]}" != "none" ]]; then
            local summary_value="" datatype="${DATATYPES[$j]}" format="${FORMATS[$j]}"
            
            # Calculate the expected summary value based on summary type
            case "${SUMMARIES[$j]}" in
                sum)
                    if [[ -n "${SUM_SUMMARIES[$j]}" && "${SUM_SUMMARIES[$j]}" != "0" ]]; then
                        if [[ "$datatype" == "kcpu" ]]; then
                            # Format kcpu summary with thousands separator for width calculation
                            local formatted_num=$(echo "${SUM_SUMMARIES[$j]}" | awk '{ printf "%\047d", $0 }')
                            summary_value="${formatted_num}m"
                        elif [[ "$datatype" == "kmem" ]]; then
                            # Format kmem summary with thousands separator for width calculation
                            local formatted_num=$(echo "${SUM_SUMMARIES[$j]}" | awk '{ printf "%\047d", $0 }')
                            summary_value="${formatted_num}M"
                        elif [[ "$datatype" == "num" ]]; then
                            # For num datatype, use format_num to apply thousands separator
                            summary_value=$(format_num "${SUM_SUMMARIES[$j]}" "$format")
                        elif [[ "$datatype" == "int" || "$datatype" == "float" ]]; then
                            summary_value="${SUM_SUMMARIES[$j]}"
                            [[ -n "$format" ]] && summary_value=$(printf "$format" "$summary_value")
                        fi
                    fi
                    ;;
                min)
                    summary_value="${MIN_SUMMARIES[$j]:-}"
                    [[ -n "$format" ]] && summary_value=$(printf "$format" "$summary_value")
                    ;;
                max)
                    summary_value="${MAX_SUMMARIES[$j]:-}"
                    [[ -n "$format" ]] && summary_value=$(printf "$format" "$summary_value")
                    ;;
                count)
                    summary_value="${COUNT_SUMMARIES[$j]:-0}"
                    ;;
                unique)
                    if [[ -n "${UNIQUE_VALUES[$j]}" ]]; then
                        local unique_count=$(echo "${UNIQUE_VALUES[$j]}" | tr ' ' '\n' | sort -u | wc -l | awk '{print $1}')
                        summary_value="$unique_count"
                    else
                        summary_value="0"
                    fi
                    ;;
            esac
            
            # If summary exists, check if its width requires column adjustment
            if [[ -n "$summary_value" ]]; then
                local summary_len=$(echo -n "$summary_value" | sed 's/\x1B\[[0-9;]*m//g' | wc -c)
                local summary_padded_width=$((summary_len + (2 * ${PADDINGS[$j]})))
                
                # Update column width if summary needs more space
                if [[ $summary_padded_width -gt ${WIDTHS[$j]} ]]; then
                    debug_log "Column $j (${HEADERS[$j]}): Adjusting width for summary value '$summary_value', new width=$summary_padded_width"
                    WIDTHS[$j]=$summary_padded_width
                fi
            fi
        fi
    done
    
    debug_log "Final column widths after summary adjustment: ${WIDTHS[*]}"
    debug_log "Max lines per row: $MAX_LINES"
    debug_log "Total rows to render: ${#ROW_JSONS[@]}"
}

# update_summaries: Update summaries for a column
# Args: column_index, value, datatype, summary_type
update_summaries() {
    local j="$1" value="$2" datatype="$3" summary_type="$4"
    
    case "$summary_type" in
        sum)
            if [[ "$datatype" == "kcpu" && "$value" =~ ^[0-9]+m$ ]]; then
                SUM_SUMMARIES[$j]=$(( ${SUM_SUMMARIES[$j]:-0} + ${value%m} ))
            elif [[ "$datatype" == "kmem" ]]; then
                if [[ "$value" =~ ^[0-9]+M$ ]]; then
                    SUM_SUMMARIES[$j]=$(( ${SUM_SUMMARIES[$j]:-0} + ${value%M} ))
                elif [[ "$value" =~ ^[0-9]+G$ ]]; then
                    SUM_SUMMARIES[$j]=$(( ${SUM_SUMMARIES[$j]:-0} + ${value%G} * 1000 ))
                elif [[ "$value" =~ ^[0-9]+K$ ]]; then
                    SUM_SUMMARIES[$j]=$(( ${SUM_SUMMARIES[$j]:-0} + ${value%K} / 1000 ))
                elif [[ "$value" =~ ^[0-9]+Mi$ ]]; then
                    SUM_SUMMARIES[$j]=$(( ${SUM_SUMMARIES[$j]:-0} + ${value%Mi} ))
                elif [[ "$value" =~ ^[0-9]+Gi$ ]]; then
                    SUM_SUMMARIES[$j]=$(( ${SUM_SUMMARIES[$j]:-0} + ${value%Gi} * 1000 ))
                fi
    elif [[ "$datatype" == "int" || "$datatype" == "float" || "$datatype" == "num" ]]; then
        if [[ "$value" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
            SUM_SUMMARIES[$j]=$(awk "BEGIN {print (${SUM_SUMMARIES[$j]:-0} + $value)}")
        fi
            fi
            ;;
        min)
            if [[ "$value" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
                if [[ -z "${MIN_SUMMARIES[$j]}" || $(awk "BEGIN {print $value < ${MIN_SUMMARIES[$j]}}") -eq 1 ]]; then
                    MIN_SUMMARIES[$j]="$value"
                fi
            fi
            ;;
        max)
            if [[ "$value" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
                if [[ -z "${MAX_SUMMARIES[$j]}" || $(awk "BEGIN {print $value > ${MAX_SUMMARIES[$j]}}") -eq 1 ]]; then
                    MAX_SUMMARIES[$j]="$value"
                fi
            fi
            ;;
        count)
            if [[ -n "$value" && "$value" != "null" ]]; then
                COUNT_SUMMARIES[$j]=$(( ${COUNT_SUMMARIES[$j]:-0} + 1 ))
            fi
            ;;
        unique)
            [[ -n "$value" && "$value" != "null" ]] && UNIQUE_VALUES[$j]+=" $value"
            ;;
    esac
}

# calculate_title_width: Calculate the width needed for the title
# Returns: width of the title section
calculate_title_width() {
    local title="$1" total_table_width="$2"
    
    if [[ -n "$title" ]]; then
        if [[ "$TITLE_POSITION" == "none" ]]; then
            # Default behavior - title width is the length of the title plus padding on both sides
            TITLE_WIDTH=$((${#title} + (2 * DEFAULT_PADDING)))
        elif [[ "$TITLE_POSITION" == "full" ]]; then
            # Full alignment - title width matches table width
            TITLE_WIDTH=$total_table_width
        else
            # For left, right, center - title width is the length of the title plus padding, clipped to table width
            TITLE_WIDTH=$((${#title} + (2 * DEFAULT_PADDING)))
            [[ $TITLE_WIDTH -gt $total_table_width ]] && TITLE_WIDTH=$total_table_width
        fi
    else
        TITLE_WIDTH=0
    fi
}

# calculate_footer_width: Calculate the width needed for the footer
# Returns: width of the footer section
calculate_footer_width() {
    local footer="$1" total_table_width="$2"
    
    if [[ -n "$footer" ]]; then
        if [[ "$FOOTER_POSITION" == "none" ]]; then
            # Default behavior - footer width is the length of the footer plus padding on both sides
            FOOTER_WIDTH=$((${#footer} + (2 * DEFAULT_PADDING)))
        elif [[ "$FOOTER_POSITION" == "full" ]]; then
            # Full alignment - footer width matches table width
            FOOTER_WIDTH=$total_table_width
        else
            # For left, right, center - footer width is the length of the footer plus padding, clipped to table width
            FOOTER_WIDTH=$((${#footer} + (2 * DEFAULT_PADDING)))
            [[ $FOOTER_WIDTH -gt $total_table_width ]] && FOOTER_WIDTH=$total_table_width
        fi
    else
        FOOTER_WIDTH=0
    fi
}

# render_table_title: Render the title section if a title is specified
render_table_title() {
    local total_table_width="$1"
    
    if [[ -n "$TABLE_TITLE" ]]; then
        debug_log "Rendering table title: $TABLE_TITLE with position: $TITLE_POSITION"
        calculate_title_width "$TABLE_TITLE" "$total_table_width"
        
        local offset=0
        case "$TITLE_POSITION" in
            left)
                offset=0
                ;;
            right)
                offset=$((total_table_width - TITLE_WIDTH))
                ;;
            center)
                offset=$(((total_table_width - TITLE_WIDTH) / 2))
                ;;
            full)
                offset=0
                ;;
            *)
                offset=0
                ;;
        esac
        
        # Render title top border with offset
        if [[ $offset -gt 0 ]]; then
            printf "%*s" "$offset" ""
        fi
        printf "${THEME[border_color]}${THEME[tl_corner]}"
        printf "${THEME[h_line]}%.0s" $(seq 1 $TITLE_WIDTH)
        printf "${THEME[tr_corner]}${THEME[text_color]}\n"
        
        # Render title text with appropriate alignment
        if [[ $offset -gt 0 ]]; then
            printf "%*s" "$offset" ""
        fi
        printf "${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}"
        
        local available_width=$((TITLE_WIDTH - (2 * DEFAULT_PADDING)))
        local title_text="$TABLE_TITLE"
        
        # Clip text if it exceeds available width
        if [[ ${#title_text} -gt $available_width ]]; then
            title_text="${title_text:0:$available_width}"
        fi
        
        case "$TITLE_POSITION" in
            left)
                # Left align - no additional offset
                printf "%*s${THEME[header_color]}%-*s${THEME[text_color]}%*s" \
                      "$DEFAULT_PADDING" "" "$available_width" "$title_text" "$DEFAULT_PADDING" ""
                ;;
            right)
                # Right align - already offset
                printf "%*s${THEME[header_color]}%*s${THEME[text_color]}%*s" \
                      "$DEFAULT_PADDING" "" "$available_width" "$title_text" "$DEFAULT_PADDING" ""
                ;;
            center)
                # Center align - already offset
                printf "%*s${THEME[header_color]}%s${THEME[text_color]}%*s" \
                      "$DEFAULT_PADDING" "" "$title_text" "$((available_width - ${#title_text} + DEFAULT_PADDING))" ""
                ;;
            full)
                # Full alignment - center text within full table width
                local text_len=${#title_text}
                local spaces=$(( (available_width - text_len) / 2 ))
                local left_spaces=$(( DEFAULT_PADDING + spaces ))
                local right_spaces=$(( DEFAULT_PADDING + available_width - text_len - spaces ))
                printf "%*s${THEME[header_color]}%s${THEME[text_color]}%*s" \
                      "$left_spaces" "" "$title_text" "$right_spaces" ""
                ;;
            *)
                # Default (none) - original behavior
                printf "%*s${THEME[header_color]}%s${THEME[text_color]}%*s" \
                      "$DEFAULT_PADDING" "" "$title_text" "$DEFAULT_PADDING" ""
                ;;
        esac
        
        printf "${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}\n"
    fi
}

# render_table_border: Shared function to render table borders (top or bottom)
# Args: border_type ("top" or "bottom"), total_table_width, element_offset, element_right_edge, element_width, element_position, is_element_full
# Logic for rendering the border:
# 1. Determine the maximum width to render, which is the longer of the table width or the element (title/footer) width, ensuring that wider elements extend the full line.
# 2. Calculate positions of column separators within the table width, adjusting for the left border to ensure accurate alignment.
# 3. For each position in the border, determine the appropriate character to use:
#    a. For the start and end of the entire line, use the appropriate corner characters.
#    b. For column separators within the table, use the appropriate junction characters.
#    c. For the start and end of the element (title/footer), if present, use the appropriate junction characters.
#    d. For all other positions, use the horizontal line character.
# 4. Print the border line character by character, using the determined characters.
render_table_border() {
    local border_type="$1"
    local total_table_width="$2"
    local element_offset="$3"
    local element_right_edge="$4"
    local element_width="$5"
    local element_position="$6"
    local is_element_full="$7"
    
    debug_log "Rendering $border_type border with total width: $total_table_width, element offset: $element_offset, element right edge: $element_right_edge, element width: $element_width"
    
    # Calculate positions of all column separators
    local column_widths_sum=0
    local column_positions=()
    for ((i=0; i<COLUMN_COUNT-1; i++)); do
        column_widths_sum=$((column_widths_sum + WIDTHS[$i]))
        column_positions+=($column_widths_sum)
        ((column_widths_sum++))  # +1 for the separator
    done
    
    # Determine maximum width to render (longer of table width or element width if present)
    # Add 2 to account for left and right border characters
    local max_width=$((total_table_width + 2))
    if [[ -n "$element_width" && $element_width -gt 0 ]]; then
        # If element width is provided, compare with table width to determine max width
        local adjusted_element_width=$((element_width + 2))
        if [[ $adjusted_element_width -gt $max_width ]]; then
            max_width=$adjusted_element_width
        fi
    fi
    
    debug_log "Max width for $border_type border: $max_width"
    
    # Print the border line character by character
    printf "${THEME[border_color]}"
    for ((i=0; i<max_width; i++)); do
        local char_to_print="${THEME[h_line]}"  # Default to horizontal line character
        
        # Determine the appropriate character for this position
        if [[ $i -eq 0 ]]; then
            # Left edge of the table
            if [[ -n "$element_width" && $element_width -gt 0 && $element_offset -eq 0 ]]; then
                # If header/footer starts at the left edge, use left junction
                char_to_print="${THEME[l_junct]}"
            elif [[ "$border_type" == "top" ]]; then
                char_to_print="${THEME[tl_corner]}"
            else
                char_to_print="${THEME[bl_corner]}"
            fi
        elif [[ $i -eq $((max_width - 1)) ]]; then
            # Right edge of the table
            if [[ -n "$element_width" && $element_width -gt 0 ]]; then
                if [[ $element_right_edge -gt $total_table_width ]]; then
                    # If header/footer extends beyond the right edge
                    if [[ "$border_type" == "top" ]]; then
                        # For top border with header wider than table, use bottom-right corner
                        char_to_print="${THEME[br_corner]}"
                    else
                        # For bottom border with footer wider than table, use top-right corner
                        char_to_print="${THEME[tr_corner]}"
                    fi
                elif [[ $element_right_edge -eq $total_table_width ]]; then
                    # If header/footer width is exactly the table width, use right junction
                    char_to_print="${THEME[r_junct]}"
                elif [[ "$border_type" == "top" ]]; then
                    char_to_print="${THEME[tr_corner]}"
                else
                    char_to_print="${THEME[br_corner]}"
                fi
            elif [[ "$border_type" == "top" ]]; then
                char_to_print="${THEME[tr_corner]}"
            else
                char_to_print="${THEME[br_corner]}"
            fi
        elif [[ $i -eq $((total_table_width + 1)) && $i -lt $((max_width - 1)) ]]; then
            # Right edge of table when header/footer is wider
            if [[ -n "$element_width" && $element_width -gt 0 && $element_right_edge -gt $((total_table_width - 1)) ]]; then
                # Header/footer is wider than the table
                if [[ "$border_type" == "top" ]]; then
                    # For top border with header wider than table, use top junction
                    char_to_print="${THEME[t_junct]}"
                else
                    # For bottom border with footer wider than table, use bottom junction
                    char_to_print="${THEME[b_junct]}"
                fi
            else
                char_to_print="${THEME[r_junct]}"
            fi
        elif [[ $i -eq $element_offset && -n "$element_width" && $element_offset -gt 0 && $element_offset -lt $((total_table_width + 1)) ]]; then
            # Start of header/footer within table bounds
            # Check if this position is also a column separator
            local is_column_separator=false
            for pos in "${column_positions[@]}"; do
                local adjusted_pos=$((pos + 1))  # Adjust for the left border character
                if [[ $i -eq $adjusted_pos ]]; then
                    is_column_separator=true
                    break
                fi
            done
            
            if [[ "$is_column_separator" == "true" ]]; then
                # If it's also a column separator, use cross
                char_to_print="${THEME[cross]}"
            elif [[ "$border_type" == "top" ]]; then
                # For top border with header not on the left, use bottom junction
                char_to_print="${THEME[b_junct]}"
            else
                # For bottom border with footer not on the left, use top junction
                char_to_print="${THEME[t_junct]}"
            fi
        elif [[ $i -eq $((element_right_edge + 1)) && -n "$element_width" && $((element_right_edge + 1)) -lt $((total_table_width + 1)) ]]; then
            # End of header/footer within table bounds
            # Check if this position is also a column separator
            local is_column_separator=false
            for pos in "${column_positions[@]}"; do
                local adjusted_pos=$((pos + 1))  # Adjust for the left border character
                if [[ $i -eq $adjusted_pos ]]; then
                    is_column_separator=true
                    break
                fi
            done
            
            if [[ "$is_column_separator" == "true" ]]; then
                # If it's also a column separator, use cross
                char_to_print="${THEME[cross]}"
            elif [[ "$border_type" == "top" ]]; then
                # For top border with title ending before table end, use bottom junction
                char_to_print="${THEME[b_junct]}"
            else
                # For bottom border with footer ending before table end, use top junction
                char_to_print="${THEME[t_junct]}"
            fi
        else
            # Check if this position is a column separator
            local is_column_separator=false
            for pos in "${column_positions[@]}"; do
                local adjusted_pos=$((pos + 1))  # Adjust for the left border character
                if [[ $i -eq $adjusted_pos ]]; then
                    is_column_separator=true
                    break
                fi
            done
            
            if [[ "$is_column_separator" == "true" ]]; then
                # Column separator
                if [[ "$border_type" == "top" ]]; then
                    char_to_print="${THEME[t_junct]}"
                else
                    char_to_print="${THEME[b_junct]}"
                fi
            elif [[ "$border_type" == "bottom" && -n "$element_width" && $element_width -gt 0 ]]; then
                # For bottom border with footer, check if this position needs a connector
                if [[ $i -eq $element_offset && $element_offset -ge 0 ]]; then
                    # Start of footer
                    char_to_print="${THEME[l_junct]}"
                elif [[ $i -eq $((element_right_edge + 1)) && $((element_right_edge + 1)) -lt $max_width ]]; then
                    # End of footer
                    char_to_print="${THEME[r_junct]}"
                fi
            fi
            
            # Special case for perfect title width and wide title
            if [[ "$border_type" == "top" && -n "$element_width" && $element_width -gt 0 ]]; then
                if [[ $element_right_edge -eq $((total_table_width - 1)) && $i -gt $((total_table_width + 1)) ]]; then
                    # Perfect title width - remove any extra characters at the end
                    char_to_print=""
                fi
            fi
        fi
        
        printf "%s" "$char_to_print"
    done
    printf "${THEME[text_color]}\n"
}

# render_table_top_border: Render the top border of the table
render_table_top_border() {
    debug_log "Rendering top border"
    
    local total_table_width=0
    for ((i=0; i<COLUMN_COUNT; i++)); do
        ((total_table_width += WIDTHS[$i]))
        [[ $i -lt $((COLUMN_COUNT-1)) ]] && ((total_table_width++))
    done
    
    local title_offset=0
    local title_right_edge=0
    local title_width=""
    local title_position="none"
    if [[ -n "$TABLE_TITLE" ]]; then
        title_width=$TITLE_WIDTH
        title_position=$TITLE_POSITION
        case "$TITLE_POSITION" in
            left)
                title_offset=0
                title_right_edge=$TITLE_WIDTH
                ;;
            right)
                title_offset=$((total_table_width - TITLE_WIDTH))
                title_right_edge=$total_table_width
                ;;
            center)
                title_offset=$(((total_table_width - TITLE_WIDTH) / 2))
                title_right_edge=$((title_offset + TITLE_WIDTH))
                ;;
            full)
                title_offset=0
                title_right_edge=$total_table_width
                ;;
            *)
                title_offset=0
                title_right_edge=$TITLE_WIDTH
                ;;
        esac
    fi
    
    # Call shared border rendering function for top border
    render_table_border "top" "$total_table_width" "$title_offset" "$title_right_edge" "$title_width" "$title_position" "$([[ "$title_position" == "full" ]] && echo true || echo false)"
    
    # If title is full width, render the title text and a connecting line
    if [[ -n "$TABLE_TITLE" && "$TITLE_POSITION" == "full" ]]; then
        # Print the title text
        printf "${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}"
        
        local available_width=$((total_table_width - (2 * DEFAULT_PADDING)))
        local title_text="$TABLE_TITLE"
        
        # Clip text if it exceeds available width
        if [[ ${#title_text} -gt $available_width ]]; then
            title_text="${title_text:0:$available_width}"
        fi
        
        # Center the text
        local text_len=${#title_text}
        local spaces=$(( (available_width - text_len) / 2 ))
        local left_spaces=$(( DEFAULT_PADDING + spaces ))
        local right_spaces=$(( DEFAULT_PADDING + available_width - text_len - spaces ))
        printf "%*s${THEME[header_color]}%s${THEME[text_color]}%*s" \
              "$left_spaces" "" "$title_text" "$right_spaces" ""
        
        printf "${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}\n"
        
        # Print the connecting line using the shared function
        render_table_border "top" "$total_table_width" "0" "$total_table_width" "$total_table_width" "full" "true"
    fi
}

# render_table_bottom_border: Render the bottom border of the table
render_table_bottom_border() {
    debug_log "Rendering bottom border"
    
    local total_table_width=0
    for ((i=0; i<COLUMN_COUNT; i++)); do
        ((total_table_width += WIDTHS[$i]))
        [[ $i -lt $((COLUMN_COUNT-1)) ]] && ((total_table_width++))
    done
    
    local footer_offset=0
    local footer_right_edge=0
    local footer_width=""
    local footer_position="none"
    if [[ -n "$TABLE_FOOTER" ]]; then
        # Calculate footer width before rendering border
        calculate_footer_width "$TABLE_FOOTER" "$total_table_width"
        footer_width=$FOOTER_WIDTH
        footer_position=$FOOTER_POSITION
        case "$FOOTER_POSITION" in
            left)
                footer_offset=0
                footer_right_edge=$FOOTER_WIDTH
                ;;
            right)
                footer_offset=$((total_table_width - FOOTER_WIDTH))
                footer_right_edge=$total_table_width
                ;;
            center)
                footer_offset=$(((total_table_width - FOOTER_WIDTH) / 2))
                footer_right_edge=$((footer_offset + FOOTER_WIDTH))
                ;;
            full)
                footer_offset=0
                footer_right_edge=$total_table_width
                ;;
            *)
                footer_offset=0
                footer_right_edge=$FOOTER_WIDTH
                ;;
        esac
        debug_log "Footer calculated: width=$footer_width, offset=$footer_offset, right_edge=$footer_right_edge, position=$footer_position"
    fi
    
    # Call shared border rendering function for bottom border with footer details to mark connectors
    render_table_border "bottom" "$total_table_width" "$footer_offset" "$footer_right_edge" "$footer_width" "$footer_position" "$([[ "$footer_position" == "full" ]] && echo true || echo false)"
}

# render_table_headers: Render the table headers row
render_table_headers() {
    debug_log "Rendering table headers"
    
    printf "${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}"
    for ((i=0; i<COLUMN_COUNT; i++)); do
        debug_log "Rendering header $i: ${HEADERS[$i]}, width=${WIDTHS[$i]}, justification=${JUSTIFICATIONS[$i]}"
        
        case "${JUSTIFICATIONS[$i]}" in
            right)
                # Total available width minus padding on both sides
                local content_width=$((WIDTHS[$i] - (2 * PADDINGS[$i])))
                printf "%*s${THEME[caption_color]}%*s${THEME[text_color]}%*s${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}" \
                      "${PADDINGS[$i]}" "" "${content_width}" "${HEADERS[$i]}" "${PADDINGS[$i]}" ""
                ;;
            center)
                local content_width=$((WIDTHS[$i] - (2 * PADDINGS[$i])))
                local header_spaces=$(( (content_width - ${#HEADERS[$i]}) / 2 ))
                local left_spaces=$(( PADDINGS[$i] + header_spaces ))
                local right_spaces=$(( PADDINGS[$i] + content_width - ${#HEADERS[$i]} - header_spaces ))
                printf "%*s${THEME[caption_color]}%s${THEME[text_color]}%*s${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}" \
                      "${left_spaces}" "" "${HEADERS[$i]}" "${right_spaces}" ""
                ;;
            *)
                printf "%*s${THEME[caption_color]}%-*s${THEME[text_color]}%*s${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}" \
                      "${PADDINGS[$i]}" "" "$((WIDTHS[$i] - (2 * PADDINGS[$i])))" "${HEADERS[$i]}" "${PADDINGS[$i]}" ""
                ;;
        esac
    done
    printf "\n"
}

# render_table_separator: Render a separator line in the table
# Args: type (middle/bottom)
render_table_separator() {
    local type="$1"
    debug_log "Rendering table separator: $type"
    
    local left_char="${THEME[l_junct]}" right_char="${THEME[r_junct]}" middle_char="${THEME[cross]}"
    [[ "$type" == "bottom" ]] && left_char="${THEME[bl_corner]}" && right_char="${THEME[br_corner]}" && middle_char="${THEME[b_junct]}"
    
    printf "${THEME[border_color]}${left_char}"
    for ((i=0; i<COLUMN_COUNT; i++)); do
        # Ensure we have the correct number of horizontal line characters for each column width
        local width=${WIDTHS[$i]}
        for ((j=0; j<width; j++)); do
            printf "${THEME[h_line]}"
        done
        [[ $i -lt $((COLUMN_COUNT-1)) ]] && printf "${middle_char}"
    done
    printf "${right_char}${THEME[text_color]}\n"
}

# format_display_value: Format a cell value for display
# Args: value, null_value, zero_value, datatype, format, string_limit, wrap_mode, wrap_char
# Returns: formatted display value
format_display_value() {
    local value="$1" null_value="$2" zero_value="$3" datatype="$4" format="$5" string_limit="$6" wrap_mode="$7" wrap_char="$8"
    
    local validate_fn="${DATATYPE_HANDLERS[${datatype}_validate]}" format_fn="${DATATYPE_HANDLERS[${datatype}_format]}"
    value=$("$validate_fn" "$value")
    local display_value=$("$format_fn" "$value" "$format" "$string_limit" "$wrap_mode" "$wrap_char")
    
    if [[ "$value" == "null" ]]; then
        case "$null_value" in
            0) display_value="0" ;;
            missing) display_value="Missing" ;;
            *) display_value="" ;;
        esac
    elif [[ "$value" == "0" || "$value" == "0m" || "$value" == "0M" || "$value" == "0G" || "$value" == "0K" ]]; then
        case "$zero_value" in
            0) display_value="0" ;;
            missing) display_value="Missing" ;;
            *) display_value="" ;;
        esac
    fi
    
    echo "$display_value"
}

# render_data_rows: Render the data rows of the table
# Args: max_lines
render_data_rows() {
    local max_lines="$1"
    debug_log "================ RENDERING DATA ROWS ================"
    debug_log "Rendering ${#ROW_JSONS[@]} rows with max_lines=$max_lines"
    debug_log "Number of columns when rendering: $COLUMN_COUNT"
    debug_log "Column headers when rendering: ${HEADERS[*]}"
    
    # Initialize last break values
    local last_break_values=()
    for ((j=0; j<COLUMN_COUNT; j++)); do
        last_break_values[$j]=""
    done
    
    # Process each row in order
    for ((row_idx=0; row_idx<${#ROW_JSONS[@]}; row_idx++)); do
        local row_json="${ROW_JSONS[$row_idx]}"
        debug_log "Rendering row $row_idx: $row_json"
        
        # Check if we need a break
        local needs_break=false
        for ((j=0; j<COLUMN_COUNT; j++)); do
            if [[ "${BREAKS[$j]}" == "true" ]]; then
                local key="${KEYS[$j]}" value
                value=$(jq -r ".${key} // \"\"" <<<"$row_json")
                if [[ -n "${last_break_values[$j]}" && "$value" != "${last_break_values[$j]}" ]]; then
                    needs_break=true
                    debug_log "Break detected for column $j: '${last_break_values[$j]}' -> '$value'"
                    break
                fi
            fi
        done
        
        if [[ "$needs_break" == "true" ]]; then
            render_table_separator "middle"
        fi
        
        # Prepare values for all lines
        local -A line_values
        local row_line_count=1
        for ((j=0; j<COLUMN_COUNT; j++)); do
            local key="${KEYS[$j]}" value
            value=$(jq -r ".${key} // null" <<<"$row_json")
            debug_log "Row $row_idx, Column $j (${HEADERS[$j]}): Raw value='$value'"
            
            local display_value=$(format_display_value "$value" "${NULL_VALUES[$j]}" "${ZERO_VALUES[$j]}" "${DATATYPES[$j]}" "${FORMATS[$j]}" "${STRING_LIMITS[$j]}" "${WRAP_MODES[$j]}" "${WRAP_CHARS[$j]}")
            debug_log "Row $row_idx, Column $j (${HEADERS[$j]}): Display value='$display_value'"
            
            if [[ -n "${WRAP_CHARS[$j]}" && "${WRAP_MODES[$j]}" == "wrap" && -n "$display_value" && "$value" != "null" ]]; then
                local IFS="${WRAP_CHARS[$j]}"
                read -ra parts <<<"$display_value"
                debug_log "Wrapping value into ${#parts[@]} parts"
                for k in "${!parts[@]}"; do
                    line_values[$j,$k]="${parts[k]}"
                    debug_log "Row $row_idx, Column $j, Line $k: '${parts[k]}'"
                done
                # Track the maximum number of lines needed for this row
                [[ ${#parts[@]} -gt $row_line_count ]] && row_line_count=${#parts[@]}
            else
                line_values[$j,0]="$display_value"
                debug_log "Row $row_idx, Column $j, Line 0: '$display_value'"
            fi
        done
        
        debug_log "Row $row_idx needs $row_line_count lines"
        
        # Render each line of the row, but only up to the number of lines needed for this specific row
        for ((line=0; line<row_line_count; line++)); do
            debug_log "Rendering row $row_idx, line $line"
            printf "${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}"
            for ((j=0; j<COLUMN_COUNT; j++)); do
                local display_value="${line_values[$j,$line]:-}"
                debug_log "Cell value for row $row_idx, column $j, line $line: '$display_value'"
                
                case "${JUSTIFICATIONS[$j]}" in
                    right)
                        # For right-justified text, calculate exact content space needed
                        local content_width=$((WIDTHS[$j] - (2 * PADDINGS[$j])))
                        # Use full column width regardless of whether display_value is empty
                        printf "%*s%*s%*s${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}" \
                              "${PADDINGS[$j]}" "" "${content_width}" "$display_value" "${PADDINGS[$j]}" ""
                        ;;
                    center)
                        local content_width=$((WIDTHS[$j] - (2 * PADDINGS[$j])))
                        if [[ -z "$display_value" ]]; then
                            # For empty cells, just pad to full width
                            printf "%*s%*s${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}" \
                                  "${PADDINGS[$j]}" "" "$((WIDTHS[$j] - (2 * PADDINGS[$j]) + PADDINGS[$j]))" ""
                        else
                            # For cells with content, center the text
                            local value_spaces=$(( (content_width - ${#display_value}) / 2 ))
                            local left_spaces=$(( PADDINGS[$j] + value_spaces ))
                            local right_spaces=$(( PADDINGS[$j] + content_width - ${#display_value} - value_spaces ))
                            printf "%*s%s%*s${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}" \
                                  "${left_spaces}" "" "$display_value" "${right_spaces}" ""
                        fi
                        ;;
                    *)
                        # For left-justified text, ensure we use the full column width
                        printf "%*s%-*s%*s${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}" \
                              "${PADDINGS[$j]}" "" "$((WIDTHS[$j] - (2 * PADDINGS[$j])))" "$display_value" "${PADDINGS[$j]}" ""
                        ;;
                esac
            done
            printf "\n"
        done
        
        # Update break values for the next iteration
        for ((j=0; j<COLUMN_COUNT; j++)); do
            if [[ "${BREAKS[$j]}" == "true" ]]; then
                local key="${KEYS[$j]}" value
                value=$(jq -r ".${key} // \"\"" <<<"$row_json")
                last_break_values[$j]="$value"
            fi
        done
    done
}

# render_table_footer: Render the footer section if a footer is specified
# Args: total_table_width - the total width of the table
# Note: The top border of the footer is not rendered here as it is handled by render_table_bottom_border()
#       to ensure connectors are marked on the table's bottom border.
render_table_footer() {
    local total_table_width="$1"
    
    if [[ -n "$TABLE_FOOTER" ]]; then
        debug_log "Rendering table footer: $TABLE_FOOTER with position: $FOOTER_POSITION"
        calculate_footer_width "$TABLE_FOOTER" "$total_table_width"
        
        local footer_offset=0
        case "$FOOTER_POSITION" in
            left)
                footer_offset=0
                ;;
            right)
                footer_offset=$((total_table_width - FOOTER_WIDTH))
                ;;
            center)
                footer_offset=$(((total_table_width - FOOTER_WIDTH) / 2))
                ;;
            full)
                footer_offset=0
                ;;
            *)
                footer_offset=0
                ;;
        esac
        
        # Render footer text with appropriate alignment
        if [[ $footer_offset -gt 0 ]]; then
            printf "%*s" "$footer_offset" ""
        fi
        
        local available_width=$((FOOTER_WIDTH - (2 * DEFAULT_PADDING)))
        local footer_text="$TABLE_FOOTER"
        
        # Clip text if it exceeds available width
        if [[ ${#footer_text} -gt $available_width ]]; then
            footer_text="${footer_text:0:$available_width}"
        fi
        
        case "$FOOTER_POSITION" in
            left)
                # Left align - no additional offset
                printf "${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}%*s${THEME[footer_color]}%-*s${THEME[text_color]}%*s${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}" \
                      "$DEFAULT_PADDING" "" "$available_width" "$footer_text" "$DEFAULT_PADDING" ""
                ;;
            right)
                # Right align - already offset
                printf "${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}%*s${THEME[footer_color]}%*s${THEME[text_color]}%*s${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}" \
                      "$DEFAULT_PADDING" "" "$available_width" "$footer_text" "$DEFAULT_PADDING" ""
                ;;
            center)
                # Center align - already offset
                printf "${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}%*s${THEME[footer_color]}%s${THEME[text_color]}%*s${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}" \
                      "$DEFAULT_PADDING" "" "$footer_text" "$((available_width - ${#footer_text} + DEFAULT_PADDING))" ""
                ;;
            full)
                # Full alignment - center text within full table width
                local text_len=${#footer_text}
                local spaces=$(( (available_width - text_len) / 2 ))
                local left_spaces=$(( DEFAULT_PADDING + spaces ))
                local right_spaces=$(( DEFAULT_PADDING + available_width - text_len - spaces ))
                printf "${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}%*s${THEME[footer_color]}%s${THEME[text_color]}%*s${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}" \
                      "$left_spaces" "" "$footer_text" "$right_spaces" ""
                ;;
            *)
                # Default (none) - original behavior
                printf "${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}%*s${THEME[footer_color]}%s${THEME[text_color]}%*s${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}" \
                      "$DEFAULT_PADDING" "" "$footer_text" "$DEFAULT_PADDING" ""
                ;;
        esac
        
        printf "\n"
        
        # Render footer bottom border
        if [[ $footer_offset -gt 0 ]]; then
            printf "%*s" "$footer_offset" ""
        fi
        printf "${THEME[border_color]}${THEME[bl_corner]}"
        printf "${THEME[h_line]}%.0s" $(seq 1 $FOOTER_WIDTH)
        printf "${THEME[br_corner]}${THEME[text_color]}\n"
    fi
}

# render_summaries_row: Render the summaries row if any summaries are defined with special coloring
# Returns: 0 if summaries were rendered, 1 otherwise
render_summaries_row() {
    debug_log "Checking if summaries row should be rendered"
    
    # Check if any summaries are defined
    local has_summaries=false
    for ((i=0; i<COLUMN_COUNT; i++)); do
        [[ "${SUMMARIES[$i]}" != "none" ]] && has_summaries=true && break
    done
    
    if [[ "$has_summaries" == true ]]; then
        debug_log "Rendering summaries row"
        render_table_separator "middle"
        
        printf "${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}"
        for ((i=0; i<COLUMN_COUNT; i++)); do
            local summary_value="" datatype="${DATATYPES[$i]}" format="${FORMATS[$i]}"
            case "${SUMMARIES[$i]}" in
                sum)
                    if [[ -n "${SUM_SUMMARIES[$i]}" && "${SUM_SUMMARIES[$i]}" != "0" ]]; then
                        if [[ "$datatype" == "kcpu" ]]; then
                            # Format kcpu summary with thousands separator
                            local formatted_num=$(echo "${SUM_SUMMARIES[$i]}" | awk '{ printf "%\047d", $0 }')
                            summary_value="${formatted_num}m"
                        elif [[ "$datatype" == "kmem" ]]; then
                            # Format kmem summary with thousands separator
                            local formatted_num=$(echo "${SUM_SUMMARIES[$i]}" | awk '{ printf "%\047d", $0 }')
                            summary_value="${formatted_num}M"
                        elif [[ "$datatype" == "num" ]]; then
                            # For num datatype, use format_num to apply thousands separator
                            summary_value=$(format_num "${SUM_SUMMARIES[$i]}" "$format")
                        elif [[ "$datatype" == "int" || "$datatype" == "float" ]]; then
                            summary_value="${SUM_SUMMARIES[$i]}"
                            [[ -n "$format" ]] && summary_value=$(printf "$format" "$summary_value")
                        fi
                    fi
                    ;;
                min)
                    summary_value="${MIN_SUMMARIES[$i]:-}"
                    [[ -n "$format" ]] && summary_value=$(printf "$format" "$summary_value")
                    ;;
                max)
                    summary_value="${MAX_SUMMARIES[$i]:-}"
                    [[ -n "$format" ]] && summary_value=$(printf "$format" "$summary_value")
                    ;;
                count)
                    summary_value="${COUNT_SUMMARIES[$i]:-0}"
                    ;;
                unique)
                    if [[ -n "${UNIQUE_VALUES[$i]}" ]]; then
                        summary_value=$(echo "${UNIQUE_VALUES[$i]}" | tr ' ' '\n' | sort -u | wc -l | awk '{print $1}')
                    else
                        summary_value="0"
                    fi
                    ;;
            esac
            
            debug_log "Summary for column $i (${HEADERS[$i]}, ${SUMMARIES[$i]}): $summary_value"
            
            # Use summary_color for all summary values
            case "${JUSTIFICATIONS[$i]}" in
                right)
                    local content_width=$((WIDTHS[$i] - (2 * PADDINGS[$i])))
                    printf "%*s${THEME[summary_color]}%*s${THEME[text_color]}%*s${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}" \
                          "${PADDINGS[$i]}" "" "${content_width}" "$summary_value" "${PADDINGS[$i]}" ""
                    ;;
                center)
                    local content_width=$((WIDTHS[$i] - (2 * PADDINGS[$i])))
                    local value_spaces=$(( (content_width - ${#summary_value}) / 2 ))
                    local left_spaces=$(( PADDINGS[$i] + value_spaces ))
                    local right_spaces=$(( PADDINGS[$i] + content_width - ${#summary_value} - value_spaces ))
                    printf "%*s${THEME[summary_color]}%s${THEME[text_color]}%*s${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}" \
                          "${left_spaces}" "" "$summary_value" "${right_spaces}" ""
                    ;;
                *)
                    printf "%*s${THEME[summary_color]}%-*s${THEME[text_color]}%*s${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}" \
                          "${PADDINGS[$i]}" "" "$((WIDTHS[$i] - (2 * PADDINGS[$i])))" "$summary_value" "${PADDINGS[$i]}" ""
                    ;;
            esac
        done
        printf "\n"
        
        # Do not render bottom border here; it will be handled by render_table_bottom_border()
        # regardless of whether a footer is present
        :
        return 0
    fi
    
    debug_log "No summaries to render"
    return 1
}

# calculate_table_width: Calculate the total width of the table
# Returns: total width of the table in characters
calculate_table_width() {
    local width=0
    for ((i=0; i<COLUMN_COUNT; i++)); do
        ((width += WIDTHS[$i]))
        [[ $i -lt $((COLUMN_COUNT-1)) ]] && ((width++))
    done
    echo "$width"
}

# draw_table: Main function to render an ASCII table from JSON layout and data files
# Args: layout_json_file (path to layout JSON), data_json_file (path to data JSON),
#       [--debug] (enable debug logs), [--version] (show version)
# Outputs: ASCII table to stdout, errors and debug messages to stderr
draw_table() {
    local layout_file="$1" data_file="$2" debug=false
    shift 2
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --debug) debug=true; shift ;;
            --version) echo "tables.sh version $TABLES_VERSION"; return 0 ;;
            *) echo -e "${THEME[border_color]}Error: Unknown option: $1${THEME[text_color]}" >&2; return 1 ;;
        esac
    done

    # Set debug flag
    DEBUG_FLAG="$debug"
    debug_log "Starting table rendering with debug=$debug"
    debug_log "Layout file: $layout_file"
    debug_log "Data file: $data_file"

    # Validate input files
    validate_input_files "$layout_file" "$data_file" || return 1
    
    # Parse layout JSON and set the theme
    parse_layout_file "$layout_file" || return 1
    get_theme "$THEME_NAME"
    
    # Initialize summaries storage
    initialize_summaries
    
    # Read and prepare data
    local data_json
    data_json=$(prepare_data "$data_file")
    
    # Sort data if specified
    local sorted_data
    sorted_data=$(sort_data "$data_json")
    
    # Process data rows, update widths and calculate summaries
    process_data_rows "$sorted_data"
    
    debug_log "================ RENDERING TABLE ================"
    debug_log "Final column widths: ${WIDTHS[*]}"
    debug_log "Max lines per row: $MAX_LINES"
    debug_log "Total rows to render: ${#ROW_JSONS[@]}"
    
    # Calculate total table width
    local total_table_width=$(calculate_table_width)
    debug_log "Total table width: $total_table_width"
    
    # Render the table with title if specified
    if [[ -n "$TABLE_TITLE" ]]; then
        render_table_title "$total_table_width"
    fi
    render_table_top_border
    render_table_headers
    render_table_separator "middle"
    render_data_rows "$MAX_LINES"
    
    # Render summaries row if needed
    has_summaries=false
    if render_summaries_row; then
        has_summaries=true
    fi
    
    # Render bottom border, which will handle footer connection if present
    render_table_bottom_border
    
    # Render footer if specified
    if [[ -n "$TABLE_FOOTER" ]]; then
        render_table_footer "$total_table_width"
    fi
    
    debug_log "Table rendering complete"
}

# Main: Call draw_table with all arguments
if [[ "${BASH_SOURCE[0]}" == "$0" ]]; then
    draw_table "$@"
fi
