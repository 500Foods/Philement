#!/usr/bin/env bash
# tables.sh - Library for JSON to ANSI tables
declare -g COLUMN_COUNT=0 MAX_LINES=1 THEME_NAME="Red" DEFAULT_PADDING=1
declare -A DATATYPE_HANDLERS=([text_validate]="validate_text" [text_format]="format_text" [text_summary_types]="count unique" [int_validate]="validate_number" [int_format]="format_number" [int_summary_types]="sum min max avg count unique" [num_validate]="validate_number" [num_format]="format_num" [num_summary_types]="sum min max avg count unique" [float_validate]="validate_number" [float_format]="format_number" [float_summary_types]="sum min max avg count unique" [kcpu_validate]="validate_kcpu" [kcpu_format]="format_kcpu" [kcpu_summary_types]="sum min max avg count unique" [kmem_validate]="validate_kmem" [kmem_format]="format_kmem" [kmem_summary_types]="sum min max avg count unique")
declare -A RED_THEME=([border_color]='\033[0;31m' [caption_color]='\033[0;32m' [header_color]='\033[1;37m' [footer_color]='\033[0;36m' [summary_color]='\033[1;37m' [text_color]='\033[0m' [tl_corner]='╭' [tr_corner]='╮' [bl_corner]='╰' [br_corner]='╯' [h_line]='─' [v_line]='│' [t_junct]='┬' [b_junct]='┴' [l_junct]='├' [r_junct]='┤' [cross]='┼')
declare -A BLUE_THEME=([border_color]='\033[0;34m' [caption_color]='\033[0;34m' [header_color]='\033[1;37m' [footer_color]='\033[0;36m' [summary_color]='\033[1;37m' [text_color]='\033[0m' [tl_corner]='╭' [tr_corner]='╮' [bl_corner]='╰' [br_corner]='╯' [h_line]='─' [v_line]='│' [t_junct]='┬' [b_junct]='┴' [l_junct]='├' [r_junct]='┤' [cross]='┼')
declare -A THEME
for key in "${!RED_THEME[@]}"; do THEME[$key]="${RED_THEME[$key]}"; done
# shellcheck disable=SC2317  # Called indirectly via DATATYPE_HANDLERS
validate_text() { local value="$1"; [[ "$value" != "null" ]] && echo "$value" || echo ""; }
# shellcheck disable=SC2317  # Called indirectly via DATATYPE_HANDLERS
validate_number() { local value="$1"; if [[ "$value" =~ ^[0-9]+(\.[0-9]+)?$ || "$value" == "0" || "$value" == "null" ]]; then echo "$value"; else echo ""; fi; }
# shellcheck disable=SC2317  # Called indirectly via DATATYPE_HANDLERS
validate_kcpu() { local value="$1"; if [[ "$value" =~ ^[0-9]+m$ || "$value" == "0" || "$value" == "0m" || "$value" == "null" || "$value" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then echo "$value"; else echo "$value"; fi; }
# shellcheck disable=SC2317  # Called indirectly via DATATYPE_HANDLERS
validate_kmem() { local value="$1"; if [[ "$value" =~ ^[0-9]+[KMG]$ || "$value" =~ ^[0-9]+Mi$ || "$value" =~ ^[0-9]+Gi$ || "$value" =~ ^[0-9]+Ki$ || "$value" == "0" || "$value" == "null" ]]; then echo "$value"; else echo "$value"; fi; }
get_theme() {
    local theme_name="$1"; unset THEME; declare -g -A THEME
    case "${theme_name,,}" in
        red) for key in "${!RED_THEME[@]}"; do THEME[$key]="${RED_THEME[$key]}"; done ;;
        blue) for key in "${!BLUE_THEME[@]}"; do THEME[$key]="${BLUE_THEME[$key]}"; done ;;
        *) for key in "${!RED_THEME[@]}"; do THEME[$key]="${RED_THEME[$key]}"; done
           echo -e "${THEME[border_color]}Warning: Unknown theme '$theme_name', using Red${THEME[text_color]}" >&2 ;;
    esac
}
get_display_length() {
    local text="$1"
    local clean_text
    clean_text=$(echo -n "$text" | sed 's/\x1B\[[0-9;]*[mK]//g')
    echo "${#clean_text}"
}
format_with_commas() {
    local num="$1"
    local result="$num"
    while [[ $result =~ ^([0-9]+)([0-9]{3}.*) ]]; do
        result="${BASH_REMATCH[1]},${BASH_REMATCH[2]}"
    done
    echo "$result"
}
# shellcheck disable=SC2317  # Called indirectly via DATATYPE_HANDLERS
format_text() {
    local value="$1" format="$2" string_limit="$3" wrap_mode="$4" wrap_char="$5" justification="$6"
    [[ -z "$value" || "$value" == "null" ]] && { echo ""; return; }
    if [[ "$string_limit" -gt 0 && ${#value} -gt $string_limit ]]; then
        if [[ "$wrap_mode" == "wrap" && -n "$wrap_char" ]]; then
            local wrapped="" IFS="$wrap_char"; read -ra parts <<< "$value"
            for part in "${parts[@]}"; do wrapped+="$part\n"; done
            echo -e "$wrapped" | head -n $((string_limit / ${#wrap_char}))
        elif [[ "$wrap_mode" == "wrap" ]]; then echo "${value:0:$string_limit}"
        else
            case "$justification" in
                "right") echo "${value: -${string_limit}}";;
                "center") local start=$(( (${#value} - string_limit) / 2 )); echo "${value:${start}:${string_limit}}";;
                *) echo "${value:0:$string_limit}";;
            esac
        fi
    else echo "$value"; fi
}
# shellcheck disable=SC2317  # Called indirectly via DATATYPE_HANDLERS
format_number() { local value="$1" format="$2"; [[ -z "$value" || "$value" == "null" || "$value" == "0" ]] && { echo ""; return; }; [[ -n "$format" && "$value" =~ ^[0-9]+(\.[0-9]+)?$ ]] && printf '%s' "$value" || echo "$value"; }
# shellcheck disable=SC2317  # Called indirectly via DATATYPE_HANDLERS
format_num() {
    local value="$1" format="$2"
    if [[ -z "$value" || "$value" == "null" || "$value" == "0" ]]; then
        echo ""
        return
    fi
    if [[ "$value" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
        if [[ -n "$format" ]]; then
            printf '%s' "$value"
        else
            format_with_commas "$value"
        fi
    else
        echo "$value"
    fi
}
# shellcheck disable=SC2317  # Called indirectly via DATATYPE_HANDLERS
format_kcpu() {
    local value="$1" format="$2"
    [[ -z "$value" || "$value" == "null" ]] && { echo ""; return; }
    [[ "$value" == "0" || "$value" == "0m" ]] && { echo "0m"; return; }
    
    if [[ "$value" =~ ^[0-9]+m$ ]]; then
        local num_part="${value%m}"
        local formatted_num
        formatted_num=$(format_with_commas "$num_part")
        echo "${formatted_num}m"
    elif [[ "$value" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
        local int_value=${value%.*}
        [[ "$int_value" == "$value" ]] && int_value=$value
        local num_value=$((int_value * 1000))
        printf "%sm" "$(format_with_commas "$num_value")"
    else
        echo "$value"
    fi
}
# shellcheck disable=SC2317  # Called indirectly via DATATYPE_HANDLERS
format_kmem() {
    local value="$1" format="$2"
    [[ -z "$value" || "$value" == "null" ]] && { echo ""; return; }
    [[ "$value" =~ ^0[MKG]$ || "$value" == "0Mi" || "$value" == "0Gi" || "$value" == "0Ki" ]] && { echo "0M"; return; }
    
    if [[ "$value" =~ ^[0-9]+[KMG]$ ]]; then
        local num_part="${value%[KMG]}"
        local unit="${value: -1}"
        local formatted_num
        formatted_num=$(format_with_commas "$num_part")
        echo "${formatted_num}${unit}"
    elif [[ "$value" =~ ^[0-9]+Mi$ ]]; then
        local num_part="${value%Mi}"
        local formatted_num
        formatted_num=$(format_with_commas "$num_part")
        echo "${formatted_num}M"
    elif [[ "$value" =~ ^[0-9]+Gi$ ]]; then
        local num_part="${value%Gi}"
        local formatted_num
        formatted_num=$(format_with_commas "$num_part")
        echo "${formatted_num}G"
    elif [[ "$value" =~ ^[0-9]+Ki$ ]]; then
        local num_part="${value%Ki}"
        local formatted_num
        formatted_num=$(format_with_commas "$num_part")
        echo "${formatted_num}K"
    else
        echo "$value"
    fi
}
format_display_value() {
    local value="$1" null_value="$2" zero_value="$3" datatype="$4" format="$5" string_limit="$6" wrap_mode="$7" wrap_char="$8" justification="$9"
    local validate_fn="${DATATYPE_HANDLERS[${datatype}_validate]}" format_fn="${DATATYPE_HANDLERS[${datatype}_format]}"
    value=$("$validate_fn" "$value")
    local display_value
    display_value=$("$format_fn" "$value" "$format" "$string_limit" "$wrap_mode" "$wrap_char" "$justification")
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
declare -gx TABLE_TITLE="" TITLE_WIDTH=0 TITLE_POSITION="none"
declare -gx TABLE_FOOTER="" FOOTER_WIDTH=0 FOOTER_POSITION="none"
declare -ax HEADERS=() KEYS=() JUSTIFICATIONS=() DATATYPES=() NULL_VALUES=() ZERO_VALUES=()
declare -ax FORMATS=() SUMMARIES=() BREAKS=() STRING_LIMITS=() WRAP_MODES=() WRAP_CHARS=()
declare -ax PADDINGS=() WIDTHS=() SORT_KEYS=() SORT_DIRECTIONS=() SORT_PRIORITIES=()
declare -ax IS_WIDTH_SPECIFIED=() VISIBLES=()
validate_input_files() {
    local layout_file="$1" data_file="$2"
    [[ ! -s "$layout_file" || ! -s "$data_file" ]] && echo -e "${THEME[border_color]}Error: Layout or data JSON file empty/missing${THEME[text_color]}" >&2 && return 1
    return 0
}
parse_layout_file() {
    local layout_file="$1" columns_json sort_json
    THEME_NAME=$(jq -r '.theme // "Red"' "$layout_file")
    TABLE_TITLE=$(jq -r '.title // ""' "$layout_file")
    TITLE_POSITION=$(jq -r '.title_position // "none"' "$layout_file" | tr '[:upper:]' '[:lower:]')
    TABLE_FOOTER=$(jq -r '.footer // ""' "$layout_file")
    FOOTER_POSITION=$(jq -r '.footer_position // "none"' "$layout_file" | tr '[:upper:]' '[:lower:]')
    columns_json=$(jq -c '.columns // []' "$layout_file")
    sort_json=$(jq -c '.sort // []' "$layout_file")
    case "$TITLE_POSITION" in left|right|center|full|none) ;; *) echo -e "${THEME[border_color]}Warning: Invalid title position '$TITLE_POSITION', using 'none'${THEME[text_color]}" >&2; TITLE_POSITION="none";; esac
    case "$FOOTER_POSITION" in left|right|center|full|none) ;; *) echo -e "${THEME[border_color]}Warning: Invalid footer position '$FOOTER_POSITION', using 'none'${THEME[text_color]}" >&2; FOOTER_POSITION="none";; esac
    [[ -z "$columns_json" || "$columns_json" == "[]" ]] && echo -e "${THEME[border_color]}Error: No columns defined in layout JSON${THEME[text_color]}" >&2 && return 1
    parse_column_config "$columns_json"; parse_sort_config "$sort_json"
}
parse_column_config() {
    local columns_json="$1"
    HEADERS=(); KEYS=(); JUSTIFICATIONS=(); DATATYPES=(); NULL_VALUES=(); ZERO_VALUES=()
    FORMATS=(); SUMMARIES=(); BREAKS=(); STRING_LIMITS=(); WRAP_MODES=(); WRAP_CHARS=()
    PADDINGS=(); WIDTHS=(); IS_WIDTH_SPECIFIED=(); VISIBLES=()
    local column_count
    column_count=$(jq '. | length' <<<"$columns_json")
    COLUMN_COUNT=$column_count
    for ((i=0; i<column_count; i++)); do
        local col_json
        col_json=$(jq -c ".[$i]" <<<"$columns_json")
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
        local visible_key_check
        visible_key_check=$(jq -r 'has("visible")' <<<"$col_json")
        if [[ "$visible_key_check" == "true" ]]; then
            local visible_value
            visible_value=$(jq -r '.visible' <<<"$col_json")
            VISIBLES[i]="$visible_value"
        else
            VISIBLES[i]="$visible_raw"
        fi
        validate_column_config "$i" "${HEADERS[$i]}" "${JUSTIFICATIONS[$i]}" "${DATATYPES[$i]}" "${SUMMARIES[$i]}"
    done
    for ((i=0; i<COLUMN_COUNT; i++)); do
        local col_json
        col_json=$(jq -c ".[$i]" <<<"$columns_json")
        local specified_width
        specified_width=$(jq -r '.width // 0' <<<"$col_json")
        if [[ $specified_width -gt 0 ]]; then
            WIDTHS[i]=$specified_width; IS_WIDTH_SPECIFIED[i]="true"
        else
            WIDTHS[i]=$((${#HEADERS[i]} + (2 * PADDINGS[i]))); IS_WIDTH_SPECIFIED[i]="false"
        fi
    done
}
validate_column_config() {
    local i="$1" header="$2" justification="$3" datatype="$4" summary="$5"
    [[ -z "$header" ]] && echo -e "${THEME[border_color]}Error: Column $i has no header${THEME[text_color]}" >&2 && return 1
    [[ "$justification" != "left" && "$justification" != "right" && "$justification" != "center" ]] && echo -e "${THEME[border_color]}Warning: Invalid justification '$justification' for column $header, using 'left'${THEME[text_color]}" >&2 && JUSTIFICATIONS[i]="left"
    [[ -z "${DATATYPE_HANDLERS[${datatype}_validate]}" ]] && echo -e "${THEME[border_color]}Warning: Invalid datatype '$datatype' for column $header, using 'text'${THEME[text_color]}" >&2 && DATATYPES[i]="text"
    local valid_summaries="${DATATYPE_HANDLERS[${DATATYPES[$i]}_summary_types]}"
    [[ "$summary" != "none" && ! " $valid_summaries " =~ $summary ]] && echo -e "${THEME[border_color]}Warning: Summary '$summary' not supported for datatype '${DATATYPES[$i]}' in column $header, using 'none'${THEME[text_color]}" >&2 && SUMMARIES[i]="none"
}
parse_sort_config() {
    local sort_json="$1"
    SORT_KEYS=(); SORT_DIRECTIONS=(); SORT_PRIORITIES=()
    local sort_count
    sort_count=$(jq '. | length' <<<"$sort_json")
    for ((i=0; i<sort_count; i++)); do
        local sort_item
        sort_item=$(jq -c ".[$i]" <<<"$sort_json")
        SORT_KEYS[i]=$(jq -r '.key // ""' <<<"$sort_item")
        SORT_DIRECTIONS[i]=$(jq -r '.direction // "asc"' <<<"$sort_item" | tr '[:upper:]' '[:lower:]')
        SORT_PRIORITIES[i]=$(jq -r '.priority // 0' <<<"$sort_item")
        [[ -z "${SORT_KEYS[$i]}" ]] && echo -e "${THEME[border_color]}Warning: Sort item $i has no key, ignoring${THEME[text_color]}" >&2 && continue
        [[ "${SORT_DIRECTIONS[$i]}" != "asc" && "${SORT_DIRECTIONS[$i]}" != "desc" ]] && echo -e "${THEME[border_color]}Warning: Invalid sort direction '${SORT_DIRECTIONS[$i]}' for key ${SORT_KEYS[$i]}, using 'asc'${THEME[text_color]}" >&2 && SORT_DIRECTIONS[i]="asc"
    done
}
declare -a ROW_JSONS=()
declare -A SUM_SUMMARIES=() COUNT_SUMMARIES=() MIN_SUMMARIES=() MAX_SUMMARIES=()
declare -A UNIQUE_VALUES=() AVG_SUMMARIES=() AVG_COUNTS=()
declare -a IS_WIDTH_SPECIFIED=()
declare -a DATA_ROWS=()
initialize_summaries() {
    SUM_SUMMARIES=(); COUNT_SUMMARIES=(); MIN_SUMMARIES=(); MAX_SUMMARIES=()
    UNIQUE_VALUES=(); AVG_SUMMARIES=(); AVG_COUNTS=()
    for ((i=0; i<COLUMN_COUNT; i++)); do
        SUM_SUMMARIES[$i]=0; COUNT_SUMMARIES[$i]=0; MIN_SUMMARIES[$i]=""; MAX_SUMMARIES[$i]=""
        UNIQUE_VALUES[$i]=""; AVG_SUMMARIES[$i]=0; AVG_COUNTS[$i]=0
    done
}
prepare_data() {
    local data_file="$1"
    DATA_ROWS=()
    local data_json
    data_json=$(jq -c '. // []' "$data_file")
    local row_count
    row_count=$(jq '. | length' <<<"$data_json")
    [[ $row_count -eq 0 ]] && return
        local jq_expr=".[] | ["
        for key in "${KEYS[@]}"; do jq_expr+=".${key} // null,"; done
        jq_expr="${jq_expr%,}] | join(\"\t\")"
        local all_data
        all_data=$(jq -r "$jq_expr" "$data_file")
    IFS=$'\n' read -d '' -r -a rows <<< "$all_data"
    for ((i=0; i<row_count; i++)); do
        IFS=$'\t' read -r -a values <<< "${rows[$i]}"
        declare -A row_data
        for ((j=0; j<${#KEYS[@]}; j++)); do
            local key="${KEYS[$j]}" value="${values[$j]}"
            [[ "$value" == "null" ]] && value="null" || value="${value:-null}"
            row_data["$key"]="$value"
        done
        local row_data_str
        row_data_str=$(declare -p row_data)
        DATA_ROWS[i]="$row_data_str"
    done
}
sort_data() {
    [[ ${#SORT_KEYS[@]} -eq 0 ]] && return
    local indices=(); for ((i=0; i<${#DATA_ROWS[@]}; i++)); do indices+=("$i"); done
    get_sort_value() {
        local idx="$1" key="$2"
        declare -A row_data
        if ! eval "${DATA_ROWS[$idx]}"; then echo ""; return; fi
        if [[ -v "row_data[$key]" ]]; then echo "${row_data[$key]}"; else echo ""; fi
    }
    local primary_key="${SORT_KEYS[0]}" primary_dir="${SORT_DIRECTIONS[0]}"
    local sorted_indices=()
    IFS=$'\n' read -d '' -r -a sorted_indices < <(for idx in "${indices[@]}"; do
        value=$(get_sort_value "$idx" "$primary_key"); printf "%s\t%s\n" "$value" "$idx"
    done | sort -k1,1"${primary_dir:0:1}" | cut -f2)
    local temp_rows=("${DATA_ROWS[@]}"); DATA_ROWS=()
    for idx in "${sorted_indices[@]}"; do DATA_ROWS+=("${temp_rows[$idx]}"); done
}
process_data_rows() {
    local row_count; MAX_LINES=1; row_count=${#DATA_ROWS[@]}
    [[ $row_count -eq 0 ]] && return
    ROW_JSONS=()
    for ((i=0; i<row_count; i++)); do
        local row_json line_count=1
        row_json="{\"row\":$i}"; ROW_JSONS+=("$row_json")
        declare -A row_data
        if ! eval "${DATA_ROWS[$i]}"; then continue; fi
        for ((j=0; j<COLUMN_COUNT; j++)); do
            local key="${KEYS[$j]}" datatype="${DATATYPES[$j]}" format="${FORMATS[$j]}" string_limit="${STRING_LIMITS[$j]}" wrap_mode="${WRAP_MODES[$j]}" wrap_char="${WRAP_CHARS[$j]}"
            local validate_fn="${DATATYPE_HANDLERS[${datatype}_validate]}" format_fn="${DATATYPE_HANDLERS[${datatype}_format]}"
            local value="null"
            if [[ -v "row_data[$key]" ]]; then value="${row_data[$key]}"; fi
            value=$("$validate_fn" "$value")
            local display_value
            display_value=$("$format_fn" "$value" "$format" "$string_limit" "$wrap_mode" "$wrap_char")
            if [[ "$value" == "null" ]]; then
                case "${NULL_VALUES[$j]}" in 0) display_value="0";; missing) display_value="Missing";; *) display_value="";; esac
            elif [[ "$value" == "0" || "$value" == "0m" || "$value" == "0M" || "$value" == "0G" || "$value" == "0K" ]]; then
                case "${ZERO_VALUES[$j]}" in 0) display_value="0";; missing) display_value="Missing";; *) display_value="";; esac
            fi
            if [[ "${IS_WIDTH_SPECIFIED[j]}" != "true" && "${VISIBLES[j]}" == "true" ]]; then
                if [[ -n "$wrap_char" && "$wrap_mode" == "wrap" && -n "$display_value" && "$value" != "null" ]]; then
                    local max_len=0 IFS="$wrap_char"; read -ra parts <<<"$display_value"
                    for part in "${parts[@]}"; do
                        local len
                        len=$(get_display_length "$part")
                        [[ $len -gt $max_len ]] && max_len=$len
                    done
                    local padded_width=$((max_len + (2 * PADDINGS[j]))); [[ $padded_width -gt ${WIDTHS[j]} ]] && WIDTHS[j]=$padded_width
                    [[ ${#parts[@]} -gt $line_count ]] && line_count=${#parts[@]}
                else
                    local len
                    len=$(get_display_length "$display_value")
                    local padded_width=$((len + (2 * PADDINGS[j]))); [[ $padded_width -gt ${WIDTHS[j]} ]] && WIDTHS[j]=$padded_width
                fi
            fi
            update_summaries "$j" "$value" "${DATATYPES[$j]}" "${SUMMARIES[$j]}"
        done
        [[ $line_count -gt $MAX_LINES ]] && MAX_LINES=$line_count
    done
    for ((j=0; j<COLUMN_COUNT; j++)); do
        if [[ "${SUMMARIES[$j]}" != "none" ]]; then
            local summary_value="" datatype="${DATATYPES[$j]}" format="${FORMATS[$j]}"
            case "${SUMMARIES[$j]}" in
                sum)
                    if [[ -n "${SUM_SUMMARIES[$j]}" && "${SUM_SUMMARIES[$j]}" != "0" ]]; then
                        if [[ "$datatype" == "kcpu" ]]; then
                            local formatted_num
                            formatted_num=$(format_with_commas "${SUM_SUMMARIES[$j]}")
                            summary_value="${formatted_num}m"
                        elif [[ "$datatype" == "kmem" ]]; then
                            local formatted_num
                            formatted_num=$(format_with_commas "${SUM_SUMMARIES[$j]}")
                            summary_value="${formatted_num}M"
                        elif [[ "$datatype" == "num" ]]; then summary_value=$(format_num "${SUM_SUMMARIES[$j]}" "$format")
                        elif [[ "$datatype" == "int" || "$datatype" == "float" ]]; then summary_value="${SUM_SUMMARIES[$j]}"; [[ -n "$format" ]] && summary_value=$(printf '%s' "$summary_value"); fi
                    fi;;
                min) summary_value="${MIN_SUMMARIES[$j]:-}"; [[ -n "$format" ]] && summary_value=$(printf '%s' "$summary_value");;
                max) summary_value="${MAX_SUMMARIES[$j]:-}"; [[ -n "$format" ]] && summary_value=$(printf '%s' "$summary_value");;
                count) summary_value="${COUNT_SUMMARIES[$j]:-0}";;
                unique)
                    if [[ -n "${UNIQUE_VALUES[$j]}" ]]; then
                        local unique_count
                        unique_count=$(echo "${UNIQUE_VALUES[$j]}" | tr ' ' '\n' | sort -u | wc -l)
                        summary_value="$unique_count"
                    else summary_value="0"; fi;;
                avg)
                    if [[ -n "${AVG_SUMMARIES[$j]}" && "${AVG_COUNTS[$j]}" -gt 0 ]]; then
                        local avg_result
                        avg_result=$((${AVG_SUMMARIES[$j]} / ${AVG_COUNTS[$j]}))
                        if [[ "$datatype" == "int" ]]; then summary_value=$(printf "%.0f" "$avg_result")
                        elif [[ "$datatype" == "float" ]]; then
                            if [[ -n "$format" && "$format" =~ %.([0-9]+)f ]]; then local decimals="${BASH_REMATCH[1]}"; summary_value=$(printf "%.${decimals}f" "$avg_result")
                            else summary_value=$(printf "%.2f" "$avg_result"); fi
                        elif [[ "$datatype" == "num" ]]; then summary_value=$(format_num "$avg_result" "$format")
                        else summary_value="$avg_result"; fi
                    else summary_value="0"; fi;;
            esac
            if [[ -n "$summary_value" && "${IS_WIDTH_SPECIFIED[j]}" != "true" && "${VISIBLES[j]}" == "true" ]]; then
                local summary_len
                summary_len=$(get_display_length "$summary_value")
                local summary_padded_width=$((summary_len + (2 * PADDINGS[j])))
                [[ $summary_padded_width -gt ${WIDTHS[j]} ]] && WIDTHS[j]=$summary_padded_width
            fi
        fi
    done
}
update_summaries() {
    local j="$1" value="$2" datatype="$3" summary_type="$4"
    case "$summary_type" in
        sum)
            if [[ "$datatype" == "kcpu" && "$value" =~ ^[0-9]+m$ ]]; then SUM_SUMMARIES[$j]=$(( ${SUM_SUMMARIES[$j]:-0} + ${value%m} ))
            elif [[ "$datatype" == "kmem" ]]; then
                if [[ "$value" =~ ^[0-9]+M$ ]]; then SUM_SUMMARIES[$j]=$(( ${SUM_SUMMARIES[$j]:-0} + ${value%M} ))
                elif [[ "$value" =~ ^[0-9]+G$ ]]; then SUM_SUMMARIES[$j]=$(( ${SUM_SUMMARIES[$j]:-0} + ${value%G} * 1000 ))
                elif [[ "$value" =~ ^[0-9]+K$ ]]; then SUM_SUMMARIES[$j]=$(( ${SUM_SUMMARIES[$j]:-0} + ${value%K} / 1000 ))
                elif [[ "$value" =~ ^[0-9]+Mi$ ]]; then SUM_SUMMARIES[$j]=$(( ${SUM_SUMMARIES[$j]:-0} + ${value%Mi} ))
                elif [[ "$value" =~ ^[0-9]+Gi$ ]]; then SUM_SUMMARIES[$j]=$(( ${SUM_SUMMARIES[$j]:-0} + ${value%Gi} * 1000 )); fi
            elif [[ "$datatype" == "int" || "$datatype" == "num" ]]; then
                if [[ "$value" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then local int_value=${value%.*}; [[ "$int_value" == "$value" ]] && int_value=$value; SUM_SUMMARIES[$j]=$((${SUM_SUMMARIES[$j]:-0} + int_value)); fi
            elif [[ "$datatype" == "float" ]]; then
                if [[ "$value" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then SUM_SUMMARIES[$j]=$(echo "${SUM_SUMMARIES[$j]:-0} + $value" | bc); fi
            fi;;
        min)
            if [[ "$value" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
                if [[ -z "${MIN_SUMMARIES[$j]}" ]] || (( $(printf "%.0f" "$value") < $(printf "%.0f" "${MIN_SUMMARIES[$j]}") )); then MIN_SUMMARIES[$j]="$value"; fi
            fi;;
        max)
            if [[ "$value" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
                if [[ -z "${MAX_SUMMARIES[$j]}" ]] || (( $(printf "%.0f" "$value") > $(printf "%.0f" "${MAX_SUMMARIES[$j]}") )); then MAX_SUMMARIES[$j]="$value"; fi
            fi;;
        count) if [[ -n "$value" && "$value" != "null" ]]; then COUNT_SUMMARIES[$j]=$(( ${COUNT_SUMMARIES[$j]:-0} + 1 )); fi;;
        unique) if [[ -n "$value" && "$value" != "null" ]]; then
            if [[ -z "${UNIQUE_VALUES[$j]}" ]]; then UNIQUE_VALUES[$j]="$value"; else UNIQUE_VALUES[$j]+=" $value"; fi; fi;;
        avg)
            if [[ "$datatype" == "int" || "$datatype" == "float" || "$datatype" == "num" ]]; then
                if [[ "$value" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
                    local int_value=${value%.*}; [[ "$int_value" == "$value" ]] && int_value=$value; AVG_SUMMARIES[$j]=$((${AVG_SUMMARIES[$j]:-0} + int_value)); AVG_COUNTS[$j]=$(( ${AVG_COUNTS[$j]:-0} + 1 )); fi
            fi;;
    esac
}
calculate_title_width() {
    local title="$1" total_table_width="$2"
    if [[ -n "$title" ]]; then
        local evaluated_title
        evaluated_title=$(eval "echo \"$title\"")
        if [[ "$TITLE_POSITION" == "none" ]]; then TITLE_WIDTH=$((${#evaluated_title} + (2 * DEFAULT_PADDING)))
        elif [[ "$TITLE_POSITION" == "full" ]]; then TITLE_WIDTH=$total_table_width
        else TITLE_WIDTH=$((${#evaluated_title} + (2 * DEFAULT_PADDING))); [[ $TITLE_WIDTH -gt $total_table_width ]] && TITLE_WIDTH=$total_table_width; fi
    else TITLE_WIDTH=0; fi
}
calculate_footer_width() {
    local footer="$1" total_table_width="$2"
    if [[ -n "$footer" ]]; then
        local evaluated_footer
        evaluated_footer=$(eval "echo \"$footer\"")
        if [[ "$FOOTER_POSITION" == "none" ]]; then FOOTER_WIDTH=$((${#evaluated_footer} + (2 * DEFAULT_PADDING)))
        elif [[ "$FOOTER_POSITION" == "full" ]]; then FOOTER_WIDTH=$total_table_width
        else FOOTER_WIDTH=$((${#evaluated_footer} + (2 * DEFAULT_PADDING))); [[ $FOOTER_WIDTH -gt $total_table_width ]] && FOOTER_WIDTH=$total_table_width; fi
    else FOOTER_WIDTH=0; fi
}
calculate_table_width() {
    local total_table_width=0 visible_count=0
    for ((i=0; i<COLUMN_COUNT; i++)); do
        if [[ "${VISIBLES[i]}" == "true" ]]; then
            ((total_table_width += WIDTHS[i])); ((visible_count++))
        fi
    done
    [[ $visible_count -gt 1 ]] && ((total_table_width += visible_count - 1))
    echo "$total_table_width"
}
clip_text() {
    local text="$1" width="$2" justification="$3"
    if [[ ${#text} -le $width ]]; then
        echo "$text"
        return
    fi
    case "$justification" in
        right) echo "${text: -${width}}" ;;
        center) local excess=$(( ${#text} - width )); local left_clip=$(( excess / 2 )); echo "${text:${left_clip}:${width}}" ;;
        *) echo "${text:0:${width}}" ;;
    esac
}
render_cell() {
    local content="$1" width="$2" padding="$3" justification="$4" color="$5"
    local content_width=$((width - (2 * padding)))
    case "$justification" in
        right) printf "%*s${color}%*s${THEME[text_color]}%*s${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}" "$padding" "" "$content_width" "$content" "$padding" "" ;;
        center) local spaces=$(( (content_width - ${#content}) / 2 )); local left_spaces=$(( padding + spaces )); local right_spaces=$(( padding + content_width - ${#content} - spaces )); printf "%*s${color}%s${THEME[text_color]}%*s${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}" "$left_spaces" "" "$content" "$right_spaces" "" ;;
        *) printf "%*s${color}%-*s${THEME[text_color]}%*s${THEME[border_color]}${THEME[v_line]}${THEME[text_color]}" "$padding" "" "$content_width" "$content" "$padding" "" ;;
    esac
}
render_table_element() {
    local element_type="$1" total_table_width="$2"
    local element_text element_position element_width color_theme
    if [[ "$element_type" == "title" ]]; then
        [[ -z "$TABLE_TITLE" ]] && return
        element_text=$(eval echo "$TABLE_TITLE" 2>/dev/null)
        element_position="$TITLE_POSITION"
        calculate_title_width "$element_text" "$total_table_width"
        element_width="$TITLE_WIDTH"
        color_theme="${THEME[header_color]}"
    else
        [[ -z "$TABLE_FOOTER" ]] && return
        element_text=$(eval echo "$TABLE_FOOTER" 2>/dev/null)
        element_position="$FOOTER_POSITION"
        calculate_footer_width "$element_text" "$total_table_width"
        element_width="$FOOTER_WIDTH"
        color_theme="${THEME[footer_color]}"
    fi
    local offset=0
    case "$element_position" in
        left) offset=0 ;;
        right) offset=$((total_table_width - element_width)) ;;
        center) offset=$(((total_table_width - element_width) / 2)) ;;
        full) offset=0 ;;
        *) offset=0 ;;
    esac
    if [[ "$element_type" == "title" ]]; then
        [[ $offset -gt 0 ]] && printf "%*s" "$offset" ""
        printf "${THEME[border_color]}%s" "${THEME[tl_corner]}"
        printf "${THEME[h_line]}%.0s" $(seq 1 "$element_width")
        printf "%s${THEME[text_color]}\n" "${THEME[tr_corner]}"
    fi
    [[ $offset -gt 0 ]] && printf "%*s" "$offset" ""
    printf "${THEME[border_color]}%s${THEME[text_color]}" "${THEME[v_line]}"
    local available_width=$((element_width - (2 * DEFAULT_PADDING)))
    element_text=$(clip_text "$element_text" "$available_width" "$element_position")
    case "$element_position" in
        left) printf "%*s${color_theme}%-*s${THEME[text_color]}%*s" "$DEFAULT_PADDING" "" "$available_width" "$element_text" "$DEFAULT_PADDING" "" ;;
        right) printf "%*s${color_theme}%*s${THEME[text_color]}%*s" "$DEFAULT_PADDING" "" "$available_width" "$element_text" "$DEFAULT_PADDING" "" ;;
        center) printf "%*s${color_theme}%s${THEME[text_color]}%*s" "$DEFAULT_PADDING" "" "$element_text" "$((available_width - ${#element_text} + DEFAULT_PADDING))" "" ;;
        full) local text_len=${#element_text}; local spaces=$(( (available_width - text_len) / 2 )); local left_spaces=$(( DEFAULT_PADDING + spaces )); local right_spaces=$(( DEFAULT_PADDING + available_width - text_len - spaces )); printf "%*s${color_theme}%s${THEME[text_color]}%*s" "$left_spaces" "" "$element_text" "$right_spaces" "" ;;
        *) printf "%*s${color_theme}%s${THEME[text_color]}%*s" "$DEFAULT_PADDING" "" "$element_text" "$DEFAULT_PADDING" "" ;;
    esac
    printf "${THEME[border_color]}%s${THEME[text_color]}\n" "${THEME[v_line]}"
    if [[ "$element_type" == "footer" ]]; then
        [[ $offset -gt 0 ]] && printf "%*s" "$offset" ""
        echo -ne "${THEME[border_color]}${THEME[bl_corner]}"
        for i in $(seq 1 "$element_width"); do echo -ne "${THEME[h_line]}"; done
        echo -ne "${THEME[br_corner]}${THEME[text_color]}\n"
    fi
}
render_table_title() {
    render_table_element "title" "$1"
}
render_table_border() {
    local border_type="$1" total_table_width="$2" element_offset="$3" element_right_edge="$4" element_width="$5"
    local column_widths_sum=0 column_positions=()
    for ((i=0; i<COLUMN_COUNT-1; i++)); do
        if [[ "${VISIBLES[i]}" == "true" ]]; then
            column_widths_sum=$((column_widths_sum + WIDTHS[i]))
            local has_more_visible=false
            for ((j=$((i+1)); j<COLUMN_COUNT; j++)); do
                [[ "${VISIBLES[j]}" == "true" ]] && has_more_visible=true && break
            done
            [[ "$has_more_visible" == "true" ]] && column_positions+=("$column_widths_sum") && ((column_widths_sum++))
        fi
    done
    local max_width=$((total_table_width + 2))
    if [[ -n "$element_width" && $element_width -gt 0 ]]; then
        local adjusted_element_width=$((element_width + 2))
        [[ $adjusted_element_width -gt $max_width ]] && max_width=$adjusted_element_width
    fi
    local border_string=""
    local left_char right_char junction_char
    if [[ "$border_type" == "top" ]]; then
        left_char="${THEME[tl_corner]}"
        right_char="${THEME[tr_corner]}"
        junction_char="${THEME[t_junct]}"
    else
        left_char="${THEME[bl_corner]}"
        right_char="${THEME[br_corner]}"
        junction_char="${THEME[b_junct]}"
    fi
    if [[ -n "$element_width" && $element_width -gt 0 && $element_offset -eq 0 ]]; then
        left_char="${THEME[l_junct]}"
    fi
    for ((i=0; i<max_width; i++)); do
        local char_to_print="${THEME[h_line]}"
        if [[ $i -eq 0 ]]; then
            char_to_print="$left_char"
        elif [[ $i -eq $((max_width - 1)) ]]; then
            if [[ -n "$element_width" && $element_width -gt 0 && $element_right_edge -gt $total_table_width ]]; then
                if [[ "$border_type" == "top" ]]; then
                    char_to_print="${THEME[br_corner]}"
                else
                    char_to_print="${THEME[tr_corner]}"
                fi
            elif [[ -n "$element_width" && $element_width -gt 0 && $element_right_edge -eq $total_table_width ]]; then
                char_to_print="${THEME[r_junct]}"
            else
                char_to_print="$right_char"
            fi
        else
            for pos in "${column_positions[@]}"; do
                if [[ $((pos + 1)) -eq $i ]]; then
                    char_to_print="$junction_char"
                    break
                fi
            done
            if [[ -n "$element_width" && $element_width -gt 0 ]]; then
                if [[ $i -eq $element_offset && $element_offset -gt 0 && $element_offset -lt $((total_table_width + 1)) ]]; then
                    if [[ "$border_type" == "top" ]]; then
                        char_to_print="${THEME[b_junct]}"
                    else
                        char_to_print="${THEME[t_junct]}"
                    fi
                elif [[ $i -eq $((element_right_edge + 1)) && $((element_right_edge + 1)) -lt $((total_table_width + 1)) ]]; then
                    if [[ "$border_type" == "top" ]]; then
                        char_to_print="${THEME[b_junct]}"
                    else
                        char_to_print="${THEME[t_junct]}"
                    fi
                elif [[ $i -eq $((total_table_width + 1)) && $i -lt $((max_width - 1)) && $element_right_edge -gt $((total_table_width - 1)) ]]; then
                    if [[ "$border_type" == "top" ]]; then
                        char_to_print="${THEME[t_junct]}"
                    else
                        char_to_print="${THEME[b_junct]}"
                    fi
                fi
            fi
        fi
        border_string+="$char_to_print"
    done
    printf "${THEME[border_color]}%s${THEME[text_color]}\n" "$border_string"
}
render_table_top_border() {
    local total_table_width
    total_table_width=$(calculate_table_width)
    local title_offset=0 title_right_edge=0 title_width="" title_position="none"
    if [[ -n "$TABLE_TITLE" ]]; then
        title_width=$TITLE_WIDTH; title_position=$TITLE_POSITION
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
render_table_bottom_border() {
    local total_table_width
    total_table_width=$(calculate_table_width)
    local footer_offset=0 footer_right_edge=0 footer_width="" footer_position="none"
    if [[ -n "$TABLE_FOOTER" ]]; then
        calculate_footer_width "$TABLE_FOOTER" "$total_table_width"
        footer_width=$FOOTER_WIDTH; footer_position=$FOOTER_POSITION
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
render_table_headers() {
    printf "${THEME[border_color]}%s${THEME[text_color]}" "${THEME[v_line]}"
    for ((i=0; i<COLUMN_COUNT; i++)); do
        local visible="${VISIBLES[i]}"
        if [[ "$visible" == "true" ]]; then
            local header_text="${HEADERS[$i]}" width="${WIDTHS[i]}" padding="${PADDINGS[i]}" justification="${JUSTIFICATIONS[$i]}"
            local content_width=$((width - (2 * padding)))
            header_text=$(clip_text "$header_text" "$content_width" "$justification")
            render_cell "$header_text" "$width" "$padding" "$justification" "${THEME[caption_color]}"
        fi
    done
    printf "\n"
}
render_table_separator() {
    local type="$1"
    local left_char="${THEME[l_junct]}" right_char="${THEME[r_junct]}" middle_char="${THEME[cross]}"
    [[ "$type" == "bottom" ]] && left_char="${THEME[bl_corner]}" && right_char="${THEME[br_corner]}" && middle_char="${THEME[b_junct]}"
    printf "${THEME[border_color]}%s" "${left_char}"
    for ((i=0; i<COLUMN_COUNT; i++)); do
        if [[ "${VISIBLES[i]}" == "true" ]]; then
            local width=${WIDTHS[i]}
            for ((j=0; j<width; j++)); do printf "%s" "${THEME[h_line]}"; done
            if [[ $i -lt $((COLUMN_COUNT-1)) ]]; then
                local next_visible=false
                for ((k=$((i+1)); k<COLUMN_COUNT; k++)); do
                    if [[ "${VISIBLES[k]}" == "true" ]]; then next_visible=true; break; fi
                done
                [[ "$next_visible" == "true" ]] && printf "%s" "${middle_char}"
            fi
        fi
    done
    printf "%s${THEME[text_color]}\n" "${right_char}"
}
render_data_rows() {
    [[ ${#DATA_ROWS[@]} -eq 0 ]] && return
    local last_break_values=()
    for ((j=0; j<COLUMN_COUNT; j++)); do last_break_values[j]=""; done
    for ((row_idx=0; row_idx<${#DATA_ROWS[@]}; row_idx++)); do
        eval "${DATA_ROWS[$row_idx]}"
        # Check if we need a break
        local needs_break=false
        for ((j=0; j<COLUMN_COUNT; j++)); do
            if [[ "${BREAKS[$j]}" == "true" ]]; then
                local key="${KEYS[$j]}" value
                value="${row_data[$key]}"
                if [[ -n "${last_break_values[$j]}" && "$value" != "${last_break_values[$j]}" ]]; then
                    needs_break=true
                    break
                fi
            fi
        done
        if [[ "$needs_break" == "true" ]]; then
            render_table_separator "middle"
        fi
        local -A line_values
        local row_line_count=1
        for ((j=0; j<COLUMN_COUNT; j++)); do
            local key="${KEYS[$j]}" value
            value="${row_data[$key]}"
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
                    render_cell "$display_value" "${WIDTHS[j]}" "${PADDINGS[j]}" "${JUSTIFICATIONS[j]}" "${THEME[text_color]}"
                fi
            done
            printf "\n"
        done
        for ((j=0; j<COLUMN_COUNT; j++)); do
            if [[ "${BREAKS[$j]}" == "true" ]]; then
                local key="${KEYS[$j]}" value
                value="${row_data[$key]}"
                last_break_values[j]="$value"
            fi
        done
    done
}
render_table_footer() {
    render_table_element "footer" "$1"
}
render_summaries_row() {
    local has_summaries=false
    for ((i=0; i<COLUMN_COUNT; i++)); do
        [[ "${SUMMARIES[$i]}" != "none" ]] && has_summaries=true && break
    done
    if [[ "$has_summaries" == true ]]; then
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
                                formatted_num=$(format_with_commas "${SUM_SUMMARIES[$i]}")
                                summary_value="${formatted_num}m"
                            elif [[ "$datatype" == "kmem" ]]; then
                                local formatted_num
                                formatted_num=$(format_with_commas "${SUM_SUMMARIES[$i]}")
                                summary_value="${formatted_num}M"
                            elif [[ "$datatype" == "num" ]]; then
                                summary_value=$(format_num "${SUM_SUMMARIES[$i]}" "$format")
                            elif [[ "$datatype" == "int" ]]; then
                                summary_value="${SUM_SUMMARIES[$i]}"
                                [[ -n "$format" ]] && summary_value=$(printf "%s" "$format" | xargs printf "%s" "$summary_value")
                            elif [[ "$datatype" == "float" ]]; then
                                summary_value="${SUM_SUMMARIES[$i]}"
                                if [[ -n "$format" ]]; then
                                    summary_value=$(printf "%s" "$format" | xargs printf "%s" "$summary_value")
                                else
                                    summary_value=$(printf "%.3f" "$summary_value")
                                fi
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
                            summary_value=$(echo "${UNIQUE_VALUES[$i]}" | tr ' ' '\n' | sort -u | wc -l)
                        else
                            summary_value="0"
                        fi
                        ;;
                    avg)
                        if [[ -n "${AVG_SUMMARIES[$i]}" && "${AVG_COUNTS[$i]}" -gt 0 ]]; then
                            local avg_result
                            # Use bash arithmetic for division (will be integer division, but good enough for most cases)
                            avg_result=$((${AVG_SUMMARIES[$i]} / ${AVG_COUNTS[$i]}))
                            
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
                render_cell "$summary_value" "${WIDTHS[i]}" "${PADDINGS[i]}" "${JUSTIFICATIONS[i]}" "${THEME[summary_color]}"
            fi
        done
        printf "\n"
        return 0
    fi
    return 1
}
draw_table() {
    local layout_file="$1" data_file="$2"
    if [[ "$1" == "--help" || "$1" == "-h" ]]; then show_help; return 0; fi
    if [[ "$1" == "--version" ]]; then echo "tables.sh version $TABLES_VERSION"; return 0; fi
    if [[ $# -eq 0 ]]; then show_help; return 0; fi
    if [[ -z "$layout_file" || -z "$data_file" ]]; then
        echo "Error: Both layout and data files are required" >&2
        echo "Use --help for usage information" >&2
        return 1
    fi
    shift 2
    while [[ $# -gt 0 ]]; do
        case "$1" in
            *) echo "Error: Unknown option: $1" >&2; echo "Use --help for usage information" >&2; return 1 ;;
        esac
    done
    validate_input_files "$layout_file" "$data_file" || return 1
    parse_layout_file "$layout_file" || return 1
    get_theme "$THEME_NAME"
    initialize_summaries
    prepare_data "$data_file"
    sort_data
    process_data_rows
    local total_table_width
    total_table_width=$(calculate_table_width)
    [[ -n "$TABLE_TITLE" ]] && render_table_title "$total_table_width"
    render_table_top_border
    render_table_headers
    render_table_separator "middle"
    render_data_rows "$MAX_LINES"
    has_summaries=false
    render_summaries_row && has_summaries=true
    render_table_bottom_border
    [[ -n "$TABLE_FOOTER" ]] && render_table_footer "$total_table_width"
}
tables_main() {
    draw_table "$@"
}
tables_render() {
    local layout_file="$1" data_file="$2"
    shift 2
    draw_table "$layout_file" "$data_file" "$@"
}
tables_render_from_json() {
    local layout_json="$1" data_json="$2"
    shift 2
    local temp_layout temp_data
    temp_layout=$(mktemp)
    temp_data=$(mktemp)
    trap 'rm -f "$temp_layout" "$temp_data"' RETURN
    echo "$layout_json" > "$temp_layout"
    echo "$data_json" > "$temp_data"
    draw_table "$temp_layout" "$temp_data" "$@"
}
tables_get_themes() {
    echo "Available themes: Red, Blue"
}
tables_version() {
    echo "$TABLES_VERSION"
}
tables_reset() {
    COLUMN_COUNT=0 ; MAX_LINES=1 ; THEME_NAME="Red" ; TABLE_TITLE="" ; TITLE_WIDTH=0 ; TITLE_POSITION="none" ; TABLE_FOOTER="" ; FOOTER_WIDTH=0 ; FOOTER_POSITION="none"
    HEADERS=() ; KEYS=() ; JUSTIFICATIONS=() ; DATATYPES=() ; NULL_VALUES=() ; ZERO_VALUES=() ; FORMATS=() ; SUMMARIES=() ; IS_WIDTH_SPECIFIED=() ; VISIBLES=() 
    BREAKS=() ; STRING_LIMITS=() ; WRAP_MODES=() ; WRAP_CHARS=() ; PADDINGS=() ; WIDTHS=() ; SORT_KEYS=() ; SORT_DIRECTIONS=() ; SORT_PRIORITIES=() ; ROW_JSONS=() ; DATA_ROWS=()
    SUM_SUMMARIES=() ; COUNT_SUMMARIES=() ; MIN_SUMMARIES=() ; MAX_SUMMARIES=() ; UNIQUE_VALUES=() ; AVG_SUMMARIES=() ; AVG_COUNTS=()    
    get_theme "$THEME_NAME"
}
if [[ "${BASH_SOURCE[0]}" != "$0" ]]; then
    export -f tables_render
    export -f tables_render_from_json
    export -f tables_get_themes
    export -f tables_version
    export -f tables_reset
    export -f draw_table
    export -f get_theme
    export -f format_with_commas
    export -f get_display_length
else
    tables_main "$@"
fi