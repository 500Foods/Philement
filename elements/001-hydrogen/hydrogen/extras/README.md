# Hydrogen Extras

This folder contains utility scripts and one-off diagnostic tools for the Hydrogen project.

## Build Scripts

### `make-all.sh`

**Alias:** mka  
**Purpose:** Compilation Test Script  
**Description:** Tests that all components compile without errors or warnings by running the compilation test suite.

```bash
./make-all.sh
```

### `make-clean.sh`

**Alias:** mkc  
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

***Alias:** mkt  
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

### `run-unity-test.sh`

**Purpose:** Unity Test Runner by Name
**Description:** Runs Unity tests by name from any directory. This script automatically:

- Finds the test source file in `tests/unity/src/` recursively
- Determines the correct build target and executable path
- Builds the specific test using CMake/Ninja
- Runs the test and displays results with colored output
- Handles errors gracefully with helpful error messages

**Usage:**

```bash
./run-unity-test.sh <test_name>
```

**Examples:**

```bash
# Run a specific test by name
./run-unity-test.sh beryllium_test_analyze_gcode

# Run the main hydrogen test
./run-unity-test.sh hydrogen_test

# Run an API utility test
./run-unity-test.sh api_utils_test_create_jwt

# Show usage information
./run-unity-test.sh
```

**Features:**

- ✅ Works from any directory (no need to cd to project root)
- ✅ Automatically finds test files in subdirectories
- ✅ Builds only the specific test needed (fast)
- ✅ Colored output for better readability
- ✅ Shows available tests when test name is not found
- ✅ Provides clear error messages and build guidance
- ✅ Returns appropriate exit codes for CI/CD integration

**Example Output:**

```bash
[INFO] Project root: /path/to/hydrogen
[INFO] Looking for test: beryllium_test_analyze_gcode
[INFO] Searching for test source file...
[SUCCESS] Found test source: /path/to/tests/unity/src/print/beryllium_test_analyze_gcode.c
[INFO] Expected executable: /path/to/build/unity/src/print/beryllium_test_analyze_gcode
[INFO] Building test: beryllium_test_analyze_gcode
[INFO] Build command: ninja -C /path/to/build beryllium_test_analyze_gcode
ninja: Entering directory `/path/to/build'
ninja: no work to do.
[SUCCESS] Test built successfully
[INFO] Running test: beryllium_test_analyze_gcode
Test Output:
test_file.c:147:test_function:PASS
test_file.c:173:another_test:FAIL
...
[SUCCESS] Test beryllium_test_analyze_gcode PASSED
```

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

The `one-offs/` directory contains specialized C programs for debugging and testing payload embedding functionality. These came about during various development debugging sessions where
some particular feature needed to be tested in a standalone fashion. The code tends to be
either non-trivial or non-obvious, so these were kept for future posterity.

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

Of special note, when using the release build, it is compressed with UPX and stripped of symbols. So strings like the payload marker appear exactly once, where one would expect the payload marker to appear. The coverage build, on the other hand, is neither compressed nor stripped of its debug symbols, leading to the revelation that the payload marker appears numerous times in the file, and thus we have to be mindful to examine the *last* payload marker when using it as designed.

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

## Database Extensions

### DB2 Base64 Decode UDF

**Location:** `base64decode_udf_db2/`
**Purpose:** DB2 User-Defined Functions for Base64 decoding
**Description:** Provides chunked Base64 decoding capabilities for DB2 databases, including both C-implemented UDFs and SQL wrappers for large data handling.

See [base64decode_udf_db2/README.md](base64decode_udf_db2/README.md) for detailed documentation.

## Error Handling

All scripts include proper error handling and return appropriate exit codes:

- `0`: Success
- `1`: General error or failure
- Scripts provide colored output for better visibility of results
