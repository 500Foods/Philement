# Tables Themes Library Documentation

## Overview

The Tables Themes Library (`lib/tables_themes.sh`) provides a comprehensive theme system for the tables.sh framework. This library manages visual appearance with customizable themes that define colors and ASCII characters for all table elements.

## Purpose

- Define visual themes for table styling
- Manage color schemes for different table elements
- Provide Unicode box-drawing characters for borders
- Support theme switching and customization
- Ensure consistent visual appearance across all tables

## Key Features

- **Multiple Themes** - Pre-defined Red and Blue themes
- **Color Management** - ANSI color codes for different elements
- **Unicode Characters** - Box-drawing characters for professional appearance
- **Theme Switching** - Dynamic theme changes during runtime
- **Extensible Design** - Easy to add new themes

## Available Themes

### Red Theme

The Red theme uses red borders with distinct colors for different table elements.

**Color Scheme:**

- **Border Color** - Red (`\033[0;31m`)
- **Caption Color** - Green (`\033[0;32m`) - Column headers
- **Header Color** - Bright White (`\033[1;37m`) - Table title
- **Footer Color** - Cyan (`\033[0;36m`) - Table footer
- **Summary Color** - Bright White (`\033[1;37m`) - Summary row
- **Text Color** - Default (`\033[0m`) - Regular content

### Blue Theme

The Blue theme uses blue borders with the same color scheme for other elements.

**Color Scheme:**

- **Border Color** - Blue (`\033[0;34m`)
- **Caption Color** - Blue (`\033[0;34m`) - Column headers
- **Header Color** - Bright White (`\033[1;37m`) - Table title
- **Footer Color** - Cyan (`\033[0;36m`) - Table footer
- **Summary Color** - Bright White (`\033[1;37m`) - Summary row
- **Text Color** - Default (`\033[0m`) - Regular content

## Theme Structure

Each theme is defined as an associative array with the following elements:

### Color Elements

- `border_color` - Color for all border elements
- `caption_color` - Color for column headers
- `header_color` - Color for table title text
- `footer_color` - Color for table footer text
- `summary_color` - Color for summary row values
- `text_color` - Default color for regular content

### Border Characters

- `tl_corner` - Top-left corner (`╭`)
- `tr_corner` - Top-right corner (`╮`)
- `bl_corner` - Bottom-left corner (`╰`)
- `br_corner` - Bottom-right corner (`╯`)
- `h_line` - Horizontal line (`─`)
- `v_line` - Vertical line (`│`)
- `t_junct` - Top junction (`┬`)
- `b_junct` - Bottom junction (`┴`)
- `l_junct` - Left junction (`├`)
- `r_junct` - Right junction (`┤`)
- `cross` - Cross junction (`┼`)

## Global Variables

### Theme Arrays

```bash
# Pre-defined theme definitions
declare -A RED_THEME=(...)
declare -A BLUE_THEME=(...)

# Active theme (current theme in use)
declare -A THEME
```

### Default Initialization

The `THEME` array is initialized with the Red theme by default:

```bash
# Initialize with RED_THEME
for key in "${!RED_THEME[@]}"; do
    THEME[$key]="${RED_THEME[$key]}"
done
```

## Functions

### get_theme

Updates the active theme based on the theme name.

**Parameters:**

- `theme_name` - Name of the theme to activate ("Red" or "Blue")

**Side Effects:**

- Clears the existing `THEME` array
- Populates `THEME` with the selected theme
- Prints warning for unknown themes

**Features:**

- Case-insensitive theme names
- Falls back to Red theme for unknown names
- Provides user feedback for invalid themes

**Example:**

```bash
# Switch to Blue theme
get_theme "Blue"

# Switch to Red theme (case insensitive)
get_theme "red"

# Invalid theme (falls back to Red with warning)
get_theme "Green"
```

## Theme Usage

### Accessing Theme Elements

```bash
# Use border color
echo -e "${THEME[border_color]}Border text${THEME[text_color]}"

# Use specific border characters
printf "${THEME[border_color]}%s${THEME[text_color]}" "${THEME[tl_corner]}"

# Use header color
printf "${THEME[header_color]}Table Title${THEME[text_color]}"
```

### Color Reset

Always reset to text color after using themed colors:

```bash
printf "${THEME[border_color]}Colored text${THEME[text_color]}"
```

## Unicode Box-Drawing Characters

The themes use Unicode box-drawing characters for professional appearance:

```table
╭─────┬─────╮
│ A   │ B   │
├─────┼─────┤
│ 1   │ 2   │
╰─────┴─────╯
```

### Character Set

- **Corners**: `╭` `╮` `╰` `╯`
- **Lines**: `─` `│`
- **Junctions**: `┬` `┴` `├` `┤` `┼`

## ANSI Color Codes

The themes use standard ANSI escape sequences:

### Color Codes Used

- `\033[0;31m` - Red
- `\033[0;32m` - Green
- `\033[0;34m` - Blue
- `\033[0;36m` - Cyan
- `\033[1;37m` - Bright White
- `\033[0m` - Reset to default

### Color Attributes

- `0` - Normal intensity
- `1` - Bold/bright
- `31` - Red foreground
- `32` - Green foreground
- `34` - Blue foreground
- `36` - Cyan foreground
- `37` - White foreground

## Theme Integration

The theme system integrates with all rendering components:

### Border Rendering

```bash
printf "${THEME[border_color]}%s${THEME[text_color]}" "${THEME[tl_corner]}"
```

### Header Rendering

```bash
printf "${THEME[caption_color]}%s${THEME[text_color]}" "$header_text"
```

### Data Rendering

```bash
printf "${THEME[text_color]}%s" "$data_value"
```

### Summary Rendering

```bash
printf "${THEME[summary_color]}%s${THEME[text_color]}" "$summary_value"
```

## Extending Themes

### Adding New Themes

To add a new theme, define a new associative array:

```bash
declare -A GREEN_THEME=(
    [border_color]='\033[0;32m'  # Green border
    [caption_color]='\033[0;33m' # Yellow captions
    [header_color]='\033[1;37m'  # Bright white header
    [footer_color]='\033[0;36m'  # Cyan footer
    [summary_color]='\033[1;37m' # Bright white summary
    [text_color]='\033[0m'       # Default text
    # ... border characters same as other themes
)
```

### Updating get_theme Function

Add the new theme to the case statement:

```bash
case "${theme_name,,}" in
    red) # ... existing code ... ;;
    blue) # ... existing code ... ;;
    green)
        for key in "${!GREEN_THEME[@]}"; do
            THEME[$key]="${GREEN_THEME[$key]}"
        done
        ;;
    *) # ... default case ... ;;
esac
```

### Custom Color Schemes

Themes can use any ANSI color codes:

```bash
# 256-color support
[border_color]='\033[38;5;196m'  # Bright red (256-color)

# RGB color support (if terminal supports it)
[border_color]='\033[38;2;255;0;0m'  # RGB red
```

## Terminal Compatibility

### Unicode Support

Most modern terminals support Unicode box-drawing characters:

- **Supported**: iTerm2, Terminal.app, GNOME Terminal, Konsole
- **Limited**: Some older terminals may show fallback characters

### Color Support

ANSI colors are widely supported:

- **8-color**: Basic ANSI colors (most terminals)
- **16-color**: Bright variants (most modern terminals)
- **256-color**: Extended palette (most modern terminals)
- **True color**: RGB support (newer terminals)

## Usage Patterns

### Basic Theme Usage

```bash
#!/bin/bash
source "lib/tables_themes.sh"

# Use default theme (Red)
echo -e "${THEME[border_color]}Red border${THEME[text_color]}"

# Switch to Blue theme
get_theme "Blue"
echo -e "${THEME[border_color]}Blue border${THEME[text_color]}"
```

### Dynamic Theme Switching

```bash
# Function to render with different themes
render_with_theme() {
    local theme_name="$1"
    get_theme "$theme_name"
    
    # Render table with current theme
    printf "${THEME[border_color]}%s${THEME[text_color]}\n" "${THEME[tl_corner]}${THEME[h_line]}${THEME[tr_corner]}"
}

# Render with different themes
render_with_theme "Red"
render_with_theme "Blue"
```

### Theme-aware Functions

```bash
# Function that respects current theme
draw_border() {
    local width="$1"
    printf "${THEME[border_color]}"
    printf "%s" "${THEME[tl_corner]}"
    for ((i=0; i<width; i++)); do
        printf "%s" "${THEME[h_line]}"
    done
    printf "%s" "${THEME[tr_corner]}"
    printf "${THEME[text_color]}\n"
}
```

## Performance Considerations

- **Color Codes** - ANSI sequences add minimal overhead
- **String Operations** - Theme switching involves array copying
- **Memory Usage** - Multiple theme definitions use minimal memory
- **Terminal Rendering** - Color rendering is handled by terminal

## Best Practices

1. **Always reset colors** after using themed colors
2. **Test themes** in target terminal environments
3. **Provide fallbacks** for terminals without Unicode support
4. **Use consistent naming** for new theme elements
5. **Document custom themes** for team usage

## Troubleshooting

### Colors Not Displaying

- Check terminal color support
- Verify ANSI escape sequence format
- Test with simple color codes first

### Unicode Characters Not Displaying

- Verify terminal Unicode support
- Check font support for box-drawing characters
- Consider ASCII fallbacks for compatibility

### Theme Not Switching

- Ensure `get_theme()` is called before rendering
- Check theme name spelling and case
- Verify theme array is properly defined

## Dependencies

- **bash** - Associative arrays and string operations
- **Terminal** - ANSI color and Unicode support

## Integration with Tables System

This library provides the visual foundation for the tables.sh system:

1. **tables_config.sh** reads theme names from configuration
2. **tables_render.sh** uses theme colors and characters
3. **tables.sh** coordinates theme initialization
4. All other libraries respect the active theme

## Related Documentation

- [Tables Library](tables.md) - Main tables system documentation
- [Tables Config Library](tables_config.md) - Configuration parsing
- [Tables Data Library](tables_data.md) - Data processing functions
- [Tables Datatypes Library](tables_datatypes.md) - Data type handling
- [Tables Render Library](tables_render.md) - Rendering functions
