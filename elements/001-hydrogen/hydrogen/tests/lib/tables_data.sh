#!/usr/bin/env bash

# tables_data.sh: Data processing pipeline for tables.sh
# Handles data preparation, sorting, and row processing

# Global arrays for data storage and summaries
declare -a ROW_JSONS=()
declare -A SUM_SUMMARIES=()
declare -A COUNT_SUMMARIES=()
declare -A MIN_SUMMARIES=()
declare -A MAX_SUMMARIES=()
declare -A UNIQUE_VALUES=()
declare -A AVG_SUMMARIES=()
declare -A AVG_COUNTS=()
declare -a IS_WIDTH_SPECIFIED=()

# initialize_summaries: Initialize summaries storage
initialize_summaries() {
    debug_log "Initializing summaries storage"
    
    # Clear summary associative arrays
    SUM_SUMMARIES=()
    COUNT_SUMMARIES=()
    MIN_SUMMARIES=()
    MAX_SUMMARIES=()
    UNIQUE_VALUES=()
    AVG_SUMMARIES=()
    AVG_COUNTS=()
    
    for ((i=0; i<COLUMN_COUNT; i++)); do
        SUM_SUMMARIES[$i]=0
        COUNT_SUMMARIES[$i]=0
        MIN_SUMMARIES[$i]=""
        MAX_SUMMARIES[$i]=""
        UNIQUE_VALUES[$i]=""
        AVG_SUMMARIES[$i]=0
        AVG_COUNTS[$i]=0
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
    
    local temp_data_file
    temp_data_file=$(mktemp)
    echo "[" > "$temp_data_file"
    
    local row_count
    row_count=$(jq '. | length' <<<"$data_json")
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
        local temp_data_file
        temp_data_file=$(mktemp)
        echo "$data_json" > "$temp_data_file"
        
        local sorted_data
        sorted_data=$(jq -c "sort_by($jq_sort)" "$temp_data_file" 2>/tmp/jq_stderr.$$)
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
            local display_value
            display_value=$("$format_fn" "$value" "$format" "$string_limit" "$wrap_mode" "$wrap_char")
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
            
            # Do not update column width based on content if a width is specified in the layout or if the column is not visible
            if [[ "${IS_WIDTH_SPECIFIED[j]}" != "true" && "${VISIBLES[j]}" == "true" ]]; then
                # No specified width in layout, adjust based on content for visible columns only
                if [[ -n "$wrap_char" && "$wrap_mode" == "wrap" && -n "$display_value" && "$value" != "null" ]]; then
                    local max_len=0
                    local IFS="$wrap_char"
                    read -ra parts <<<"$display_value"
                    for part in "${parts[@]}"; do
                        local len
                        len=$(echo -n "$part" | sed 's/\x1B\[[0-9;]*m//g' | wc -c)
                        [[ $len -gt $max_len ]] && max_len=$len
                    done
                    local padded_width=$((max_len + (2 * PADDINGS[j])))
                    [[ $padded_width -gt ${WIDTHS[j]} ]] && WIDTHS[j]=$padded_width
                    [[ ${#parts[@]} -gt $line_count ]] && line_count=${#parts[@]}
                    debug_log "Wrapped value: parts=${#parts[@]}, max_len=$max_len, new width=${WIDTHS[j]}"
                else
                    local len
                    len=$(echo -n "$display_value" | sed 's/\x1B\[[0-9;]*m//g' | wc -c)
                    local padded_width=$((len + (2 * PADDINGS[j])))
                    [[ $padded_width -gt ${WIDTHS[j]} ]] && WIDTHS[j]=$padded_width
                    debug_log "Plain value: len=$len, new width=${WIDTHS[$j]}"
                fi
            else
                debug_log "Enforcing specified width or visibility for column $j (${HEADERS[$j]}): width=${WIDTHS[j]} (from layout or hidden)"
            fi
            
            # Update summaries
            update_summaries "$j" "$value" "${DATATYPES[$j]}" "${SUMMARIES[$j]}"
        done
        
        [[ $line_count -gt $MAX_LINES ]] && MAX_LINES=$line_count
    done
    
    # After processing all rows, check if summaries need wider columns, but respect specified width limits
    for ((j=0; j<COLUMN_COUNT; j++)); do
        if [[ "${SUMMARIES[$j]}" != "none" ]]; then
            local summary_value="" datatype="${DATATYPES[$j]}" format="${FORMATS[$j]}"
            
            # Calculate the expected summary value based on summary type
            case "${SUMMARIES[$j]}" in
                sum)
                    if [[ -n "${SUM_SUMMARIES[$j]}" && "${SUM_SUMMARIES[$j]}" != "0" ]]; then
                        if [[ "$datatype" == "kcpu" ]]; then
                            local formatted_num
                            formatted_num=$(echo "${SUM_SUMMARIES[$j]}" | awk '{ printf "%\047d", $0 }')
                            summary_value="${formatted_num}m"
                        elif [[ "$datatype" == "kmem" ]]; then
                            local formatted_num
                            formatted_num=$(echo "${SUM_SUMMARIES[$j]}" | awk '{ printf "%\047d", $0 }')
                            summary_value="${formatted_num}M"
                        elif [[ "$datatype" == "num" ]]; then
                            summary_value=$(format_num "${SUM_SUMMARIES[$j]}" "$format")
                        elif [[ "$datatype" == "int" || "$datatype" == "float" ]]; then
                            summary_value="${SUM_SUMMARIES[$j]}"
                            [[ -n "$format" ]] && summary_value=$(printf '%s' "$summary_value")
                        fi
                    fi
                    ;;
                min)
                    summary_value="${MIN_SUMMARIES[$j]:-}"
                    [[ -n "$format" ]] && summary_value=$(printf '%s' "$summary_value")
                    ;;
                max)
                    summary_value="${MAX_SUMMARIES[$j]:-}"
                    [[ -n "$format" ]] && summary_value=$(printf '%s' "$summary_value")
                    ;;
                count)
                    summary_value="${COUNT_SUMMARIES[$j]:-0}"
                    ;;
                unique)
                    if [[ -n "${UNIQUE_VALUES[$j]}" ]]; then
                        local unique_count
                        unique_count=$(echo "${UNIQUE_VALUES[$j]}" | tr ' ' '
' | sort -u | wc -l | awk '{print $1}')
                        summary_value="$unique_count"
                    else
                        summary_value="0"
                    fi
                    ;;
                avg)
                    if [[ -n "${AVG_SUMMARIES[$j]}" && "${AVG_COUNTS[$j]}" -gt 0 ]]; then
                        local avg_result
                        avg_result=$(awk "BEGIN {printf \"%.10f\", ${AVG_SUMMARIES[$j]} / ${AVG_COUNTS[$j]}}")
                        
                        # Format based on datatype
                        if [[ "$datatype" == "int" ]]; then
                            summary_value=$(printf "%.0f" "$avg_result")
                        elif [[ "$datatype" == "float" ]]; then
                            # Use same decimal precision as format if available, otherwise 2 decimals
                            if [[ -n "$format" && "$format" =~ %.([0-9]+)f ]]; then
                                local decimals="${BASH_REMATCH[1]}"
                                summary_value=$(printf "%.${decimals}f" "$avg_result")
                            else
                                summary_value=$(printf "%.2f" "$avg_result")
                            fi
                        elif [[ "$datatype" == "num" ]]; then
                            summary_value=$(format_num "$avg_result" "$format")
                        else
                            summary_value="$avg_result"
                        fi
                    else
                        summary_value="0"
                    fi
                    ;;
            esac
            
            # If summary exists, check if its width requires column adjustment, but only if no width is specified in layout and column is visible
            if [[ -n "$summary_value" && "${IS_WIDTH_SPECIFIED[j]}" != "true" && "${VISIBLES[j]}" == "true" ]]; then
                local summary_len
                summary_len=$(echo -n "$summary_value" | sed 's/\x1B\[[0-9;]*m//g' | wc -c)
                local summary_padded_width=$((summary_len + (2 * PADDINGS[j])))
                
                # Update column width if summary needs more space for visible columns only
                if [[ $summary_padded_width -gt ${WIDTHS[j]} ]]; then
                    debug_log "Column $j (${HEADERS[$j]}): Adjusting width for summary value '$summary_value', new width=$summary_padded_width"
                    WIDTHS[j]=$summary_padded_width
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
                if [[ -z "${MIN_SUMMARIES[$j]}" ]] || awk "BEGIN {if ($value < ${MIN_SUMMARIES[$j]}) exit 0; else exit 1}" 2>/dev/null; then
                    MIN_SUMMARIES[$j]="$value"
                fi
            fi
            ;;
        max)
            if [[ "$value" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
                if [[ -z "${MAX_SUMMARIES[$j]}" ]] || awk "BEGIN {if ($value > ${MAX_SUMMARIES[$j]}) exit 0; else exit 1}" 2>/dev/null; then
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
            if [[ -n "$value" && "$value" != "null" ]]; then
                if [[ -z "${UNIQUE_VALUES[$j]}" ]]; then
                    UNIQUE_VALUES[$j]="$value"
                else
                    UNIQUE_VALUES[$j]+=" $value"
                fi
            fi
            ;;
        avg)
            if [[ "$datatype" == "int" || "$datatype" == "float" || "$datatype" == "num" ]]; then
                if [[ "$value" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
                    AVG_SUMMARIES[$j]=$(awk "BEGIN {print (${AVG_SUMMARIES[$j]:-0} + $value)}")
                    AVG_COUNTS[$j]=$(( ${AVG_COUNTS[$j]:-0} + 1 ))
                fi
            fi
            ;;
    esac
}
