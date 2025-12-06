# File Utils Library Documentation

## Overview

The File Utils Library (`lib/file_utils.sh`) provides file and path manipulation functions for test scripts in the Hydrogen test suite. This library was created as part of the migration from `support_utils.sh` to modular, focused scripts.

## Purpose

- Provide safe file and directory operations with error handling
- Standardize path manipulation across test scripts
- Handle JSON configuration file parsing
- Enable consistent file size and validation operations

## Key Features

- **Safe directory operations** with error handling
- **Path normalization** for consistent display
- **JSON configuration parsing** with fallback methods
- **File validation** and size operations
- **Cross-platform compatibility**

## Functions

### Path Manipulation

#### `convert_to_relative_path(absolute_path)`

Converts absolute paths to paths relative to the hydrogen project root.

**Parameters:**

- `absolute_path` - The absolute path to convert

**Returns:**

- Relative path starting with "hydrogen/" or original path if conversion fails

**Example:**

```bash
relative_path=$(convert_to_relative_path "/home/user/projects/elements/001-hydrogen/hydrogen/tests/results/test.log")
# Returns: "hydrogen/tests/results/test.log"
```

### Directory Operations

#### `safe_cd(target_dir)`

Safely changes to a directory with proper error handling and reporting.

**Parameters:**

- `target_dir` - The directory to change to

**Returns:**

- 0 on success, 1 on failure

**Example:**

```bash
if safe_cd "/path/to/directory"; then
    echo "Successfully changed directory"
else
    echo "Failed to change directory"
fi
```

### File Operations

#### `get_file_size(file_path)`

Gets the size of a file in bytes with error handling.

**Parameters:**

- `file_path` - Path to the file

**Returns:**

- File size in bytes, or "0" if file doesn't exist or error occurs

**Example:**

```bash
size=$(get_file_size "myfile.txt")
echo "File size: $size bytes"
```

### Configuration File Utilities

#### `get_config_path(config_file)`

Gets the full path to a configuration file in the configs/ subdirectory.

**Parameters:**

- `config_file` - Name of the configuration file

**Returns:**

- Full path to the configuration file

**Example:**

```bash
config_path=$(get_config_path "hydrogen.json")
```

#### `extract_web_server_port(config_file)`

Extracts the web server port from a JSON configuration file.

**Parameters:**

- `config_file` - Path to the JSON configuration file

**Returns:**

- Port number, or 5000 as default if extraction fails

**Features:**

- Uses `jq` if available for proper JSON parsing
- Falls back to grep/sed if `jq` is not available
- Returns sensible default on parsing failure

**Example:**

```bash
port=$(extract_web_server_port "config.json")
echo "Server will run on port: $port"
```

#### `get_webserver_port(config)`

Alternative function to get web server port with different default.

**Parameters:**

- `config` - Path to the JSON configuration file

**Returns:**

- Port number, or 8080 as default if extraction fails

**Example:**

```bash
port=$(get_webserver_port "hydrogen.json")
```

### JSON Validation

#### `validate_json(file)`

Validates JSON syntax in a file with proper error reporting.

**Parameters:**

- `file` - Path to the JSON file to validate

**Returns:**

- 0 if valid JSON, 1 if invalid

**Features:**

- Uses `jq` for proper validation if available
- Falls back to basic structure check if `jq` not available
- Integrates with log_output.sh for consistent error reporting

**Example:**

```bash
if validate_json "config.json"; then
    echo "Configuration file is valid"
else
    echo "Configuration file has JSON errors"
fi
```

### Utility Functions

#### `print_file_utils_version()`

Displays the library version information.

**Example:**

```bash
print_file_utils_version
```

## Usage

### Basic Usage

```bash
#!/bin/bash
# Source the required libraries
source "lib/log_output.sh"    # Required for error reporting
source "lib/file_utils.sh"

# Use the functions
if safe_cd "/path/to/work/directory"; then
    file_size=$(get_file_size "important.txt")
    print_info "File size: $file_size bytes"
fi
```

### Configuration File Handling

```bash
#!/bin/bash
source "lib/log_output.sh"
source "lib/file_utils.sh"

# Get configuration file path
config_file=$(get_config_path "hydrogen.json")

# Validate the configuration
if validate_json "$config_file"; then
    # Extract port for server startup
    port=$(extract_web_server_port "$config_file")
    print_info "Using port: $port"
else
    print_error "Invalid configuration file"
    exit 1
fi
```

### Path Normalization

```bash
#!/bin/bash
source "lib/log_output.sh"
source "lib/file_utils.sh"

# Convert absolute paths for display
log_file="/very/long/path/elements/001-hydrogen/hydrogen/tests/results/test.log"
display_path=$(convert_to_relative_path "$log_file")
print_info "Log file: $display_path"
```

## Dependencies

- **log_output.sh** - For consistent error reporting (optional but recommended)
- **jq** - For proper JSON parsing (optional, has fallback)
- **stat** - For file size operations (standard on most systems)
- **Bash 4.0+** - Uses modern bash features

## Error Handling

The library provides robust error handling:

- **Safe operations** - Functions check for errors and return appropriate codes
- **Graceful fallbacks** - JSON parsing falls back to basic methods if `jq` unavailable
- **Consistent reporting** - Integrates with log_output.sh when available
- **Default values** - Configuration parsing returns sensible defaults on failure

## Migration Notes

This library replaces the following functions from `support_utils.sh`:

- `convert_to_relative_path()` - Path normalization
- `get_config_path()` - Configuration file path handling
- `extract_web_server_port()`, `get_webserver_port()` - Port extraction
- `validate_json()` - JSON validation

Additional functions were added from test_10_compilation.sh:

- `safe_cd()` - Safe directory changing
- `get_file_size()` - File size operations

## Version History

- **1.0.0** (2025-07-02) - Initial creation from support_utils.sh migration

## Related Documentation

- [Log Output Library](log_output.md) - Output formatting and logging
- [Test Framework Library](framework.md) - Test lifecycle management
- [LIBRARIES.md](LIBRARIES.md) - Library index
