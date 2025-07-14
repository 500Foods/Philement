# Hydrogen Extras

This folder contains utility scripts and one-off diagnostic tools for the Hydrogen project.

## Build Scripts

### `make-all.sh`

**Purpose:** Compilation Test Script  
**Description:** Tests that all components compile without errors or warnings by running the compilation test suite.

```bash
./make-all.sh
```

### `make-clean.sh`

**Purpose:** Comprehensive Build Cleanup  
**Description:** Performs thorough cleaning of build artifacts while preserving release variants. This script:

- Removes `build/` and `build_unity_tests/` directories completely
- Removes all hydrogen variants except `hydrogen_release`
- Removes example executables from `examples/C/`
- Removes map files and other build artifacts
- Cleans test directories (results, logs, diagnostics)
- Preserves `hydrogen_release` for production use

**Cleaned variants:** `hydrogen`, `hydrogen_debug`, `hydrogen_valgrind`, `hydrogen_perf`, `hydrogen_cleanup`, `hydrogen_naked`, `hydrogen_coverage`

```bash
./make-clean.sh
```

### `make-trial.sh`

**Purpose:** Quick Trial Build and Diagnostics  
**Description:** Performs a fast build cycle with comprehensive diagnostics including:

- Clean CMake build with error/warning detection
- Automatic binary copying for testing
- Shutdown test execution
- Unused source file detection (respects `.trial-ignore`)
- Build artifact cleanup including `hydrogen_naked` removal

```bash
./make-trial.sh
```

## Utility Scripts

### `filter-log.sh`

**Purpose:** Log Output Filtering  
**Description:** Filters Hydrogen log output using regex pattern matching. Extracts structured log entries in the format:

```log
[nnn nnn] [LEVEL module] MESSAGE
```

**Usage:**

```bash
./hydrogen | ./filter-log.sh
```

## One-Off Diagnostic Tools

The `one-offs/` directory contains specialized C programs for debugging and testing payload embedding functionality.

### `debug_payload.c`

**Purpose:** Payload Debug Analysis  
**Description:** Analyzes binary files to debug payload embedding. Features:

- Locates payload markers in binary files
- Extracts and validates payload size information
- Shows raw hex bytes of size field
- Displays last 50 bytes of file for debugging
- Validates payload size against marker offset

**Compilation:**

```bash
gcc -o debug_payload debug_payload.c
```

**Usage:**

```bash
./debug_payload <binary_file>
```

**Example Output:**

```log
File: hydrogen_release
File size: 2847264 bytes
✅ Marker found at offset: 2847200
Stored payload size: 64 bytes
Validation (payload_size <= marker_offset): ✅ PASS
```

### `find_all_markers.c`

**Purpose:** Multiple Marker Detection  
**Description:** Comprehensive marker search that finds ALL occurrences of payload markers in a binary file. Useful for debugging cases where multiple markers might exist. For each marker found:

- Reports exact byte offset
- Shows raw size field bytes
- Validates payload size logic
- Continues searching for additional markers

**Compilation:**

```bash
gcc -o find_all_markers find_all_markers.c
```

**Usage:**

```bash
./find_all_markers <binary_file>
```

### `test_payload_detection.c`

**Purpose:** Payload Validation Testing  
**Description:** Tests the complete payload detection logic used by Hydrogen. Validates:

- Marker presence and location
- Size field integrity
- Payload bounds checking (≤ 100MB, ≤ marker offset)
- Complete detection workflow

**Compilation:**

```bash
gcc -o test_payload_detection test_payload_detection.c
```

**Usage:**

```bash
./test_payload_detection <executable_path>
```

**Return Codes:**

- `0`: Payload successfully detected and validated
- `1`: No payload found or validation failed

## Payload Marker Format

All payload detection tools use the standard Hydrogen payload format:

```diagram
[BINARY DATA]
[PAYLOAD DATA]
<<< HERE BE ME TREASURE >>>
[8-byte size field in big-endian]
```

The 8-byte size field contains the payload size in bytes, stored in big-endian format. The payload data is located immediately before the marker, starting at `(marker_offset - payload_size)`.

## Integration with Build System

These scripts integrate with the CMake build system:

- `make-trial.sh` corresponds to `cmake --build . --target trial`
- `make-clean.sh` provides functionality beyond `cmake --build . --target clean_all`
- `make-all.sh` provides legacy Makefile compatibility

## Development Workflow

Typical development workflow using these tools:

1. **Development cycle:** Use `make-trial.sh` for quick iteration
2. **Full testing:** Use `make-all.sh` for comprehensive compilation testing
3. **Cleanup:** Use `make-clean.sh` to reset build environment
4. **Payload debugging:** Use one-off tools when investigating payload issues

## Error Handling

All scripts include proper error handling and return appropriate exit codes:

- `0`: Success
- `1`: General error or failure
- Scripts provide colored output for better visibility of results
