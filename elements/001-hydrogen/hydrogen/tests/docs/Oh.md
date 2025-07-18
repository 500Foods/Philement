# Oh.sh - Terminal to SVG Converter

**Oh.sh** is a powerful bash script that converts terminal output with ANSI escape sequences and UTF-8 content into GitHub-compatible SVG images with selectable text. Transform your colorful terminal sessions into beautiful, shareable SVG graphics that preserve all formatting, colors, and text selectability.

## ✨ Features

### 🎨 Full ANSI Support

- Complete ANSI color parsing (standard 16 colors + bright variants)
- Bold/weight styling preservation
- Background color support
- Proper escape sequence handling

### 🔤 Advanced Typography

- **System Fonts**: Consolas, Monaco, Courier New
- **Google Fonts**: Inconsolata, JetBrains Mono, Source Code Pro, Fira Code, Roboto Mono
- Automatic font embedding for web fonts
- Customizable font size, weight, and character spacing
- Precise grid-based layout for perfect alignment

### 📐 Flexible Layout

- Auto-detection of content dimensions
- Configurable grid width and height
- Line wrapping support
- Tab expansion with customizable tab stops
- Content clipping for oversized output

### 🛠️ Professional Output

- Clean, GitHub-compatible SVG format
- Selectable text preservation
- Proper XML escaping and encoding
- Customizable padding and styling
- Dark theme optimized colors

## 🚀 Quick Start

```bash
# Basic usage - pipe any command output
ls --color=always -la | ./oh.sh > output.svg

# Git diff with custom styling
git diff --color | ./oh.sh --font "JetBrains Mono" --font-size 16 -o diff.svg

# Terminal session with specific dimensions
./oh.sh --font Inconsolata --width 120 --wrap -i terminal.txt -o session.svg
```

## 📖 Usage

```bash
# From stdin
command | oh.sh [OPTIONS] > output.svg

# From file
oh.sh [OPTIONS] -i input.txt -o output.svg
```

### Command Line Options

| Option | Description | Default |
|--------|-------------|---------|
| `-i, --input FILE` | Input file | stdin |
| `-o, --output FILE` | Output file | stdout |
| `--font FAMILY` | Font family | Consolas |
| `--font-size SIZE` | Font size (8-72px) | 14 |
| `--font-width PX` | Character width | 0.6 * font-size |
| `--font-height PX` | Line height | 1.2 * font-size |
| `--font-weight WEIGHT` | Font weight (100-900) | 400 |
| `--width CHARS` | Grid width in characters | 80 (auto-detected) |
| `--height CHARS` | Grid height in lines | auto |
| `--wrap` | Wrap lines at width | false |
| `--tab-size SIZE` | Tab stop size (1-16) | 8 |
| `--debug` | Enable debug output | false |

## 💡 Examples

### Colorful Directory Listing

```bash
ls --color=always -la | ./oh.sh --font "Fira Code" --font-size 12 > listing.svg
```

### Git Status with Custom Styling

```bash
git status --porcelain --color | ./oh.sh --font "JetBrains Mono" --width 100 > status.svg
```

### System Information Dashboard

```bash
(echo "=== System Info ===" && uname -a && echo && df -h) | \
./oh.sh --font Inconsolata --font-size 14 --wrap > sysinfo.svg
```

### Build Output Capture

```bash
npm run build 2>&1 | ./oh.sh --font "Source Code Pro" --width 120 --height 50 > build.svg
```

## 🔧 Technical Details

### Dependencies

- **Bash** (4.0+)
- **bc** (for floating-point calculations)
- Standard Unix tools: `sed`, `awk`, `grep`, `cat`, `fold`

### Font Support

Oh.sh automatically detects and optimizes character spacing for:

- **Monospace System Fonts**: Perfect for local development
- **Google Fonts**: Automatically embedded with proper CSS imports
- **Custom Ratios**: Fine-tuned character width ratios per font family

### ANSI Processing

- Handles complex nested escape sequences
- Preserves exact positioning and spacing
- Supports foreground/background colors
- Maintains bold and weight styling
- Robust XML escaping for SVG compatibility

## 🎯 Use Cases

- **Documentation**: Include terminal examples in README files
- **Tutorials**: Create step-by-step terminal guides
- **Bug Reports**: Capture and share terminal output
- **Presentations**: Professional terminal screenshots
- **Code Reviews**: Share command-line tool output
- **Blog Posts**: Embed terminal sessions in articles

## 📚 Advanced Usage

### Custom Font Configuration

```bash
# Use JetBrains Mono with custom spacing
./oh.sh --font "JetBrains Mono" --font-size 16 --font-width 9 --font-height 20
```

### Content Wrapping and Clipping

```bash
# Wrap long lines at 80 characters
./oh.sh --width 80 --wrap -i long-output.txt

# Clip to specific dimensions
./oh.sh --width 100 --height 30 -i large-file.txt
```

### Debug Mode

```bash
# Enable detailed processing information
./oh.sh --debug -i test.txt > debug.svg
```

## 🤝 Contributing

This project is under active development. Feel free to:

- Report issues and bugs
- Suggest new features
- Submit pull requests
- Join discussions about project direction

Your feedback helps shape the future of Oh.sh!

## Repository Information

[![Count Lines of Code](https://github.com/500Foods/Oh.sh/actions/workflows/main.yml/badge.svg)](https://github.com/500Foods/Oh.sh/actions/workflows/main.yml)
<!--CLOC-START -->
```cloc
Last updated at 2025-05-30 19:52:24 UTC
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
Bourne Shell                     2            148             93            860
Text                             3              4              0             69
YAML                             2              8             13             37
Markdown                         1              5              2             25
-------------------------------------------------------------------------------
SUM:                             8            165            108            991
-------------------------------------------------------------------------------
3 Files were skipped (duplicate, binary, or without source code):
  gitattributes: 1
  gitignore: 1
  license: 1
```
<!--CLOC-END-->

## Sponsor / Donate / Support

If you find this work interesting, helpful, or valuable, or that it has saved you time, money, or both, please consider directly supporting these efforts financially via [GitHub Sponsors](https://github.com/sponsors/500Foods) or donating via [Buy Me a Pizza](https://www.buymeacoffee.com/andrewsimard500). Also, check out these other [GitHub Repositories](https://github.com/500Foods?tab=repositories&q=&sort=stargazers) that may interest you.
