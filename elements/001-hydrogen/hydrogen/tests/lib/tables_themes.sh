#!/usr/bin/env bash

# tables_themes.sh: Theme system for tables.sh
# Manages visual appearance with customizable themes

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
