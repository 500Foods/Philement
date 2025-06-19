#!/usr/bin/env bash

# tables_render.sh: Rendering system for tables.sh
# Handles all table rendering including borders, headers, data rows, and summaries

# render_table_title: Render the title section if a title is specified
render_table_title() {
    local total_table_width="$1"
    
    if [[ -n "$TABLE_TITLE" ]]; then
        # shellcheck disable=SC2153
        debug_log "Rendering table title: $TABLE_TITLE with position: $TITLE_POSITION"
        # Evaluate any shell commands in the title before width calculation
        local title_text
        title_text=$(eval echo "$TABLE_TITLE" 2>/dev/null)
        calculate_title_width "$title_text" "$total_table_width"
        
        local offset=0
        case "$TITLE_POSITION" in
            left) offset=0 ;;
            right) 
                # shellcheck disable=SC2153
                offset=$((total_table_width - TITLE_WIDTH)) ;;
            center) offset=$(((total_table_width - TITLE_WIDTH) / 2)) ;;
            full) offset=0 ;;
            *) offset=0 ;;
        esac
        
        # Render title top border with offset
        if [[ $offset -gt 0 ]]; then
            printf "%*s" "$offset" ""
        fi
        printf "${THEME[border_color]}%s" "${THEME[tl_corner]}"
        printf "${THEME[h_line]}%.0s" $(seq 1 "$TITLE_WIDTH")
        printf "%s${THEME[text_color]}\n" "${THEME[tr_corner]}"
        
        # Render title text with appropriate alignment
        if [[ $offset -gt 0 ]]; then
            printf "%*s" "$offset" ""
        fi
        printf "${THEME[border_color]}%s${THEME[text_color]}" "${THEME[v_line]}"
        
        local available_width=$((TITLE_WIDTH - (2 * DEFAULT_PADDING)))
        
        # Clip text if it exceeds available width and a position (justification) is specified
        if [[ ${#title_text} -gt $available_width && "$TITLE_POSITION" != "" ]]; then
            case "$TITLE_POSITION" in
                left)
                    title_text="${title_text:0:$available_width}"
                    ;;
                right)
                    title_text="${title_text: -$available_width}"
                    ;;
                center|full)
                    local excess=$(( ${#title_text} - available_width ))
                    local left_clip=$(( excess / 2 ))
                    title_text="${title_text:$left_clip:$available_width}"
                    ;;
                *)
                    title_text="${title_text:0:$available_width}"
                    ;;
            esac
        fi
        
        case "$TITLE_POSITION" in
            left)
                printf "%*s${THEME[header_color]}%-*s${THEME[text_color]}%*s" \
                      "$DEFAULT_PADDING" "" "$available_width" "$title_text" "$DEFAULT_PADDING" ""
                ;;
            right)
                printf "%*s${THEME[header_color]}%*s${THEME[text_color]}%*s" \
                      "$DEFAULT_PADDING" "" "$available_width" "$title_text" "$DEFAULT_PADDING" ""
                ;;
            center)
                printf "%*s${THEME[header_color]}%s${THEME[text_color]}%*s" \
                      "$DEFAULT_PADDING" "" "$title_text" "$((available_width - ${#title_text} + DEFAULT_PADDING))" ""
                ;;
            full)
                local text_len=${#title_text}
                local spaces=$(( (available_width - text_len) / 2 ))
                local left_spaces=$(( DEFAULT_PADDING + spaces ))
                local right_spaces=$(( DEFAULT_PADDING + available_width - text_len - spaces ))
                printf "%*s${THEME[header_color]}%s${THEME[text_color]}%*s" \
                      "$left_spaces" "" "$title_text" "$right_spaces" ""
                ;;
            *)
                printf "%*s${THEME[header_color]}%s${THEME[text_color]}%*s" \
                      "$DEFAULT_PADDING" "" "$title_text" "$DEFAULT_PADDING" ""
                 ;;
         esac
         
         printf "${THEME[border_color]}%s${THEME[text_color]}\n" "${THEME[v_line]}"
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
    # local element_position="$6"  # Unused variable
    # local is_element_full="$7"   # Unused variable
    
    debug_log "Rendering $border_type border with total width: $total_table_width, element offset: $element_offset, element right edge: $element_right_edge, element width: $element_width"
    
    # Calculate positions of all column separators for visible columns
    local column_widths_sum=0
    local column_positions=()
    for ((i=0; i<COLUMN_COUNT-1; i++)); do
        if [[ "${VISIBLES[i]}" == "true" ]]; then
            column_widths_sum=$((column_widths_sum + WIDTHS[i]))
            # Check if there are any more visible columns after this one
            local has_more_visible=false
            for ((j=$((i+1)); j<COLUMN_COUNT; j++)); do
                if [[ "${VISIBLES[j]}" == "true" ]]; then
                    has_more_visible=true
                    break
                fi
            done
            if [[ "$has_more_visible" == "true" ]]; then
                column_positions+=("$column_widths_sum")
                ((column_widths_sum++))  # +1 for the separator
            fi
        fi
    done
    # Ensure the column positions are correctly aligned for border rendering
    debug_log "Column positions for separators: ${column_positions[*]}"
    
    # Determine maximum width to render (should match the table width considering only visible columns)
    # Add 2 to account for left and right border characters
    local max_width=$((total_table_width + 2))
    if [[ -n "$element_width" && $element_width -gt 0 ]]; then
        # If element width is provided, compare with table width to determine max width
        local adjusted_element_width=$((element_width + 2))
        if [[ $adjusted_element_width -gt $max_width ]]; then
            max_width=$adjusted_element_width
        fi
    fi
    # Ensure max_width does not exceed the width of visible content plus borders
    local visible_content_width=$((total_table_width + 2))
    if [[ $max_width -gt $visible_content_width ]]; then
        max_width=$visible_content_width
    fi
    
    debug_log "Max width for $border_type border: $max_width"
    
    # Print the border line character by character
    # shellcheck disable=SC2059
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
        
        # Use direct output to ensure ANSI color codes are interpreted by the terminal
        echo -ne "$char_to_print"
    done
    # shellcheck disable=SC2059
    printf "${THEME[text_color]}\n"
}

# render_table_top_border: Render the top border of the table
render_table_top_border() {
    debug_log "Rendering top border"
    
    local total_table_width=0
    local visible_count=0
    for ((i=0; i<COLUMN_COUNT; i++)); do
        if [[ "${VISIBLES[i]}" == "true" ]]; then
            ((total_table_width += WIDTHS[i]))
            ((visible_count++))
        fi
    done
    if [[ $visible_count -gt 1 ]]; then
        ((total_table_width += visible_count - 1))
    fi
    
    local title_offset=0
    local title_right_edge=0
    local title_width=""
    local title_position="none"
    if [[ -n "$TABLE_TITLE" ]]; then
        title_width=$TITLE_WIDTH
        title_position=$TITLE_POSITION
        case "$TITLE_POSITION" in
            left) title_offset=0; title_right_edge=$TITLE_WIDTH ;;
            right) title_offset=$((total_table_width - TITLE_WIDTH)); title_right_edge=$total_table_width ;;
            center) title_offset=$(((total_table_width - TITLE_WIDTH) / 2)); title_right_edge=$((title_offset + TITLE_WIDTH)) ;;
            full) title_offset=0; title_right_edge=$total_table_width ;;
            *) title_offset=0; title_right_edge=$TITLE_WIDTH ;;
        esac
    fi
    
    render_table_border "top" "$total_table_width" "$title_offset" "$title_right_edge" "$title_width" "$title_position" "$([[ "$title_position" == "full" ]] && echo true || echo false)"
}

# render_table_bottom_border: Render the bottom border of the table
render_table_bottom_border() {
    debug_log "Rendering bottom border"
    
    local total_table_width=0
    local visible_count=0
    for ((i=0; i<COLUMN_COUNT; i++)); do
        if [[ "${VISIBLES[i]}" == "true" ]]; then
            ((total_table_width += WIDTHS[i]))
            ((visible_count++))
        fi
    done
    if [[ $visible_count -gt 1 ]]; then
        ((total_table_width += visible_count - 1))
    fi
    
    local footer_offset=0
    local footer_right_edge=0
    local footer_width=""
    local footer_position="none"
    if [[ -n "$TABLE_FOOTER" ]]; then
        calculate_footer_width "$TABLE_FOOTER" "$total_table_width"
        # shellcheck disable=SC2153
        footer_width=$FOOTER_WIDTH
        # shellcheck disable=SC2153
        footer_position=$FOOTER_POSITION
        case "$FOOTER_POSITION" in
            left) footer_offset=0; footer_right_edge=$FOOTER_WIDTH ;;
            right) footer_offset=$((total_table_width - FOOTER_WIDTH)); footer_right_edge=$total_table_width ;;
            center) footer_offset=$(((total_table_width - FOOTER_WIDTH) / 2)); footer_right_edge=$((footer_offset + FOOTER_WIDTH)) ;;
            full) footer_offset=0; footer_right_edge=$total_table_width ;;
            *) footer_offset=0; footer_right_edge=$FOOTER_WIDTH ;;
        esac
    fi
    
    render_table_border "bottom" "$total_table_width" "$footer_offset" "$footer_right_edge" "$footer_width" "$footer_position" "$([[ "$footer_position" == "full" ]] && echo true || echo false)"
}

# render_table_headers: Render the table headers row
render_table_headers() {
    debug_log "Rendering table headers"
    
    printf "${THEME[border_color]}%s${THEME[text_color]}" "${THEME[v_line]}"
    for ((i=0; i<COLUMN_COUNT; i++)); do
        if [[ "${VISIBLES[i]}" == "true" ]]; then
            debug_log "Rendering header $i: ${HEADERS[$i]}, width=${WIDTHS[$i]}, justification=${JUSTIFICATIONS[$i]}"
            local header_text="${HEADERS[$i]}"
            local content_width=$((WIDTHS[i] - (2 * PADDINGS[i])))
            if [[ ${#header_text} -gt $content_width ]]; then
                case "${JUSTIFICATIONS[$i]}" in
                    right)
                        header_text="${header_text: -${content_width}}"
                        ;;
                    center)
                        local excess=$(( ${#header_text} - content_width ))
                        local left_clip=$(( excess / 2 ))
                        header_text="${header_text:${left_clip}:${content_width}}"
                        ;;
                    *)
                        header_text="${header_text:0:${content_width}}"
                        ;;
                esac
            fi
            
            case "${JUSTIFICATIONS[$i]}" in
                right)
                    printf "%*s${THEME[caption_color]}%*s${THEME[text_color]}%*s${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}" \
                          "${PADDINGS[i]}" "" "${content_width}" "${header_text}" "${PADDINGS[i]}" ""
                    ;;
                center)
                    local header_spaces=$(( (content_width - ${#header_text}) / 2 ))
                    local left_spaces=$(( PADDINGS[i] + header_spaces ))
                    local right_spaces=$(( PADDINGS[i] + content_width - ${#header_text} - header_spaces ))
                    printf "%*s${THEME[caption_color]}%s${THEME[text_color]}%*s${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}" \
                          "${left_spaces}" "" "${header_text}" "${right_spaces}" ""
                    ;;
                *)
                    printf "%*s${THEME[caption_color]}%-*s${THEME[text_color]}%*s${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}" \
                          "${PADDINGS[i]}" "" "${content_width}" "${header_text}" "${PADDINGS[i]}" ""
                    ;;
            esac
        fi
    done
    printf "\n"
}

# render_table_separator: Render a separator line in the table
render_table_separator() {
    local type="$1"
    debug_log "Rendering table separator: $type"
    
    local left_char="${THEME[l_junct]}" right_char="${THEME[r_junct]}" middle_char="${THEME[cross]}"
    [[ "$type" == "bottom" ]] && left_char="${THEME[bl_corner]}" && right_char="${THEME[br_corner]}" && middle_char="${THEME[b_junct]}"
    
    printf "${THEME[border_color]}%s" "${left_char}"
    for ((i=0; i<COLUMN_COUNT; i++)); do
        if [[ "${VISIBLES[i]}" == "true" ]]; then
            local width=${WIDTHS[i]}
            debug_log "Rendering separator for visible column $i with width $width"
            for ((j=0; j<width; j++)); do
                printf "%s" "${THEME[h_line]}"
            done
            if [[ $i -lt $((COLUMN_COUNT-1)) ]]; then
                local next_visible=false
                for ((k=$((i+1)); k<COLUMN_COUNT; k++)); do
                    if [[ "${VISIBLES[k]}" == "true" ]]; then
                        next_visible=true
                        break
                    fi
                done
                if [[ "$next_visible" == "true" ]]; then
                    printf "%s" "${middle_char}"
                    debug_log "Placed separator after column $i"
                fi
            fi
        else
            debug_log "Skipping hidden column $i"
        fi
    done
    printf "%s${THEME[text_color]}\n" "${right_char}"
}

# render_data_rows: Render the data rows of the table
render_data_rows() {
    local max_lines="$1"
    debug_log "================ RENDERING DATA ROWS ================"
    debug_log "Rendering ${#ROW_JSONS[@]} rows with max_lines=$max_lines"
    
    # Initialize last break values
    local last_break_values=()
    for ((j=0; j<COLUMN_COUNT; j++)); do
        last_break_values[j]=""
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
            
            local display_value
            display_value=$(format_display_value "$value" "${NULL_VALUES[j]}" "${ZERO_VALUES[j]}" "${DATATYPES[j]}" "${FORMATS[j]}" "${STRING_LIMITS[j]}" "${WRAP_MODES[j]}" "${WRAP_CHARS[j]}")
            
            if [[ -n "${WRAP_CHARS[$j]}" && "${WRAP_MODES[$j]}" == "wrap" && -n "$display_value" && "$value" != "null" ]]; then
                local IFS="${WRAP_CHARS[$j]}"
                read -ra parts <<<"$display_value"
                for k in "${!parts[@]}"; do
                    local part="${parts[k]}"
                    local content_width=$((WIDTHS[j] - (2 * PADDINGS[j])))
                    if [[ ${#part} -gt $content_width ]]; then
                        case "${JUSTIFICATIONS[$j]}" in
                            right)
                                part="${part: -${content_width}}"
                                ;;
                            center)
                                local excess=$(( ${#part} - content_width ))
                                local left_clip=$(( excess / 2 ))
                                part="${part:${left_clip}:${content_width}}"
                                ;;
                            *)
                                part="${part:0:${content_width}}"
                                ;;
                        esac
                    fi
                    line_values[$j,$k]="$part"
                done
                [[ ${#parts[@]} -gt $row_line_count ]] && row_line_count=${#parts[@]}
            elif [[ "${WRAP_MODES[$j]}" == "wrap" && -n "$display_value" && "$value" != "null" ]]; then
                local content_width=$((WIDTHS[j] - (2 * PADDINGS[j])))
                local words=()
                IFS=' ' read -ra words <<<"$display_value"
                local current_line=""
                local line_index=0
                for word in "${words[@]}"; do
                    if [[ -z "$current_line" ]]; then
                        current_line="$word"
                    elif [[ $(( ${#current_line} + ${#word} + 1 )) -le $content_width ]]; then
                        current_line="$current_line $word"
                    else
                        line_values[$j,$line_index]="$current_line"
                        current_line="$word"
                        ((line_index++))
                    fi
                done
                if [[ -n "$current_line" ]]; then
                    line_values[$j,$line_index]="$current_line"
                    ((line_index++))
                fi
                [[ $line_index -gt $row_line_count ]] && row_line_count=$line_index
            else
                local content_width=$((WIDTHS[j] - (2 * PADDINGS[j])))
                if [[ ${#display_value} -gt $content_width ]]; then
                    case "${JUSTIFICATIONS[$j]}" in
                        right)
                            display_value="${display_value: -${content_width}}"
                            ;;
                        center)
                            local excess=$(( ${#display_value} - content_width ))
                            local left_clip=$(( excess / 2 ))
                            display_value="${display_value:${left_clip}:${content_width}}"
                            ;;
                        *)
                            display_value="${display_value:0:${content_width}}"
                            ;;
                    esac
                fi
                line_values[$j,0]="$display_value"
            fi
        done
        
        # Render each line of the row
        for ((line=0; line<row_line_count; line++)); do
            printf "${THEME[border_color]}%s${THEME[text_color]}" "${THEME[v_line]}"
            for ((j=0; j<COLUMN_COUNT; j++)); do
                if [[ "${VISIBLES[j]}" == "true" ]]; then
                    local display_value="${line_values[$j,$line]:-}"
                    local content_width=$((WIDTHS[j] - (2 * PADDINGS[j])))
                    
                    # Clip the display value if it exceeds the content width and a width is specified
                    if [[ ${#display_value} -gt $content_width && "${IS_WIDTH_SPECIFIED[j]}" == "true" ]]; then
                        case "${JUSTIFICATIONS[$j]}" in
                            right)
                                display_value="${display_value: -$content_width}"
                                ;;
                            center)
                                local excess=$(( ${#display_value} - content_width ))
                                local left_clip=$(( excess / 2 ))
                                display_value="${display_value:$left_clip:$content_width}"
                                ;;
                            *)
                                display_value="${display_value:0:$content_width}"
                                ;;
                        esac
                    fi
                    
                    case "${JUSTIFICATIONS[$j]}" in
                        right)
                            printf "%*s%*s%*s${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}" \
                                  "${PADDINGS[j]}" "" "${content_width}" "$display_value" "${PADDINGS[j]}" ""
                            ;;
                        center)
                            if [[ -z "$display_value" ]]; then
                                printf "%*s%*s${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}" \
                                      "${PADDINGS[j]}" "" "$((WIDTHS[j] - (2 * PADDINGS[j]) + PADDINGS[j]))" ""
                            else
                                local value_spaces=$(( (content_width - ${#display_value}) / 2 ))
                                local left_spaces=$(( PADDINGS[j] + value_spaces ))
                                local right_spaces=$(( PADDINGS[j] + content_width - ${#display_value} - value_spaces ))
                                printf "%*s%s%*s${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}" \
                                      "${left_spaces}" "" "$display_value" "${right_spaces}" ""
                            fi
                            ;;
                        *)
                            printf "%*s%-*s%*s${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}" \
                                  "${PADDINGS[j]}" "" "${content_width}" "$display_value" "${PADDINGS[j]}" ""
                            ;;
                    esac
                fi
            done
            printf "\n"
        done
        
        # Update break values for the next iteration
        for ((j=0; j<COLUMN_COUNT; j++)); do
            if [[ "${BREAKS[$j]}" == "true" ]]; then
                local key="${KEYS[$j]}" value
                value=$(jq -r ".${key} // \"\"" <<<"$row_json")
                last_break_values[j]="$value"
            fi
        done
    done
}

# render_table_footer: Render the footer section if a footer is specified
render_table_footer() {
    local total_table_width="$1"
    
    if [[ -n "$TABLE_FOOTER" ]]; then
        debug_log "Rendering table footer: $TABLE_FOOTER with position: $FOOTER_POSITION"
        # Evaluate any shell commands in the footer before width calculation
        local footer_text
        footer_text=$(eval echo "$TABLE_FOOTER" 2>/dev/null)
        calculate_footer_width "$footer_text" "$total_table_width"
        
        local footer_offset=0
        case "$FOOTER_POSITION" in
            left) footer_offset=0 ;;
            right) footer_offset=$((total_table_width - FOOTER_WIDTH)) ;;
            center) footer_offset=$(((total_table_width - FOOTER_WIDTH) / 2)) ;;
            full) footer_offset=0 ;;
            *) footer_offset=0 ;;
        esac
        
        # Render footer text with appropriate alignment
        if [[ $footer_offset -gt 0 ]]; then
            printf "%*s" "$footer_offset" ""
        fi
        
        local available_width=$((FOOTER_WIDTH - (2 * DEFAULT_PADDING)))
        
        if [[ ${#footer_text} -gt $available_width ]]; then
            case "$FOOTER_POSITION" in
                left)
                    footer_text="${footer_text:0:$available_width}"
                    ;;
                right)
                    footer_text="${footer_text: -$available_width}"
                    ;;
                center)
                    local excess=$(( ${#footer_text} - available_width ))
                    local left_clip=$(( excess / 2 ))
                    footer_text="${footer_text:$left_clip:$available_width}"
                    ;;
                full)
                    footer_text="${footer_text:0:$available_width}"
                    ;;
                *)
                    footer_text="${footer_text:0:$available_width}"
                    ;;
            esac
        fi
        
        case "$FOOTER_POSITION" in
            left)
                printf "${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}%*s${THEME[footer_color]}%-*s${THEME[text_color]}%*s${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}" \
                      "$DEFAULT_PADDING" "" "$available_width" "$footer_text" "$DEFAULT_PADDING" ""
                ;;
            right)
                printf "${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}%*s${THEME[footer_color]}%*s${THEME[text_color]}%*s${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}" \
                      "$DEFAULT_PADDING" "" "$available_width" "$footer_text" "$DEFAULT_PADDING" ""
                ;;
            center)
                printf "${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}%*s${THEME[footer_color]}%s${THEME[text_color]}%*s${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}" \
                      "$DEFAULT_PADDING" "" "$footer_text" "$((available_width - ${#footer_text} + DEFAULT_PADDING))" ""
                ;;
            full)
                local text_len=${#footer_text}
                local spaces=$(( (available_width - text_len) / 2 ))
                local left_spaces=$(( DEFAULT_PADDING + spaces ))
                local right_spaces=$(( DEFAULT_PADDING + available_width - text_len - spaces ))
                printf "${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}%*s${THEME[footer_color]}%s${THEME[text_color]}%*s${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}" \
                      "$left_spaces" "" "$footer_text" "$right_spaces" ""
                ;;
            *)
                printf "${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}%*s${THEME[footer_color]}%s${THEME[text_color]}%*s${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}" \
                      "$DEFAULT_PADDING" "" "$footer_text" "$DEFAULT_PADDING" ""
                ;;
        esac
        
        printf "\n"
        
        # Render footer bottom border
        if [[ $footer_offset -gt 0 ]]; then
            printf "%*s" "$footer_offset" ""
        fi
        echo -ne "${THEME[border_color]}${THEME[bl_corner]}"
        for i in $(seq 1 "$FOOTER_WIDTH"); do
            echo -ne "${THEME[h_line]}"
        done
        echo -ne "${THEME[br_corner]}${THEME[text_color]}\n"
    fi
}

# render_summaries_row: Render the summaries row if any summaries are defined
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
        
        printf "${THEME[border_color]}%s${THEME[text_color]}" "${THEME[v_line]}"
        for ((i=0; i<COLUMN_COUNT; i++)); do
            if [[ "${VISIBLES[i]}" == "true" ]]; then
                local summary_value="" datatype="${DATATYPES[$i]}" format="${FORMATS[$i]}"
                case "${SUMMARIES[$i]}" in
                    sum)
                        if [[ -n "${SUM_SUMMARIES[$i]}" && "${SUM_SUMMARIES[$i]}" != "0" ]]; then
                            if [[ "$datatype" == "kcpu" ]]; then
                                local formatted_num
                                formatted_num=$(echo "${SUM_SUMMARIES[$i]}" | awk '{ printf "%\047d", $0 }')
                                summary_value="${formatted_num}m"
                            elif [[ "$datatype" == "kmem" ]]; then
                                local formatted_num
                                formatted_num=$(echo "${SUM_SUMMARIES[$i]}" | awk '{ printf "%\047d", $0 }')
                                summary_value="${formatted_num}M"
                            elif [[ "$datatype" == "num" ]]; then
                                summary_value=$(format_num "${SUM_SUMMARIES[$i]}" "$format")
                            elif [[ "$datatype" == "int" || "$datatype" == "float" ]]; then
                                summary_value="${SUM_SUMMARIES[$i]}"
                                [[ -n "$format" ]] && summary_value=$(printf "%s" "$format" | xargs printf "%s" "$summary_value")
                            fi
                        fi
                        ;;
                    min)
                        summary_value="${MIN_SUMMARIES[$i]:-}"
                        [[ -n "$format" ]] && summary_value=$(printf "%s" "$format" | xargs printf "%s" "$summary_value")
                        ;;
                    max)
                        summary_value="${MAX_SUMMARIES[$i]:-}"
                        [[ -n "$format" ]] && summary_value=$(printf "%s" "$format" | xargs printf "%s" "$summary_value")
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
                    avg)
                        if [[ -n "${AVG_SUMMARIES[$i]}" && "${AVG_COUNTS[$i]}" -gt 0 ]]; then
                            local avg_result
                            avg_result=$(awk "BEGIN {printf \"%.10f\", ${AVG_SUMMARIES[$i]} / ${AVG_COUNTS[$i]}}")
                            
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
                
                # Use summary_color for all summary values and clip if necessary based on whether width is specified
                local content_width=$((WIDTHS[i] - (2 * PADDINGS[i])))
                if [[ ${#summary_value} -gt $content_width && "${IS_WIDTH_SPECIFIED[i]}" == "true" ]]; then
                    case "${JUSTIFICATIONS[$i]}" in
                        right)
                            summary_value="${summary_value: -$content_width}"
                            ;;
                        center)
                            local excess=$(( ${#summary_value} - content_width ))
                            local left_clip=$(( excess / 2 ))
                            summary_value="${summary_value:$left_clip:$content_width}"
                            ;;
                        *)
                            summary_value="${summary_value:0:$content_width}"
                            ;;
                    esac
                fi
                
                case "${JUSTIFICATIONS[$i]}" in
                    right)
                        printf "%*s${THEME[summary_color]}%*s${THEME[text_color]}%*s${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}" \
                              "${PADDINGS[i]}" "" "${content_width}" "$summary_value" "${PADDINGS[i]}" ""
                        ;;
                    center)
                        local value_spaces=$(( (content_width - ${#summary_value}) / 2 ))
                        local left_spaces=$(( PADDINGS[i] + value_spaces ))
                        local right_spaces=$(( PADDINGS[i] + content_width - ${#summary_value} - value_spaces ))
                        printf "%*s${THEME[summary_color]}%s${THEME[text_color]}%*s${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}" \
                              "${left_spaces}" "" "$summary_value" "${right_spaces}" ""
                        ;;
                    *)
                        printf "%*s${THEME[summary_color]}%-*s${THEME[text_color]}%*s${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}" \
                              "${PADDINGS[i]}" "" "${content_width}" "$summary_value" "${PADDINGS[i]}" ""
                        ;;
                esac
            fi
        done
        printf "\n"
        return 0
    fi
    
    debug_log "No summaries to render"
    return 1
}
