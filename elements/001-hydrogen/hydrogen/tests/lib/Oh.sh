#!/bin/bash

# oh.sh - Convert ANSI terminal output to GitHub-compatible SVG
# 
# CHANGELOG
# 1.001 - Updated Cache directory
# 1.000 - Initial release with basic functionality
# 0.026 - Fix XML character escaping in xml_escape function; improve width calculation reporting with auto-width limits
# 0.025 - Fix --width to enforce grid width and clip lines; add debug logging for clipping
# 0.024 - Fix syntax error in generate_svg loop; add empty input check; enhance debug logging for cell_width and height truncation
# 0.023 - Implement grid-based layout for font-agnostic alignment; add --width, --wrap, --height,
#         --font-width, --font-height, --font-weight; remove font-specific width calculations;
#         process wrapping/truncation early; clip text exceeding grid
# 0.022 - Fix XML parsing error in URL, fix parse_ansi_line to avoid over-accumulating visible_column
# 0.021 - Make parse_ansi_line generic, adjust Inconsolata font ratio
# 0.020 - Fix alignment by normalizing filename column position, remove expand_tabs
# 0.019 - Fix ShellCheck errors (unclosed quotes, parameter expansions)
# 0.018 - Retyped entire script to eliminate hidden characters causing unclosed quote errors
# 0.017 - Retype main function to fix hidden character causing unclosed quote error
# 0.016 - Fix syntax error in main function (unclosed single quote)
# 0.015 - Fix tab expansion by handling it in a separate pass before ANSI parsing
# 0.014 - Fix tab expansion to account for ANSI codes properly (attempted but incomplete)
# 0.013 - Fix position calculation to not count ANSI escape sequences as visible characters
# 0.012 - Debug positioning and fix overly aggressive XML escaping
# 0.011 - Switch to integer-based positioning to fix alignment drift
# 0.010 - Fix character counting to exclude ANSI codes from dimension calculations
# 0.009 - Add xml:space="preserve" to maintain exact whitespace spacing
# 0.008 - Fix XML encoding in URLs and proper tab handling for column alignment
# 0.007 - Font selection and accurate character width calculations
# 0.006 - ANSI color parsing and SVG styling
# 0.005 - Strip ANSI escape sequences to fix XML parsing errors
# 0.004 - Input handling and basic text rendering to SVG
# 0.003 - SVG output pipeline with file/stdout support
# 0.002 - Better help handling, version output, and empty input detection
# 0.001 - Initial framework with argument parsing and help system

set -euo pipefail

# MetaData
SCRIPT_NAME="Oh.sh"
SCRIPT_VERSION="1.001"

# Default values
INPUT_FILE=""
OUTPUT_FILE=""
DEBUG=false
SCRIPT_START=$(date +%s.%N)

# Caching 
CACHE_DIR="${HOME}/.cache/Oh"
mkdir -p "${CACHE_DIR}" 

# SVG defaults
FONT_SIZE=14
FONT_FAMILY="Consolas"
FONT_WIDTH=$(echo "scale=2; ${FONT_SIZE} * 0.6" | bc)  # Default character width
FONT_HEIGHT=$(echo "scale=2; ${FONT_SIZE} * 1.2" | bc)  # Default line height
FONT_WEIGHT=400
WIDTH=80  # Default grid width in characters
HEIGHT=0  # Default gird height in lines
WRAP=false
PADDING=20
BG_COLOR="#1e1e1e"
TEXT_COLOR="#ffffff"
TAB_SIZE=8  # Standard tab stop every 8 characters

# Default storage
declare -a INPUT_LINES
declare -a LINE_SEGMENTS
declare -a HASH_CACHE

# Multi-argument option storage
declare -A MULTI_ARGS

# Character width ratios for common fonts (used as fallback for --font-width)
declare -A FONT_CHAR_RATIOS
FONT_CHAR_RATIOS["Consolas"]=0.6
FONT_CHAR_RATIOS["Monaco"]=0.6
FONT_CHAR_RATIOS["Courier New"]=0.6
FONT_CHAR_RATIOS["Inconsolata"]=0.60
FONT_CHAR_RATIOS["JetBrains Mono"]=0.55
FONT_CHAR_RATIOS["Source Code Pro"]=0.55
FONT_CHAR_RATIOS["Fira Code"]=0.58
FONT_CHAR_RATIOS["Roboto Mono"]=0.6
FONT_CHAR_RATIOS["Ubuntu Mono"]=0.5
FONT_CHAR_RATIOS["Menlo"]=0.6

# Google Fonts (URLs will be XML-escaped)
declare -A GOOGLE_FONTS
GOOGLE_FONTS["Inconsolata"]="https://fonts.googleapis.com/css2?family=Inconsolata:wght@400;700&display=swap"
GOOGLE_FONTS["JetBrains Mono"]="https://fonts.googleapis.com/css2?family=JetBrains+Mono:wght@400;700&display=swap"
GOOGLE_FONTS["Source Code Pro"]="https://fonts.googleapis.com/css2?family=Source+Code+Pro:wght@400;700&display=swap"
GOOGLE_FONTS["Fira Code"]="https://fonts.googleapis.com/css2?family=Fira+Code:wght@400;700&display=swap"
GOOGLE_FONTS["Roboto Mono"]="https://fonts.googleapis.com/css2?family=Roboto+Mono:wght@400;700&display=swap"

# ANSI color mappings (standard 16 colors)
declare -A ANSI_COLORS
ANSI_COLORS[30]="#000000"   # Black
ANSI_COLORS[31]="#cd3131"   # Red
ANSI_COLORS[32]="#0dbc79"   # Green  
ANSI_COLORS[33]="#e5e510"   # Yellow
ANSI_COLORS[34]="#2472c8"   # Blue
ANSI_COLORS[35]="#bc3fbc"   # Magenta
ANSI_COLORS[36]="#11a8cd"   # Cyan
ANSI_COLORS[37]="#e5e5e5"   # White
# Bright colors (90-97)
ANSI_COLORS[90]="#666666"   # Bright Black (Gray)
ANSI_COLORS[91]="#f14c4c"   # Bright Red
ANSI_COLORS[92]="#23d18b"   # Bright Green
ANSI_COLORS[93]="#f5f543"   # Bright Yellow
ANSI_COLORS[94]="#3b8eea"   # Bright Blue
ANSI_COLORS[95]="#d670d6"   # Bright Magenta
ANSI_COLORS[96]="#29b8db"   # Bright Cyan
ANSI_COLORS[97]="#e5e5e5"   # Bright White

log_output() {
    local message="$1"
    local elapsed_time
    elapsed_time=$(echo "$(date +%s.%N) - ${SCRIPT_START}" | bc || true)
    elapsed_time_formatted=$(printf "%07.3f" "${elapsed_time}")
    echo "${elapsed_time_formatted} - ${message}" >&2
}

show_version() {
    echo "${SCRIPT_NAME}   - v${SCRIPT_VERSION} - Convert ANSI terminal output to GitHub-compatible SVG" >&2
}

show_help() {
    show_version
    cat >&2 << HELP_EOF

USAGE:
    command | oh.sh [OPTIONS] > output.svg
    oh.sh [OPTIONS] -i input.txt -o output.svg

OPTIONS:
    -h, --help              Show this help
    -i, --input FILE        Input file (default: stdin)
    -o, --output FILE       Output file (default: stdout)
    --font FAMILY           Font family (default: Consolas)
    --font-size SIZE        Font size in pixels (default: 14)
    --font-width PX         Character width in pixels (default: 0.6 * font-size)
    --font-height PX        Line height in pixels (default: 1.2 * font-size)
    --font-weight WEIGHT    Font weight (default: 400)
    --width CHARS           Grid width in characters (default: 80)
    --height CHARS          Grid height in lines (default: input line count)
    --wrap                  Wrap lines at width (default: false)
    --tab-size SIZE         Tab stop size (default: 8)
    --debug                 Enable debug output
    --version               Show version information
    
SUPPORTED FONTS:
    Consolas, Monaco, Courier New (system fonts)
    Inconsolata, JetBrains Mono, Source Code Pro, 
    Fira Code, Roboto Mono (Google Fonts - embedded automatically)
    Font metric defaults are editable in the script.

EXAMPLES:
    ls --color=always -l | ${SCRIPT_NAME} > listing.svg
    git diff --color | ${SCRIPT_NAME} --font "JetBrains Mono" --font-size 16 -o diff.svg
    ${SCRIPT_NAME} --font Inconsolata --width 60 --wrap -i terminal-output.txt -o styled.svg

HELP_EOF
}

check_for_help() {
    for arg in "$@"; do
        if [[ "${arg}" == "-h" || "${arg}" == "--help" ]]; then
            show_help
            exit 0
        fi
    done
}

parse_arguments() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -v|--version)
                exit 0
                ;;
            -i|--input)
                [[ $# -lt 2 || "$2" =~ ^- ]] && { echo "Error: --input requires a filename" >&2; exit 1; }
                INPUT_FILE="$2"
                shift
                ;;
            -o|--output)
                [[ $# -lt 2 || "$2" =~ ^- ]] && { echo "Error: --output requires a filename" >&2; exit 1; }
                OUTPUT_FILE="$2"
                shift
                ;;
            --font)
                [[ $# -lt 2 || "$2" =~ ^- ]] && { echo "Error: --font requires a font family name" >&2; exit 1; }
                FONT_FAMILY="$2"
                shift
                ;;
            --font-size)
                [[ $# -lt 2 || "$2" =~ ^- ]] && { echo "Error: --font-size requires a number" >&2; exit 1; }
                [[ ! "$2" =~ ^[0-9]+$ || "$2" -lt 8 || "$2" -gt 72 ]] && { echo "Error: --font-size must be a number between 8 and 72" >&2; exit 1; }
                FONT_SIZE="$2"
                # Update defaults for font-width/height if not overridden
                FONT_WIDTH=$(echo "scale=2; ${FONT_SIZE} * ${FONT_CHAR_RATIOS[${FONT_FAMILY}]:-0.6}" | bc)
                FONT_HEIGHT=$(echo "scale=2; ${FONT_SIZE} * 1.2" | bc)
                shift
                ;;
            --font-width)
                [[ $# -lt 2 || "$2" =~ ^- ]] && { echo "Error: --font-width requires a number" >&2; exit 1; }
                [[ ! "$2" =~ ^[0-9]+(\.[0-9]+)?$ || "$(echo "$2 < 1" | bc || true)" -eq 1 ]] && { echo "Error: --font-width must be a number >= 1" >&2; exit 1; }
                FONT_WIDTH="$2"
                shift
                ;;
            --font-height)
                [[ $# -lt 2 || "$2" =~ ^- ]] && { echo "Error: --font-height requires a number" >&2; exit 1; }
                [[ ! "$2" =~ ^[0-9]+(\.[0-9]+)?$ || "$(echo "$2 < 1" | bc || true)" -eq 1 ]] && { echo "Error: --font-height must be a number >= 1" >&2; exit 1; }
                FONT_HEIGHT="$2"
                shift
                ;;
            --font-weight)
                [[ $# -lt 2 || "$2" =~ ^- ]] && { echo "Error: --font-weight requires a number" >&2; exit 1; }
                [[ ! "$2" =~ ^[0-9]+$ || "$2" -lt 100 || "$2" -gt 900 ]] && { echo "Error: --font-weight must be a number between 100 and 900" >&2; exit 1; }
                FONT_WEIGHT="$2"
                shift
                ;;
            --width)
                [[ $# -lt 2 || "$2" =~ ^- ]] && { echo "Error: --width requires a number" >&2; exit 1; }
                [[ ! "$2" =~ ^[0-9]+$ || "$2" -lt 1 ]] && { echo "Error: --width must be a number >= 1" >&2; exit 1; }
                WIDTH="$2"
                shift
                ;;
            --height)
                [[ $# -lt 2 || "$2" =~ ^- ]] && { echo "Error: --height requires a number" >&2; exit 1; }
                [[ ! "$2" =~ ^[0-9]+$ || "$2" -lt 1 ]] && { echo "Error: --height must be a number >= 1" >&2; exit 1; }
                HEIGHT="$2"
                shift
                ;;
            --wrap)
                WRAP=true
                ;;
            --tab-size)
                [[ $# -lt 2 || "$2" =~ ^- ]] && { echo "Error: --tab-size requires a number" >&2; exit 1; }
                [[ ! "$2" =~ ^[0-9]+$ || "$2" -lt 1 || "$2" -gt 16 ]] && { echo "Error: --tab-size must be a number between 1 and 16" >&2; exit 1; }
                TAB_SIZE="$2"
                shift
                ;;
            --debug)
                DEBUG=true
                ;;
            --border)
                shift
                BORDER_ARGS=()
                while [[ $# -gt 0 && ! "$1" =~ ^- ]]; do
                    BORDER_ARGS+=("$1")
                    shift
                done
                MULTI_ARGS[border]="${BORDER_ARGS[*]}"
                continue
                ;;
            -*)
                echo "Error: Unknown option '$1'" >&2
                echo "Use -h or --help for usage information" >&2
                exit 1
                ;;
            *)
                echo "Error: Unexpected argument '$1'" >&2
                echo "Use -h or --help for usage information" >&2
                exit 1
                ;;
        esac
        shift
    done
}

# Adler-32 hash function (pure Bash, 32-bit)
adler32_hash() {
    local data="$1"
    local MOD=65521
    local a=1 b=0
    local len=${#data}
    local i=0
    local byte

    while ((i < len)); do
        byte=$(printf '%d' "'${data:i:1}")
        a=$(( (a + (byte & 0xFF)) % MOD ))
        b=$(( (b + a) % MOD ))
        ((i++))
    done

    local hash=$(( (b << 16) | a ))
    printf '%08x' "${hash}"
}

xml_escape_url() {
    local input="$1"
    input="${input//&/&amp;}"
    echo "${input}"
}

build_font_css() {
    local font="$1"
    local css_family="'${font}'"
    local css=""
    if [[ -n "${GOOGLE_FONTS[${font}]:-}" ]]; then
        local escaped_url
        escaped_url=$(xml_escape_url "${GOOGLE_FONTS[${font}]}")
        css="@import url('${escaped_url}');"
    fi
    css_family="${css_family}, 'Consolas', 'Monaco', 'Courier New', monospace"
    css="${css} .terminal-text { font-family: ${css_family};"
    [[ "${FONT_WEIGHT}" != 400 ]] && css="${css} font-weight: ${FONT_WEIGHT};"
    css="${css} }"
    echo "${css}"
}

xml_escape() {
    local input="$1"
    # Must escape ampersands first, then other entities
    input=${input//\&/\&amp;}
    input=${input//</\&lt;}
    input=${input//>/\&gt;}
    input=${input//\"/\&quot;}
    local sq="'"
    input=${input//${sq}/\&apos;}
    echo "${input}"
}

parse_ansi_line() {
    local line="$1"
    local segments=()
    local current_text=""
    local current_fg="${TEXT_COLOR}"
    local current_bg=""
    local current_bold=false
    local i=0
    local visible_column=0
    
    [[ "${DEBUG}" == true ]] && log_output "Parsing line (${#line} chars): ${line}" | cat -v
    
    while [[ ${i} -lt ${#line} ]]; do
        if [[ "${line:${i}:1}" == $'\e' ]] && [[ "${line:${i}+1:1}" == "[" ]]; then
            if [[ -n "${current_text}" ]]; then
                [[ "${DEBUG}" == true ]] && log_output "  Emitting segment: \"${current_text}\" (${#current_text} chars) at column ${visible_column} (fg=${current_fg}, bold=${current_bold})"
                segments+=("$(printf '%s|%s|%s|%s|%d' "${current_text}" "${current_fg}" "${current_bg}" "${current_bold}" "${visible_column}")")
                visible_column=$((visible_column + ${#current_text}))
                current_text=""
                [[ "${DEBUG}" == true ]] && log_output "  Updated visible_column to ${visible_column}"
            fi
            
            while [[ ${i} -lt ${#line} && "${line:${i}:1}" == $'\e' && "${line:${i}+1:1}" == "[" ]]; do
                i=$((i + 2))
                local ansi_codes=""
                
                while [[ ${i} -lt ${#line} ]]; do
                    local char="${line:${i}:1}"
                    ansi_codes+="${char}"
                    i=$((i + 1))
                    [[ "${char}" =~ [a-zA-Z] ]] && break
                done
                
                [[ "${DEBUG}" == true ]] && log_output "  ANSI codes: ${ansi_codes}" 
                
                if [[ "${ansi_codes: -1}" == "m" ]]; then
                    ansi_codes="${ansi_codes%?}"
                    IFS=";" read -ra codes <<< "${ansi_codes}"
                    for code in "${codes[@]}"; do
                        code=$(echo "${code}" | tr -d -c '0-9')
                        code=$((10#${code}))
                        [[ "${DEBUG}" == true ]] && log_output "    Processing code: ${code}"
                        if [[ -z "${code}" || "${code}" == 0 ]]; then
                            current_fg="${TEXT_COLOR}"
                            current_bg=""
                            current_bold=false
                            [[ "${DEBUG}" == true ]] && log_output "    Reset: fg=${current_fg}, bg=${current_bg}, bold=${current_bold}"
                        elif [[ "${code}" == 1 ]]; then
                            current_bold=true
                            [[ "${DEBUG}" == true ]] && log_output "    Set bold=${current_bold}"
                        elif [[ "${code}" == 22 ]]; then
                            current_bold=false
                            [[ "${DEBUG}" == true ]] && log_output "    Unset bold=${current_bold}"
                        elif [[ -n "${ANSI_COLORS[${code}]:-}" ]]; then
                            current_fg="${ANSI_COLORS[${code}]}"
                            [[ "${DEBUG}" == true ]] && log_output "    Set fg=${current_fg}"
                        elif [[ ${code} -ge 40 && ${code} -le 47 || ${code} -ge 100 && ${code} -le 107 ]]; then
                            local bg_code=$((code - 10))
                            current_bg="${ANSI_COLORS[${bg_code}]:-}"
                            [[ "${DEBUG}" == true ]] && log_output "    Set bg=${current_bg}"
                        fi
                    done
                fi
            done
        else
            current_text+="${line:${i}:1}"
            i=$((i + 1))
        fi
    done
    
    if [[ -n "${current_text}" ]]; then
        [[ "${DEBUG}" == true ]] && log_output "  Emitting segment: \"${current_text}\" (${#current_text} chars) at column ${visible_column} (fg=${current_fg}, bold=${current_bold})"
        segments+=("$(printf '%s|%s|%s|%s|%d' "${current_text}" "${current_fg}" "${current_bg}" "${current_bold}" "${visible_column}")")
    fi
    
    LINE_SEGMENTS=("${segments[@]}")
}

get_visible_line_length() {
    local line="$1"
    local total_chars=0
    
    parse_ansi_line "${line}"
    for segment in "${LINE_SEGMENTS[@]}"; do
        IFS='|' read -r text fg bg bold _ <<< "${segment}"
        total_chars=$((total_chars + ${#text}))
    done
    
    echo "${total_chars}"
}

read_input() {
    local input_source

    log_output "Reading source input"
    
    if [[ -n "${INPUT_FILE}" ]]; then
        [[ ! -f "${INPUT_FILE}" ]] && { echo "Error: Input file '${INPUT_FILE}' not found" >&2; exit 1; }
        input_source="${INPUT_FILE}"
    else
        input_source="/dev/stdin"
    fi
    
    # Convert tabs to spaces
    local tab_spaces
    printf -v tab_spaces '%*s' "${TAB_SIZE}" ''
    tab_spaces=${tab_spaces// / }
    
    # Read input, expanding tabs
    while IFS= read -r line || [[ -n "${line}" ]]; do
        line="${line//$'\t'/${tab_spaces}}"
        INPUT_LINES+=("${line}")
    done < "${input_source}"
    
    log_output "Read ${#INPUT_LINES[@]} lines from ${input_source}"

    # Check for empty input
    if [[ ${#INPUT_LINES[@]} -eq 0 ]]; then
        echo "Error: No input provided" >&2
        exit 1
    fi
    
    # Apply height truncation if specified
    if [[ "${HEIGHT}" -gt 0 && "${#INPUT_LINES[@]}" -gt "${HEIGHT}" ]]; then
        INPUT_LINES=("${INPUT_LINES[@]:0:${HEIGHT}}")
        [[ "${DEBUG}" == true ]] && log_output "Truncated to ${HEIGHT} lines"
    fi
    
    # Apply wrapping if enabled
    if [[ "${WRAP}" == true ]]; then
        local wrapped_lines=()
        for line in "${INPUT_LINES[@]}"; do
            if [[ "${#line}" -gt "${WIDTH}" ]]; then
                echo "${line}" | fold -w "${WIDTH}" | while IFS= read -r wrapped_line; do
                    wrapped_lines+=("${wrapped_line}")
                done
            else
                wrapped_lines+=("${line}")
            fi
        done
        INPUT_LINES=("${wrapped_lines[@]}")
    fi
    
    log_output "Hashing ${#INPUT_LINES[@]} lines after wrapping/truncation"

    # Set HEIGHT to line count if not specified
    [[ "${HEIGHT}" == 0 ]] && HEIGHT="${#INPUT_LINES[@]}"
    
    # Let's make some hashes
    start_hash_time=$(date +%s.%N)

    declare -a procs=()
    for line in "${INPUT_LINES[@]}"; do
        procs+=("<(echo -n \"${line}\")")  # Quote to handle special chars; -n avoids extra newline
    done    

    while IFS= read -r hash; do
        HASH_CACHE+=("${hash}")
    done < <(eval "md5sum ${procs[*]} | cut -d' ' -f1" || true)  # eval expands the procs; process sub for input

    end_hash_time=$(date +%s.%N)
    elapsed_hash_time=$(echo "${end_hash_time} - ${start_hash_time}" | bc)
    num_hash_lines=${#INPUT_LINES[@]}
    if (( num_hash_lines > 0 )); then
        hash_time_per_line=$(echo "scale=6; ${elapsed_hash_time} / ${num_hash_lines}" | bc)
    else
        hash_time_per_line=0
    fi

    # for i in "${!HASH_CACHE[@]}"; do
    #     log_output "Line ${i} hash: ${HASH_CACHE[i]}"
    # done

    # Format times to three decimal places
    elapsed_hash_time_formatted=$(printf "%.3f" "${elapsed_hash_time}")
    hash_time_per_line_formatted=$(printf "%.3f" "${hash_time_per_line}")

    log_output "Hash time: ${elapsed_hash_time_formatted}s, Time per line: ${hash_time_per_line_formatted}s"
    log_output "Processing ${num_hash_lines} lines"
    
}

calculate_dimensions() {
    local max_width=0
    local char_count
    local max_auto_width=120  # Maximum auto-detected width
    local longest_line_index=-1
    
    for i in "${!INPUT_LINES[@]}"; do
        char_count=$(get_visible_line_length "${INPUT_LINES[i]}")
        if [[ ${char_count} -gt ${max_width} ]]; then
            max_width=${char_count}
            longest_line_index=${i}
        fi
    done
    
    log_output "Content analysis: longest line is ${max_width} characters (line $((longest_line_index + 1)))" 
    [[ "${DEBUG}" == true ]] && log_output "  Longest line content: \"${INPUT_LINES[longest_line_index]:0:50}...\""
    
    # Use actual content width if WIDTH is still the default (80) and content is wider
    if [[ "${WIDTH}" == 80 && ${max_width} -gt 80 ]]; then
        if [[ ${max_width} -gt ${max_auto_width} ]]; then
            GRID_WIDTH="${max_auto_width}"
            log_output "Auto-detected width limited to ${max_auto_width} characters (content: ${max_width} chars)" 
            log_output "  Use --width ${max_width} to display full content width" 
        else
            GRID_WIDTH="${max_width}"
            log_output "Auto-detected width: ${max_width} characters" 
        fi
    else
        GRID_WIDTH="${WIDTH}"
        log_output "Using specified width: ${WIDTH} characters" 
        # Warn if clipping will occur
        if [[ "${WRAP}" == false && ${max_width} -gt ${WIDTH} ]]; then
            log_output "Warning: Lines exceed width ${WIDTH} (max: ${max_width}), will clip" 
        fi
    fi
    
    GRID_HEIGHT="${HEIGHT}"
    
    # Ensure minimum dimensions
    [[ "${GRID_WIDTH}" -lt 1 ]] && GRID_WIDTH=1
    [[ "${GRID_HEIGHT}" -lt 1 ]] && GRID_HEIGHT=1
    
    SVG_WIDTH=$(echo "scale=2; (2 * ${PADDING}) + (${GRID_WIDTH} * ${FONT_WIDTH})" | bc)
    SVG_HEIGHT=$(echo "scale=2; (2 * ${PADDING}) + (${GRID_HEIGHT} * ${FONT_HEIGHT})" | bc)
    
    log_output "SVG dimensions: ${SVG_WIDTH}x${SVG_HEIGHT} (${GRID_HEIGHT} lines, grid width: ${GRID_WIDTH} chars)" 
    log_output "Font: ${FONT_FAMILY} ${FONT_SIZE}px (char width: ${FONT_WIDTH}, line height: ${FONT_HEIGHT}, weight: ${FONT_WEIGHT})" 
}

# shellcheck disable=SC2312 # Got a script within a script
generate_svg() {
    local cell_width
    cell_width=$(echo "scale=2; (${SVG_WIDTH} - 2 * ${PADDING}) / ${GRID_WIDTH}" | bc)
    [[ "${DEBUG}" == true ]] && log_output "Cell width: ${cell_width} pixels"
    cat << EOF
<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" 
     width="${SVG_WIDTH}" 
     height="${SVG_HEIGHT}" 
     viewBox="0 0 ${SVG_WIDTH} ${SVG_HEIGHT}">
     
  <defs>
    <style>
$(build_font_css "${FONT_FAMILY}") 
    </style>
  </defs>
     
  <!-- Background -->
  <rect width="100%" height="100%" fill="${BG_COLOR}" rx="6"/>
  
  <!-- Text Content -->
EOF

    for i in "${!INPUT_LINES[@]}"; do
        [[ ${i} -ge ${GRID_HEIGHT} ]] && { [[ "${DEBUG}" == true ]] && log_output "Skipping line ${i} (exceeds grid height ${GRID_HEIGHT})"; break; }
        local line="${INPUT_LINES[i]}"
        [[ "${DEBUG}" == true ]] && log_otuput "Processing line ${i}: ${#line} chars"
        parse_ansi_line "${line}"
        
        local y_offset
        y_offset=$(echo "scale=2; ${PADDING} + (${FONT_SIZE} + (${i} * ${FONT_HEIGHT}))" | bc)
        
        for segment in "${LINE_SEGMENTS[@]}"; do
            IFS='|' read -r text fg bg bold visible_pos <<< "${segment}"
            
            if [[ -n "${text}" ]]; then
                # Clip text if it exceeds grid width
                local max_chars=$((GRID_WIDTH - visible_pos))
                if [[ ${max_chars} -le 0 ]]; then
                    [[ "${DEBUG}" == true ]] && log_output "  Skipping text at col ${visible_pos} (exceeds grid width ${GRID_WIDTH})"
                    continue
                fi
                if [[ ${#text} -gt ${max_chars} ]]; then
                    [[ "${DEBUG}" == true ]] && log_output "  Clipping text at col ${visible_pos}: '${text:0:20}'... to ${max_chars} chars"
                    text="${text:0:${max_chars}}"
                fi
                [[ -z "${text}" ]] && continue
                
                local escaped_text
                escaped_text=$(xml_escape "${text}")
                
                local current_x
                local text_width
                current_x=$(echo "scale=2; ${PADDING} + (${visible_pos} * ${cell_width})" | bc)
                text_width=$(echo "scale=2; ${#text} * ${cell_width}" | bc)
                [[ "${DEBUG}" == true ]] && log_output "  Placing text at x=${current_x} (col ${visible_pos}): \"${text:0:20}\"..." "(${#text} chars)"
                
                local style_attrs="fill=\"${fg}\""
                [[ "${bold}" == "true" ]] && style_attrs+=" font-weight=\"bold\""
                
                if [[ -n "${bg}" ]]; then
                    cat << EOF
    <rect x="${current_x}" y="$(echo "scale=2; ${y_offset} - ${FONT_SIZE} + 2" | bc)" width="${text_width}" height="$(echo "scale=2; ${FONT_SIZE} + 2" | bc)" fill="${bg}"/>
EOF
                fi
                
                cat << EOF
    <text x="${current_x}" y="${y_offset}" font-size="${FONT_SIZE}" class="terminal-text" xml:space="preserve" textLength="${text_width}" lengthAdjust="spacingAndGlyphs" ${style_attrs}>${escaped_text}</text>
EOF
            fi
        done
    done
    
    echo "</svg>"
}

output_svg() {
    local svg_content
    svg_content=$(generate_svg)
    
    if [[ -n "${OUTPUT_FILE}" ]]; then
        echo "${svg_content}" > "${OUTPUT_FILE}"
        log_output "SVG written to: ${OUTPUT_FILE}"
    else
        echo "${svg_content}"
    fi
}

main() {
    check_for_help "$@"

    if [[ $# -eq 0 && -t 0 ]]; then
        show_help
        exit 0
    fi

    show_version

    parse_arguments "$@"

    log_output "Parsed options:" 
    log_output "  Input: ${INPUT_FILE:-stdin}" 
    log_output "  Output: ${OUTPUT_FILE:-stdout}" 
    log_output "  Font: ${FONT_FAMILY} ${FONT_SIZE}px (width: ${FONT_WIDTH}, height: ${FONT_HEIGHT}, weight: ${FONT_WEIGHT})" 
    log_output "  Grid: ${WIDTH}x${HEIGHT}" 
    log_output "  Wrap: ${WRAP}" 
    log_output "  Tab size: ${TAB_SIZE}"

    if [[ -n "${MULTI_ARGS[border]:-}" ]]; then
        echo "  Border: ${MULTI_ARGS[border]}" 
    fi

    read_input
    calculate_dimensions
    output_svg

    log_output "Oh.sh v${SCRIPT_VERSION} SVG generation complete! 🎯"
}

main "$@"
