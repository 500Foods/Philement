#!/bin/bash

# oh.sh - Convert ANSI terminal output to GitHub-compatible SVG
# 
# CHANGELOG
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

# Version
VERSION="0.026"

# Default values
INPUT_FILE=""
OUTPUT_FILE=""
# HELP variable removed as it's unused
DEBUG=false

# SVG defaults
FONT_SIZE=14
FONT_FAMILY="Consolas"
FONT_WIDTH=$(echo "scale=2; $FONT_SIZE * 0.6" | bc)  # Default character width
FONT_HEIGHT=$(echo "scale=2; $FONT_SIZE * 1.2" | bc)  # Default line height
FONT_WEIGHT=400
WIDTH=80  # Default grid width in characters
HEIGHT=0  # Set to input line count by default
WRAP=false
PADDING=20
BG_COLOR="#1e1e1e"
TEXT_COLOR="#ffffff"
TAB_SIZE=8  # Standard tab stop every 8 characters

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

show_version() {
    echo "Oh.sh - Convert ANSI terminal output to GitHub-compatible SVG" >&2
    echo "Version $VERSION" >&2
}

show_help() {
    cat >&2 << 'EOF'
Oh.sh - Convert ANSI terminal output to GitHub-compatible SVG

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
    
SUPPORTED FONTS:
    Consolas, Monaco, Courier New (system fonts)
    Inconsolata, JetBrains Mono, Source Code Pro, 
    Fira Code, Roboto Mono (Google Fonts - embedded automatically)
    Font metric defaults are editable in the script.

EXAMPLES:
    ls --color=always -l | oh.sh > listing.svg
    git diff --color | oh.sh --font "JetBrains Mono" --font-size 16 -o diff.svg
    oh.sh --font Inconsolata --width 60 --wrap -i terminal-output.txt -o styled.svg

EOF
}

check_for_help() {
    for arg in "$@"; do
        if [[ "$arg" == "-h" || "$arg" == "--help" ]]; then
            show_help
            exit 0
        fi
    done
}

parse_arguments() {
    while [[ $# -gt 0 ]]; do
        case $1 in
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
                FONT_WIDTH=$(echo "scale=2; $FONT_SIZE * ${FONT_CHAR_RATIOS[$FONT_FAMILY]:-0.6}" | bc)
                FONT_HEIGHT=$(echo "scale=2; $FONT_SIZE * 1.2" | bc)
                shift
                ;;
            --font-width)
                [[ $# -lt 2 || "$2" =~ ^- ]] && { echo "Error: --font-width requires a number" >&2; exit 1; }
                [[ ! "$2" =~ ^[0-9]+(\.[0-9]+)?$ || "$(echo "$2 < 1" | bc)" -eq 1 ]] && { echo "Error: --font-width must be a number >= 1" >&2; exit 1; }
                FONT_WIDTH="$2"
                shift
                ;;
            --font-height)
                [[ $# -lt 2 || "$2" =~ ^- ]] && { echo "Error: --font-height requires a number" >&2; exit 1; }
                [[ ! "$2" =~ ^[0-9]+(\.[0-9]+)?$ || "$(echo "$2 < 1" | bc)" -eq 1 ]] && { echo "Error: --font-height must be a number >= 1" >&2; exit 1; }
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

xml_escape_url() {
    local input="$1"
    input="${input//&/&amp;}"
    echo "$input"
}

build_font_css() {
    local font="$1"
    local css_family="'$font'"
    local css=""
    if [[ -n "${GOOGLE_FONTS[$font]:-}" ]]; then
        local escaped_url
        escaped_url=$(xml_escape_url "${GOOGLE_FONTS[$font]}")
        css="@import url('$escaped_url');"
    fi
    css_family="$css_family, 'Consolas', 'Monaco', 'Courier New', monospace"
    css="$css .terminal-text { font-family: $css_family;"
    [[ "$FONT_WEIGHT" != 400 ]] && css="$css font-weight: $FONT_WEIGHT;"
    css="$css }"
    echo "$css"
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
    echo "$input"
}

parse_ansi_line() {
    local line="$1"
    local segments=()
    local current_text=""
    local current_fg="$TEXT_COLOR"
    local current_bg=""
    local current_bold=false
    local i=0
    local visible_column=0
    
    [[ "$DEBUG" == true ]] && echo "Parsing line (${#line} chars): $line" | cat -v >&2
    
    while [[ $i -lt ${#line} ]]; do
        if [[ "${line:$i:1}" == $'\e' ]] && [[ "${line:$i+1:1}" == "[" ]]; then
            if [[ -n "$current_text" ]]; then
                [[ "$DEBUG" == true ]] && echo "  Emitting segment: \"$current_text\" (${#current_text} chars) at column $visible_column (fg=$current_fg, bold=$current_bold)" >&2
                segments+=("$(printf '%s|%s|%s|%s|%d' "$current_text" "$current_fg" "$current_bg" "$current_bold" "$visible_column")")
                visible_column=$((visible_column + ${#current_text}))
                current_text=""
                [[ "$DEBUG" == true ]] && echo "  Updated visible_column to $visible_column" >&2
            fi
            
            while [[ $i -lt ${#line} && "${line:$i:1}" == $'\e' && "${line:$i+1:1}" == "[" ]]; do
                i=$((i + 2))
                local ansi_codes=""
                
                while [[ $i -lt ${#line} ]]; do
                    local char="${line:$i:1}"
                    ansi_codes+="$char"
                    i=$((i + 1))
                    [[ "$char" =~ [a-zA-Z] ]] && break
                done
                
                [[ "$DEBUG" == true ]] && echo "  ANSI codes: $ansi_codes" >&2
                
                if [[ "${ansi_codes: -1}" == "m" ]]; then
                    ansi_codes="${ansi_codes%?}"
                    IFS=";" read -ra codes <<< "$ansi_codes"
                    for code in "${codes[@]}"; do
                        code=$(echo "$code" | tr -d -c '0-9')
                        code=$((10#$code))
                        [[ "$DEBUG" == true ]] && echo "    Processing code: $code" >&2
                        if [[ -z "$code" || "$code" == 0 ]]; then
                            current_fg="$TEXT_COLOR"
                            current_bg=""
                            current_bold=false
                            [[ "$DEBUG" == true ]] && echo "    Reset: fg=$current_fg, bg=$current_bg, bold=$current_bold" >&2
                        elif [[ "$code" == 1 ]]; then
                            current_bold=true
                            [[ "$DEBUG" == true ]] && echo "    Set bold=$current_bold" >&2
                        elif [[ "$code" == 22 ]]; then
                            current_bold=false
                            [[ "$DEBUG" == true ]] && echo "    Unset bold=$current_bold" >&2
                        elif [[ -n "${ANSI_COLORS[$code]:-}" ]]; then
                            current_fg="${ANSI_COLORS[$code]}"
                            [[ "$DEBUG" == true ]] && echo "    Set fg=$current_fg" >&2
                        elif [[ $code -ge 40 && $code -le 47 || $code -ge 100 && $code -le 107 ]]; then
                            local bg_code=$((code - 10))
                            current_bg="${ANSI_COLORS[$bg_code]:-}"
                            [[ "$DEBUG" == true ]] && echo "    Set bg=$current_bg" >&2
                        fi
                    done
                fi
            done
        else
            current_text+="${line:$i:1}"
            i=$((i + 1))
        fi
    done
    
    if [[ -n "$current_text" ]]; then
        [[ "$DEBUG" == true ]] && echo "  Emitting segment: \"$current_text\" (${#current_text} chars) at column $visible_column (fg=$current_fg, bold=$current_bold)" >&2
        segments+=("$(printf '%s|%s|%s|%s|%d' "$current_text" "$current_fg" "$current_bg" "$current_bold" "$visible_column")")
    fi
    
    LINE_SEGMENTS=("${segments[@]}")
}

get_visible_line_length() {
    local line="$1"
    local total_chars=0
    
    parse_ansi_line "$line"
    for segment in "${LINE_SEGMENTS[@]}"; do
        IFS='|' read -r text fg bg bold _ <<< "$segment"
        total_chars=$((total_chars + ${#text}))
    done
    
    echo "$total_chars"
}

read_input() {
    local -a lines=()
    local input_source
    
    if [[ -n "$INPUT_FILE" ]]; then
        [[ ! -f "$INPUT_FILE" ]] && { echo "Error: Input file '$INPUT_FILE' not found" >&2; exit 1; }
        input_source="$INPUT_FILE"
    else
        input_source="/dev/stdin"
    fi
    
    # Convert tabs to spaces
    local tab_spaces
    printf -v tab_spaces '%*s' "$TAB_SIZE" ''
    tab_spaces=${tab_spaces// / }
    
    # Read input, expanding tabs
    while IFS= read -r line || [[ -n "$line" ]]; do
        line="${line//$'\t'/$tab_spaces}"
        lines+=("$line")
    done < "$input_source"
    
    # Check for empty input
    if [[ ${#lines[@]} -eq 0 ]]; then
        echo "Error: No input provided" >&2
        exit 1
    fi
    
    # Apply height truncation if specified
    if [[ "$HEIGHT" -gt 0 && "${#lines[@]}" -gt "$HEIGHT" ]]; then
        lines=("${lines[@]:0:$HEIGHT}")
        [[ "$DEBUG" == true ]] && echo "Truncated to $HEIGHT lines" >&2
    fi
    
    # Apply wrapping if enabled
    if [[ "$WRAP" == true ]]; then
        local wrapped_lines=()
        for line in "${lines[@]}"; do
            if [[ "${#line}" -gt "$WIDTH" ]]; then
                echo "$line" | fold -w "$WIDTH" | while IFS= read -r wrapped_line; do
                    wrapped_lines+=("$wrapped_line")
                done
            else
                wrapped_lines+=("$line")
            fi
        done
        lines=("${wrapped_lines[@]}")
    fi
    
    # Set HEIGHT to line count if not specified
    [[ "$HEIGHT" == 0 ]] && HEIGHT="${#lines[@]}"
    
    INPUT_LINES=("${lines[@]}")
    echo "Read ${#lines[@]} lines from $input_source" >&2
}

calculate_dimensions() {
    local max_width=0
    local char_count
    local max_auto_width=120  # Maximum auto-detected width
    local longest_line_index=-1
    
    for i in "${!INPUT_LINES[@]}"; do
        char_count=$(get_visible_line_length "${INPUT_LINES[i]}")
        if [[ $char_count -gt $max_width ]]; then
            max_width=$char_count
            longest_line_index=$i
        fi
    done
    
    echo "Content analysis: longest line is $max_width characters (line $((longest_line_index + 1)))" >&2
    [[ "$DEBUG" == true ]] && echo "  Longest line content: \"${INPUT_LINES[longest_line_index]:0:50}...\"" >&2
    
    # Use actual content width if WIDTH is still the default (80) and content is wider
    if [[ "$WIDTH" == 80 && $max_width -gt 80 ]]; then
        if [[ $max_width -gt $max_auto_width ]]; then
            GRID_WIDTH="$max_auto_width"
            echo "Auto-detected width limited to $max_auto_width characters (content: $max_width chars)" >&2
            echo "  Use --width $max_width to display full content width" >&2
        else
            GRID_WIDTH="$max_width"
            echo "Auto-detected width: $max_width characters" >&2
        fi
    else
        GRID_WIDTH="$WIDTH"
        echo "Using specified width: $WIDTH characters" >&2
        # Warn if clipping will occur
        if [[ "$WRAP" == false && $max_width -gt $WIDTH ]]; then
            echo "Warning: Lines exceed width $WIDTH (max: $max_width), will clip" >&2
        fi
    fi
    
    GRID_HEIGHT="$HEIGHT"
    
    # Ensure minimum dimensions
    [[ "$GRID_WIDTH" -lt 1 ]] && GRID_WIDTH=1
    [[ "$GRID_HEIGHT" -lt 1 ]] && GRID_HEIGHT=1
    
    SVG_WIDTH=$(echo "scale=2; (2 * $PADDING) + ($GRID_WIDTH * $FONT_WIDTH)" | bc)
    SVG_HEIGHT=$(echo "scale=2; (2 * $PADDING) + ($GRID_HEIGHT * $FONT_HEIGHT)" | bc)
    
    echo "SVG dimensions: ${SVG_WIDTH}x${SVG_HEIGHT} ($GRID_HEIGHT lines, grid width: $GRID_WIDTH chars)" >&2
    echo "Font: $FONT_FAMILY ${FONT_SIZE}px (char width: $FONT_WIDTH, line height: $FONT_HEIGHT, weight: $FONT_WEIGHT)" >&2
}

generate_svg() {
    local cell_width
    cell_width=$(echo "scale=2; ($SVG_WIDTH - 2 * $PADDING) / $GRID_WIDTH" | bc)
    [[ "$DEBUG" == true ]] && echo "Cell width: $cell_width pixels" >&2
    
    cat << EOF
<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" 
     width="${SVG_WIDTH}" 
     height="${SVG_HEIGHT}" 
     viewBox="0 0 ${SVG_WIDTH} ${SVG_HEIGHT}">
     
  <defs>
    <style>
$(build_font_css "$FONT_FAMILY")
    </style>
  </defs>
     
  <!-- Background -->
  <rect width="100%" height="100%" fill="${BG_COLOR}" rx="6"/>
  
  <!-- Text Content -->
EOF

    for i in "${!INPUT_LINES[@]}"; do
        [[ $i -ge $GRID_HEIGHT ]] && { [[ "$DEBUG" == true ]] && echo "Skipping line $i (exceeds grid height $GRID_HEIGHT)" >&2; break; }
        local line="${INPUT_LINES[i]}"
        [[ "$DEBUG" == true ]] && echo "Processing line $i: ${#line} chars" >&2
        parse_ansi_line "$line"
        
        local y_offset
        y_offset=$(echo "scale=2; $PADDING + ($FONT_SIZE + ($i * $FONT_HEIGHT))" | bc)
        
        for segment in "${LINE_SEGMENTS[@]}"; do
            IFS='|' read -r text fg bg bold visible_pos <<< "$segment"
            
            if [[ -n "$text" ]]; then
                # Clip text if it exceeds grid width
                local max_chars=$((GRID_WIDTH - visible_pos))
                if [[ $max_chars -le 0 ]]; then
                    [[ "$DEBUG" == true ]] && echo "  Skipping text at col $visible_pos (exceeds grid width $GRID_WIDTH)" >&2
                    continue
                fi
                if [[ ${#text} -gt $max_chars ]]; then
                    [[ "$DEBUG" == true ]] && echo "  Clipping text at col $visible_pos: '${text:0:20}'... to $max_chars chars" >&2
                    text="${text:0:$max_chars}"
                fi
                [[ -z "$text" ]] && continue
                
                local escaped_text
                escaped_text=$(xml_escape "$text")
                
                local current_x
                local text_width
                current_x=$(echo "scale=2; $PADDING + ($visible_pos * $cell_width)" | bc)
                text_width=$(echo "scale=2; ${#text} * $cell_width" | bc)
                [[ "$DEBUG" == true ]] && echo "  Placing text at x=$current_x (col $visible_pos): \"${text:0:20}\"..." "(${#text} chars)" >&2
                
                local style_attrs="fill=\"$fg\""
                [[ "$bold" == "true" ]] && style_attrs+=" font-weight=\"bold\""
                
                if [[ -n "$bg" ]]; then
                    cat << EOF
    <rect x="$current_x" y="$(echo "scale=2; $y_offset - $FONT_SIZE + 2" | bc)" width="$text_width" height="$(echo "scale=2; $FONT_SIZE + 2" | bc)" fill="$bg"/>
EOF
                fi
                
                cat << EOF
    <text x="$current_x" y="$y_offset" font-size="$FONT_SIZE" class="terminal-text" xml:space="preserve" textLength="$text_width" lengthAdjust="spacingAndGlyphs" $style_attrs>$escaped_text</text>
EOF
            fi
        done
    done
    
    echo "</svg>"
}

output_svg() {
    local svg_content
    svg_content=$(generate_svg)
    
    if [[ -n "$OUTPUT_FILE" ]]; then
        echo "$svg_content" > "$OUTPUT_FILE"
        echo "SVG written to: $OUTPUT_FILE" >&2
    else
        echo "$svg_content"
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

    echo "Parsed options:" >&2
    echo "  Input: ${INPUT_FILE:-stdin}" >&2
    echo "  Output: ${OUTPUT_FILE:-stdout}" >&2
    echo "  Font: $FONT_FAMILY ${FONT_SIZE}px (width: $FONT_WIDTH, height: $FONT_HEIGHT, weight: $FONT_WEIGHT)" >&2
    echo "  Grid: ${WIDTH}x${HEIGHT} (wrap: $WRAP)" >&2
    echo "  Tab size: $TAB_SIZE" >&2

    if [[ -n "${MULTI_ARGS[border]:-}" ]]; then
        echo "  Border: ${MULTI_ARGS[border]}" >&2
    fi

    declare -a INPUT_LINES
    declare -a LINE_SEGMENTS
    read_input
    calculate_dimensions
    output_svg

    echo "Version 0.026 complete! 🎯" >&2
}

main "$@"
