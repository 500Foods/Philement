#!/usr/bin/env bash

# ABOUT
#
# tables.sh
#
# A reusable library for rendering JSON data as ANSI tables in bash scripts.
# Created with the help of AI assistants like xAI/Grok 3 and Anthropic/Claude Sonnet 4

# USAGE
#
# From another script:
#   source /path/to/tables.sh
#   draw_table "layout.json" "data.json" [--debug]
#
# Command line:
#   ./tables.sh layout.json data.json [--debug] [--version] [--help]

# VERSION INFORMATION
declare -r TABLES_VERSION="1.0.2"

# VERSION HISTORY
# 1.0.3 - Visible: false columns are now excluded from width calculations
# 1.0.2 - Added help functionality and version history section
# 1.0.1 - Fixed shellcheck issues (SC2004, SC2155)
# 1.0.0 - Initial release with table rendering functionality

# Debug flag - can be set by parent script
DEBUG_FLAG=false

# Global variables
declare -g COLUMN_COUNT=0
declare -g MAX_LINES=1
declare -g THEME_NAME="Red"
declare -g DEFAULT_PADDING=1

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Source all library modules
source "$SCRIPT_DIR/lib/tables_themes.sh"
source "$SCRIPT_DIR/lib/tables_datatypes.sh"
source "$SCRIPT_DIR/lib/tables_config.sh"
source "$SCRIPT_DIR/lib/tables_data.sh"

# Debug logger function
debug_log() {
    [[ "$DEBUG_FLAG" == "true" ]] && echo "[DEBUG] $*" >&2
}

# show_help: Display usage information and available options
show_help() {
    cat << 'EOF'
tables.sh - Reusable table-drawing library for rendering JSON data as ANSI tables

USAGE:
    tables.sh <layout_json_file> <data_json_file> [OPTIONS]
    tables.sh [--help] [--version]

ARGUMENTS:
    layout_json_file    Path to JSON file containing table layout configuration
    data_json_file      Path to JSON file containing table data

OPTIONS:
    --debug            Enable debug logging to stderr
    --version          Show version information
    --help             Show this help message

EXAMPLES:
    # Basic table rendering
    tables.sh layout.json data.json

    # With debug output
    tables.sh layout.json data.json --debug

    # Show version
    tables.sh --version

DEPENDENCIES:
    - jq (JSON processor)
    - bash 4.0+
    - awk
    - sed

For more information, see the documentation in tables.md
EOF
}

# calculate_title_width: Calculate the width needed for the title
# Returns: width of the title section
calculate_title_width() {
    local title="$1"
    local total_table_width="$2"
    
    if [[ -n "$title" ]]; then
        # Evaluate any embedded commands in the title
        local evaluated_title
        evaluated_title=$(eval "echo \"$title\"")
        if [[ "$TITLE_POSITION" == "none" ]]; then
            TITLE_WIDTH=$((${#evaluated_title} + (2 * DEFAULT_PADDING)))
        elif [[ "$TITLE_POSITION" == "full" ]]; then
            TITLE_WIDTH=$total_table_width
        else
            TITLE_WIDTH=$((${#evaluated_title} + (2 * DEFAULT_PADDING)))
            [[ $TITLE_WIDTH -gt $total_table_width ]] && TITLE_WIDTH=$total_table_width
        fi
    else
        TITLE_WIDTH=0
    fi
}

# calculate_footer_width: Calculate the width needed for the footer
# Returns: width of the footer section
calculate_footer_width() {
    local footer="$1"
    local total_table_width="$2"
    
    if [[ -n "$footer" ]]; then
        # Evaluate any embedded commands in the footer
        local evaluated_footer
        evaluated_footer=$(eval "echo \"$footer\"")
        if [[ "$FOOTER_POSITION" == "none" ]]; then
            FOOTER_WIDTH=$((${#evaluated_footer} + (2 * DEFAULT_PADDING)))
        elif [[ "$FOOTER_POSITION" == "full" ]]; then
            FOOTER_WIDTH=$total_table_width
        else
            FOOTER_WIDTH=$((${#evaluated_footer} + (2 * DEFAULT_PADDING)))
            [[ $FOOTER_WIDTH -gt $total_table_width ]] && FOOTER_WIDTH=$total_table_width
        fi
    else
        FOOTER_WIDTH=0
    fi
}

# calculate_table_width: Calculate the total width of the table
# Returns: total width of the table in characters, considering only visible columns
calculate_table_width() {
    local width=0
    local visible_count=0
    for ((i=0; i<COLUMN_COUNT; i++)); do
        if [[ "${VISIBLES[i]}" == "true" ]]; then
            ((width += WIDTHS[i]))
            ((visible_count++))
        fi
    done
    # Add separators only between visible columns
    if [[ $visible_count -gt 1 ]]; then
        ((width += visible_count - 1))
    fi
    echo "$width"
}

# Source the rendering module (this is large, so we'll create it separately)
source "$SCRIPT_DIR/lib/tables_render.sh"

# draw_table: Main function to render an ANSI table from JSON layout and data files
# Args: layout_json_file (path to layout JSON), data_json_file (path to data JSON),
#       [--debug] (enable debug logs), [--version] (show version), [--help] (show help)
# Outputs: ANSI table to stdout, errors and debug messages to stderr
draw_table() {
    local layout_file="$1" data_file="$2" debug=false
    
    # Handle help and version options without requiring files
    if [[ "$1" == "--help" || "$1" == "-h" ]]; then
        show_help
        return 0
    fi
    
    if [[ "$1" == "--version" ]]; then
        echo "tables.sh version $TABLES_VERSION"
        return 0
    fi
    
    # Check if no arguments provided
    if [[ $# -eq 0 ]]; then
        show_help
        return 0
    fi
    
    # Check if required files are provided
    if [[ -z "$layout_file" || -z "$data_file" ]]; then
        echo "Error: Both layout and data files are required" >&2
        echo "Use --help for usage information" >&2
        return 1
    fi
    
    shift 2
    while [[ $# -gt 0 ]]; do
        case "$1" in
            --debug) debug=true; shift ;;
            --version) echo "tables.sh version $TABLES_VERSION"; return 0 ;;
            --help|-h) show_help; return 0 ;;
            *) echo "Error: Unknown option: $1" >&2; echo "Use --help for usage information" >&2; return 1 ;;
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
    local total_table_width
    total_table_width=$(calculate_table_width)
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
