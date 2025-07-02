#!/usr/bin/env bash

# tables_config.sh: Configuration parsing and validation for tables.sh
# Handles layout JSON parsing and column/sort configuration

# Global variables for title and footer support
declare -gx TABLE_TITLE=""
declare -gx TITLE_WIDTH=0
declare -gx TITLE_POSITION="none"
declare -gx TABLE_FOOTER=""
declare -gx FOOTER_WIDTH=0
declare -gx FOOTER_POSITION="none"

# Global arrays to store table configuration
declare -ax HEADERS=()
declare -ax KEYS=()
declare -ax JUSTIFICATIONS=()
declare -ax DATATYPES=()
declare -ax NULL_VALUES=()
declare -ax ZERO_VALUES=()
declare -ax FORMATS=()
declare -ax SUMMARIES=()
declare -ax BREAKS=()
declare -ax STRING_LIMITS=()
declare -ax WRAP_MODES=()
declare -ax WRAP_CHARS=()
declare -ax PADDINGS=()
declare -ax WIDTHS=()
declare -ax SORT_KEYS=()
declare -ax SORT_DIRECTIONS=()
declare -ax SORT_PRIORITIES=()
declare -ax IS_WIDTH_SPECIFIED=()
declare -ax VISIBLES=()

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
    IS_WIDTH_SPECIFIED=()
    VISIBLES=()
    
    local column_count
    column_count=$(jq '. | length' <<<"$columns_json")
    debug_log "Column count: $column_count"
    
    # Store column count globally
    COLUMN_COUNT=$column_count
    
    for ((i=0; i<column_count; i++)); do
        local col_json
        col_json=$(jq -c ".[$i]" <<<"$columns_json")
        debug_log "Column $i: Full JSON content: $col_json"
        HEADERS[i]=$(jq -r '.header // ""' <<<"$col_json")
        KEYS[i]=$(jq -r '.key // (.header | ascii_downcase | gsub("[^a-z0-9]"; "_"))' <<<"$col_json")
        JUSTIFICATIONS[i]=$(jq -r '.justification // "left"' <<<"$col_json" | tr '[:upper:]' '[:lower:]')
        DATATYPES[i]=$(jq -r '.datatype // "text"' <<<"$col_json" | tr '[:upper:]' '[:lower:]')
        NULL_VALUES[i]=$(jq -r '.null_value // "blank"' <<<"$col_json" | tr '[:upper:]' '[:lower:]')
        ZERO_VALUES[i]=$(jq -r '.zero_value // "blank"' <<<"$col_json" | tr '[:upper:]' '[:lower:]')
        FORMATS[i]=$(jq -r '.format // ""' <<<"$col_json")
        SUMMARIES[i]=$(jq -r '.summary // "none"' <<<"$col_json" | tr '[:upper:]' '[:lower:]')
        BREAKS[i]=$(jq -r '.break // false' <<<"$col_json")
        STRING_LIMITS[i]=$(jq -r '.string_limit // 0' <<<"$col_json")
        WRAP_MODES[i]=$(jq -r '.wrap_mode // "clip"' <<<"$col_json" | tr '[:upper:]' '[:lower:]')
        WRAP_CHARS[i]=$(jq -r '.wrap_char // ""' <<<"$col_json")
        PADDINGS[i]=$(jq -r '.padding // '"$DEFAULT_PADDING" <<<"$col_json")
	local visible_raw
        visible_raw=$(jq -r '.visible // true' <<<"$col_json")
        debug_log "Column $i: Raw visible value from JSON: $visible_raw"
        # Check if visible key exists in JSON to debug potential mismatch
	local visible_key_check
        visible_key_check=$(jq -r 'has("visible")' <<<"$col_json")
        debug_log "Column $i: Does 'visible' key exist in JSON? $visible_key_check"
        if [[ "$visible_key_check" == "true" ]]; then
	    local visible_value
            visible_value=$(jq -r '.visible' <<<"$col_json")
            debug_log "Column $i: Explicit visible value (if key exists): $visible_value"
            VISIBLES[i]="$visible_value"
        else
            VISIBLES[i]="$visible_raw"
        fi
        
        debug_log "Column $i: Header=${HEADERS[$i]}, Key=${KEYS[$i]}, Datatype=${DATATYPES[$i]}, Visible=${VISIBLES[$i]}"
        
        validate_column_config "$i" "${HEADERS[$i]}" "${JUSTIFICATIONS[$i]}" "${DATATYPES[$i]}" "${SUMMARIES[$i]}"
    done
    
    # Initialize column widths based on header lengths plus padding or specified width
    for ((i=0; i<COLUMN_COUNT; i++)); do
        local col_json
        col_json=$(jq -c ".[$i]" <<<"$columns_json")
        local specified_width
        specified_width=$(jq -r '.width // 0' <<<"$col_json")
        
        if [[ $specified_width -gt 0 ]]; then
            WIDTHS[i]=$specified_width
            IS_WIDTH_SPECIFIED[i]="true"
            debug_log "Width specified for column $i (${HEADERS[$i]}): ${WIDTHS[$i]}"
        else
            WIDTHS[i]=$((${#HEADERS[i]} + (2 * PADDINGS[i])))
            IS_WIDTH_SPECIFIED[i]="false"
            debug_log "Width not specified for column $i (${HEADERS[$i]}), using header length: ${WIDTHS[$i]}"
        fi
        debug_log "Initial width for column $i (${HEADERS[$i]}): ${WIDTHS[$i]} (including padding ${PADDINGS[$i]}), Width specified: ${IS_WIDTH_SPECIFIED[$i]}"
    done
    
    # Debug verification of array contents
    debug_log "After parse_column_config - Number of columns: $COLUMN_COUNT"
    debug_log "After parse_column_config - Headers: ${HEADERS[*]}"
    debug_log "After parse_column_config - Keys: ${KEYS[*]}"
    debug_log "After parse_column_config - Visibles: ${VISIBLES[*]}"
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
        JUSTIFICATIONS[i]="left"
    fi
    
    if [[ -z "${DATATYPE_HANDLERS[${datatype}_validate]}" ]]; then
        echo -e "${THEME[border_color]}Warning: Invalid datatype '$datatype' for column $header, using 'text'${THEME[text_color]}" >&2
        DATATYPES[i]="text"
    fi
    
    local valid_summaries="${DATATYPE_HANDLERS[${DATATYPES[$i]}_summary_types]}"
    if [[ "$summary" != "none" && ! " $valid_summaries " =~ $summary ]]; then
        echo -e "${THEME[border_color]}Warning: Summary '$summary' not supported for datatype '${DATATYPES[$i]}' in column $header, using 'none'${THEME[text_color]}" >&2
        SUMMARIES[i]="none"
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
    
    local sort_count
    sort_count=$(jq '. | length' <<<"$sort_json")
    debug_log "Sort count: $sort_count"
    
    for ((i=0; i<sort_count; i++)); do
        local sort_item
        sort_item=$(jq -c ".[$i]" <<<"$sort_json")
        SORT_KEYS[i]=$(jq -r '.key // ""' <<<"$sort_item")
        SORT_DIRECTIONS[i]=$(jq -r '.direction // "asc"' <<<"$sort_item" | tr '[:upper:]' '[:lower:]')
        SORT_PRIORITIES[i]=$(jq -r '.priority // 0' <<<"$sort_item")
        
        debug_log "Sort $i: Key=${SORT_KEYS[$i]}, Direction=${SORT_DIRECTIONS[$i]}"
        
        if [[ -z "${SORT_KEYS[$i]}" ]]; then
            echo -e "${THEME[border_color]}Warning: Sort item $i has no key, ignoring${THEME[text_color]}" >&2
            continue
        fi
        
        if [[ "${SORT_DIRECTIONS[$i]}" != "asc" && "${SORT_DIRECTIONS[$i]}" != "desc" ]]; then
            echo -e "${THEME[border_color]}Warning: Invalid sort direction '${SORT_DIRECTIONS[$i]}' for key ${SORT_KEYS[$i]}, using 'asc'${THEME[text_color]}" >&2
            SORT_DIRECTIONS[i]="asc"
        fi
    done
}
