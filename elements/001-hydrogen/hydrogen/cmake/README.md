# CMake Build System Documentation

## Overview

Hydrogen is a C project that implements a comprehensive suite of capabilities including REST API with Swagger provided by embedded payload, web services (Web, WebSocket, SMTP, mDNS), and both blackbox integration and Unity unit tests with gcov coverage reporting. The CMake build system provides multiple build variants optimized for different use cases, mirroring the functionality of the original Makefile while leveraging CMake's modern features and cross-platform capabilities.

## Available Targets

### Executable Variants

| Target | Description | Optimization | Use Case |
|--------|-------------|--------------|----------|
| `hydrogen` | Default build | `-O2 -g` | Development and general use |
| `hydrogen_debug` | Debug build | AddressSanitizer | Memory debugging |
| `hydrogen_valgrind` | Valgrind-compatible | `-O0` | Memory analysis |
| `hydrogen_perf` | Performance build | `-O3 -march=native` | Maximum performance |
| `hydrogen_coverage` | Coverage build | gcov instrumentation | Unit testing with coverage |
| `hydrogen_release` | Release build | `-s -DNDEBUG -flto` | Production deployment |

### Build Utilities

| Target | Description |
|--------|-------------|
| `all_variants` | Builds all executable variants and generates payload |
| `trial` | Runs diagnostic build with warnings/errors only |
| `payload` | Generates OpenAPI specs and payload contents |
| `coverage` | Builds coverage executable with embedded payload |
| `release_enhanced` | Creates UPX-compressed release with embedded payload |
| `unity_tests` | Builds Unity unit tests with coverage instrumentation |
| `clean_all` | Removes all build artifacts |
| `cleanish` | Cleans while preserving release executables |

### Examples & Tools

| Target | Description |
|--------|-------------|
| `all_examples` | Builds all OIDC client example programs |
| `auth_code_flow` | Builds Authorization Code flow OIDC example |
| `client_credentials` | Builds Client Credentials flow OIDC example |
| `password_flow` | Builds Resource Owner Password flow OIDC example |
| `configure_ccache` | Configures ccache with optimal settings |
| `ccache_stats` | Shows ccache performance statistics |
| `check_version` | Checks if version number needs updating |
| `cmake_help` | Displays available targets and usage |

## CMake Files

The CMake build system is organized into multiple specialized files, each managing distinct aspects of the build process:

- [**CMakeLists.txt**](CMakeLists.txt) Main configuration file that defines the project, includes all other CMakeLists files, and orchestrates the overall build structure.

- [**CMakeLists-version.cmake**](CMakeLists-version.cmake) Version numbering using git commit counts and release timestamps.

- [**CMakeLists-init.cmake**](CMakeLists-init.cmake) Fundamental CMake setup including package discovery, compiler flags, library dependencies, and source file enumeration.

- [**CMakeLists-base.cmake**](CMakeLists-base.cmake) Common build optimizations, CPU-specific flags, and parallel build settings.

- [**CMakeLists-cache.cmake**](CMakeLists-cache.cmake) Compiler caching with ccache for improved build performance.

- [**CMakeLists-ninja.cmake**](CMakeLists-ninja.cmake) Ninja generator-specific configurations and targets.

- [**CMakeLists-regular.cmake**](CMakeLists-regular.cmake) Regular build target with standard optimizations.

- [**CMakeLists-debug.cmake**](CMakeLists-debug.cmake) Debug build target with AddressSanitizer instrumentation.

- [**CMakeLists-valgrind.cmake**](CMakeLists-valgrind.cmake) Valgrind-compatible build target optimized for memory analysis.

- [**CMakeLists-perf.cmake**](CMakeLists-perf.cmake) Performance build target with aggressive optimizations.

- [**CMakeLists-coverage.cmake**](CMakeLists-coverage.cmake) Coverage build target with gcov instrumentation for unit testing.

- [**CMakeLists-release.cmake**](CMakeLists-release.cmake) Release build target with compression and payload embedding.

- [**CMakeLists-examples.cmake**](CMakeLists-examples.cmake) Build targets for OIDC client example programs.

- [**CMakeLists-unity.cmake**](CMakeLists-unity.cmake) Unity unit test framework integration and test executable generation.

- [**CMakeLists-package.cmake**](CMakeLists-package.cmake) Packaging and installation targets using CPack.

- [**CMakeLists-targets.cmake**](CMakeLists-targets.cmake) High-level build targets including all_variants, trial, clean operations, and help.

- [**CMakeLists-output.cmake**](CMakeLists-output.cmake) Terminal output formatting and build status display.

Note: The CHANGELOG is also maintained in [**CMakeLists.txt**](CMakeLists.txt).

This modular organization allows for targeted modifications when specific build aspects require changes.

## Quick Start

### Prerequisites

Ensure you have the required dependencies installed:

```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake pkg-config \
    libjansson-dev libmicrohttpd-dev libwebsockets-dev \
    libbrotli-dev libssl-dev libuuid-dev liblua-dev upx-ucl
```

### Building Hydrogen

For easier development, the following aliases are available and can be run from any directory:

- `mkt`: Run a trial build (diagnostic compilation showing only warnings/errors)
- `mka`: Build all targets and generate payload
- `mkb`: Build the whole project and run all tests

Here's an example of what a normal trial build looks like.

```bash
➜  hydrogen git:(main) ✗ mkt 
17:32:52.716 - Cleaning Build Directory
17:32:53.203 - Configuring CMake
17:33:01.079 - Dependency Check
17:33:01.093 - Dependency Check Passed
17:33:02.361 - Updating 611 Completions
17:33:02.380 - Default Build
17:33:08.172 - Unity Build
17:33:49.806 - Build Successful
```

### Using CMake Presets

```bash
# List available presets
cmake --list-presets

# Configure with a preset
cmake --preset debug

# Build with a preset
cmake --build --preset debug

# Test with a preset
ctest --preset debug
```

## Build Targets

- **`hydrogen`** (default): Balanced optimization with debug symbols for development
- **`hydrogen_debug`**: AddressSanitizer enabled for memory error detection
- **`hydrogen_valgrind`**: Optimized for Valgrind memory analysis (`-O0`)
- **`hydrogen_perf`**: Maximum performance with aggressive optimizations (`-O3 -march=native`)
- **`hydrogen_coverage`**: gcov instrumentation for unit test coverage reporting
- **`hydrogen_release`**: Production build with stripping and LTO

### Special Operations

- **`trial`**: Diagnostic build showing only warnings/errors and unused file detection
- **`all_variants`**: Builds all executables plus payload generation
- **`release_enhanced`**: Creates UPX-compressed release with embedded payload, producing both `hydrogen_release` (with payload) and `hydrogen_naked` (compressed backup without payload)

### Examples

- **`all_examples`**: Builds all OIDC client example programs
- Individual examples: `auth_code_flow`, `client_credentials`, `password_flow` (with `_debug` variants)

See the target tables above for complete listings and build commands for each target.
