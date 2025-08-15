# Oh - Terminal to SVG Converter

**Oh** is a powerful terminal-to-SVG converter available in both **bash** (Oh.sh) and **C** (Oh.c) implementations. Convert terminal output with ANSI escape sequences and UTF-8 content into GitHub-compatible SVG images with selectable text. Transform your colorful terminal sessions into beautiful, shareable SVG graphics that preserve all formatting, colors, and text selectability.

## 🔥 Dual Implementation

- **Oh.sh** - Pure bash implementation for maximum portability
- **Oh.c** - High-performance C implementation with jansson JSON library
- **Shared Cache** - Both versions use identical caching for seamless interoperability
- **Identical Output** - Both produce functionally identical SVG results

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

### Bash Version (Oh.sh)

```bash
# Basic usage - pipe any command output
ls --color=always -la | ./Oh.sh > output.svg

# Git diff with custom styling
git diff --color | ./Oh.sh --font "JetBrains Mono" --font-size 16 -o diff.svg

# Terminal session with specific dimensions
./Oh.sh --font Inconsolata --width 120 --wrap -i terminal.txt -o session.svg
```

### C Version (Oh)

```bash
# Build the C version
make

# Same usage as bash version, but faster
ls --color=always -la | ./Oh > output.svg
git diff --color | ./Oh --font "JetBrains Mono" --font-size 16 -o diff.svg
./Oh --font Inconsolata --width 120 --wrap -i terminal.txt -o session.svg
```

## 📖 Usage

### Bash Version

```bash
# From stdin
command | ./Oh.sh [OPTIONS] > output.svg

# From file
./Oh.sh [OPTIONS] -i input.txt -o output.svg
```

### C Version

```bash
# From stdin
command | ./Oh [OPTIONS] > output.svg

# From file
./Oh [OPTIONS] -i input.txt -o output.svg
```

## 🔨 Building & Installation

### C Version Prerequisites

- **GCC** compiler with C99 support
- **jansson** library for JSON handling
- **pkg-config** for build configuration

### Build C Version

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install libjansson-dev

# Install dependencies (macOS with Homebrew)
brew install jansson

# Build
make

# Install to system (optional)
make install

# Clean build artifacts
make clean
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
# Bash version
ls --color=always -la | ./Oh.sh --font "Fira Code" --font-size 12 > listing.svg

# C version (faster for large outputs)
ls --color=always -la | ./Oh --font "Fira Code" --font-size 12 > listing.svg
```

### Git Status with Custom Styling

```bash
# Bash version
git status --porcelain --color | ./Oh.sh --font "JetBrains Mono" --width 100 > status.svg

# C version
git status --porcelain --color | ./Oh --font "JetBrains Mono" --width 100 > status.svg
```

### System Information Dashboard

```bash
# Both versions support the same syntax
(echo "=== System Info ===" && uname -a && echo && df -h) | \
./Oh.sh --font Inconsolata --font-size 14 --wrap > sysinfo.svg
```

### Build Output Capture

```bash
# C version excels at processing large build logs
npm run build 2>&1 | ./Oh --font "Source Code Pro" --width 120 --height 50 > build.svg
```

### Performance Comparison

```bash
# Large file processing - C version shows significant speed advantage
time ./Oh.sh -i large-terminal-output.txt -o bash-result.svg
time ./Oh -i large-terminal-output.txt -o c-result.svg

# Both share the same cache for subsequent runs
./Oh.sh -i large-terminal-output.txt -o test1.svg  # Uses cache from C version
./Oh -i large-terminal-output.txt -o test2.svg     # Uses cache from bash version
```

## 🔧 Technical Details

### Dependencies

**Bash Version (Oh.sh):**

- **Bash** (4.0+)
- **bc** (for floating-point calculations)
- Standard Unix tools: `sed`, `awk`, `grep`, `cat`, `fold`

**C Version (Oh):**

- **GCC** compiler with C99 support
- **jansson** library for JSON handling
- **pkg-config** for build configuration
- Standard math library (`libm`)

### ⚡ Intelligent Caching System

Both implementations feature a sophisticated caching system for maximum performance:

#### Cache Types

- **Line Cache** - Parsed ANSI segments stored as JSON for instant reuse
- **SVG Fragment Cache** - Pre-rendered SVG text elements
- **Incremental Cache** - Global state tracking for smart cache invalidation

#### Cache Benefits

- **Dramatic Speed Improvement** - Previously processed content loads instantly
- **Shared Cache** - Oh.sh and Oh.c use identical cache files interchangeably
- **Smart Invalidation** - Automatic cache refresh when input or configuration changes
- **Storage Location** - `~/.cache/Oh/` with organized subdirectories

#### Cache Interoperability

```bash
# Generate cache with bash version
./Oh.sh -i large-file.txt -o output1.svg

# C version automatically uses the same cache
./Oh -i large-file.txt -o output2.svg  # Instant cache hit!

# Cache statistics shown in debug mode
./Oh --debug -i file.txt  # Shows cache hit/miss ratios
```

### Font Support

Both versions automatically detect and optimize character spacing for:

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
# Use JetBrains Mono with custom spacing (both versions)
./Oh.sh --font "JetBrains Mono" --font-size 16 --font-width 9 --font-height 20
./Oh --font "JetBrains Mono" --font-size 16 --font-width 9 --font-height 20
```

### Content Wrapping and Clipping

```bash
# Wrap long lines at 80 characters
./Oh.sh --width 80 --wrap -i long-output.txt
./Oh --width 80 --wrap -i long-output.txt

# Clip to specific dimensions
./Oh.sh --width 100 --height 30 -i large-file.txt
./Oh --width 100 --height 30 -i large-file.txt
```

### Debug Mode & Cache Analysis

```bash
# Enable detailed processing information
./Oh.sh --debug -i test.txt > debug.svg
./Oh --debug -i test.txt > debug.svg

# View cache statistics and performance metrics
./Oh --debug -i large-file.txt  # Shows cache hit ratios and timing

# Clean cache for fresh runs
make clean-cache  # For C version
rm -rf ~/.cache/Oh  # Manual cleanup
```

### Choosing the Right Version

**Use Oh.sh when:**

- Maximum portability is needed
- No compilation dependencies desired
- Running on minimal systems
- Prototyping or one-off tasks

**Use Oh (C version) when:**

- Processing large files regularly
- Performance is critical
- Integrating into build pipelines
- Heavy terminal output processing

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
Last updated at 2025-08-15 06:06:30 UTC
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
SVG                              4              6              4           3624
C                                3            195            112           1107
Bourne Shell                     1             58             71            866
Markdown                         1             98              2            247
Text                             2              0              0            163
C/C++ Header                     1             14             15            121
make                             1             13             13             39
YAML                             2              8             13             37
-------------------------------------------------------------------------------
SUM:                            15            392            230           6204
-------------------------------------------------------------------------------
3 Files were skipped (duplicate, binary, or without source code):
  gitattributes: 1
  gitignore: 1
  license: 1
```
<!--CLOC-END-->

## Sponsor / Donate / Support

If you find this work interesting, helpful, or valuable, or that it has saved you time, money, or both, please consider directly supporting these efforts financially via [GitHub Sponsors](https://github.com/sponsors/500Foods) or donating via [Buy Me a Pizza](https://www.buymeacoffee.com/andrewsimard500). Also, check out these other [GitHub Repositories](https://github.com/500Foods?tab=repositories&q=&sort=stargazers) that may interest you.
