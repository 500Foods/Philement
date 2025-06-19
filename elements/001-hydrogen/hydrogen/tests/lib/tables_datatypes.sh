#!/usr/bin/env bash

# tables_datatypes.sh: Data type system for tables.sh
# Handles validation, formatting, and summary functions for different data types

# Datatype registry: Maps datatypes to validation, formatting, and summary functions
# Format: [datatype_function]="function_name"
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
    local value="$1" format="$2" string_limit="$3" wrap_mode="$4" wrap_char="$5" justification="$6"
    [[ -z "$value" || "$value" == "null" ]] && { echo ""; return; }
    if [[ "$string_limit" -gt 0 && ${#value} -gt $string_limit ]]; then
        if [[ "$wrap_mode" == "wrap" && -n "$wrap_char" ]]; then
            local wrapped=""
            local IFS="$wrap_char"
            read -ra parts <<< "$value"
            for part in "${parts[@]}"; do
                wrapped+="$part
"
            done
            echo -e "$wrapped" | head -n $((string_limit / ${#wrap_char}))
        elif [[ "$wrap_mode" == "wrap" ]]; then
            # Word wrapping logic could be implemented here if needed
            echo "${value:0:$string_limit}"
        else
            case "$justification" in
                "right")
                    echo "${value: -${string_limit}}"
                    ;;
                "center")
                    local start=$(( (${#value} - string_limit) / 2 ))
                    echo "${value:${start}:${string_limit}}"
                    ;;
                *)
                    echo "${value:0:$string_limit}"
                    ;;
            esac
        fi
    else
        echo "$value"
    fi
}

format_number() {
    local value="$1" format="$2"
    [[ -z "$value" || "$value" == "null" || "$value" == "0" ]] && { echo ""; return; }
    if [[ -n "$format" && "$value" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
        printf '%s' "$value"
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
            printf '%s' "$value"
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
        local formatted_num
        formatted_num=$(echo "$num_part" | awk '{ printf "%\047d", $0 }')
        echo "${formatted_num}m"
    elif [[ "$value" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
        local num_value
        num_value=$(awk "BEGIN {print $value * 1000}")
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
        local formatted_num
        formatted_num=$(echo "$num_part" | awk '{ printf "%\047d", $0 }')
        echo "${formatted_num}${unit}"
    elif [[ "$value" =~ ^[0-9]+Mi$ ]]; then
        local num_part="${value%Mi}"
        local formatted_num
        formatted_num=$(echo "$num_part" | awk '{ printf "%\047d", $0 }')
        echo "${formatted_num}M"
    elif [[ "$value" =~ ^[0-9]+Gi$ ]]; then
        local num_part="${value%Gi}"
        local formatted_num
        formatted_num=$(echo "$num_part" | awk '{ printf "%\047d", $0 }')
        echo "${formatted_num}G"
    elif [[ "$value" =~ ^[0-9]+Ki$ ]]; then
        local num_part="${value%Ki}"
        local formatted_num
        formatted_num=$(echo "$num_part" | awk '{ printf "%\047d", $0 }')
        echo "${formatted_num}K"
    else
        echo ""
    fi
}

# format_display_value: Format a cell value for display
# Args: value, null_value, zero_value, datatype, format, string_limit, wrap_mode, wrap_char, justification
# Returns: formatted display value
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
