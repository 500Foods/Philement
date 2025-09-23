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

## Quick Start

### Prerequisites

Ensure you have the required dependencies installed:

```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake pkg-config \
    libjansson-dev libmicrohttpd-dev libwebsockets-dev \
    libbrotli-dev libssl-dev

# Optional for release builds
sudo apt-get install upx-ucl
```

### Basic Build

```bash
# Navigate to the hydrogen directory
cd elements/001-hydrogen/hydrogen

# Configure the build
cmake -B build

# Build the default target
cmake --build build --target hydrogen

# Run the executable
./build/hydrogen
```

### Using CMake Presets (Recommended)

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

## Critical Build Commands

Following any C code changes, use these commands for testing and building:

- **Trial Build** (`mkt` equivalent): `cmake --build build --target trial` - Performs diagnostic compilation showing only warnings/errors, detects unused source files, and runs shutdown tests
- **Build All Variants** (`mka` equivalent): `cmake --build build --target all_variants` - Builds all targets (default, debug, valgrind, perf, coverage, release) and generates payload
- **Build and Run Unit Test** (`mku <test>` equivalent): After `trial`, use `ctest --test-dir build -R <test_name>` to run specific Unity unit tests

## Build Configuration

### CMake Options

```bash
# Set build type
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Enable verbose output
cmake --build build --verbose

# Parallel builds (use all cores)
cmake --build build --parallel $(nproc)
```

## Key Build Targets

### Primary Executables

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

## Payload System

The payload system packages OpenAPI specifications and other resources. More information can be found in the [Payload Documentatio](../payloads/README.md).

### Payload Generation

```bash
# Generate payload contents
cmake --build build --target payload
```

### Payload Structure

- OpenAPI specifications generated from source
- Compressed with Brotli
- Encrypted for security
- Embedded in release executable

## Advanced Features

### Trial Build

Diagnostic build that shows only warnings/errors and detects build issues:

```bash
cmake --build build --target trial
```

Features: Filters output, detects unused files, runs shutdown tests.

### Build Analysis

Each variant generates a `.map` file for linker analysis:

```bash
less build/hydrogen.map  # View symbol layout
grep "DISCARD" build/hydrogen.map  # Find unused symbols
```

## Build Artifacts

### Generated Files

| File | Description |
|------|-------------|
| `hydrogen` | Default build executable |
| `hydrogen_debug` | Debug build with AddressSanitizer |
| `hydrogen_valgrind` | Valgrind-compatible build |
| `hydrogen_perf` | Performance-optimized build |
| `hydrogen_coverage` | Coverage build with gcov instrumentation |
| `hydrogen_release` | Production release with embedded payload |
| `hydrogen_naked` | UPX-compressed release (without payload) |
| `build/*.map` | Linker map files for each variant |
| `build/CMakeCache.txt` | CMake configuration cache |
| `build/compile_commands.json` | Compilation database |

### Cleaning

```bash
# Remove all build artifacts
cmake --build build --target clean_all

# Clean preserving release executables
cmake --build build --target cleanish

# Standard CMake clean
cmake --build build --target clean
```

## Integration with Development Tools

### IDE Integration

The build system generates `compile_commands.json` for IDE integration:

```bash
# Enable compilation database
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

### Static Analysis

```bash
# Run with scan-build
scan-build cmake --build build

# Use with clang-tidy
clang-tidy --compile-commands=build/compile_commands.json src/*.c
```

### Testing Integration

```bash
# Run all tests
ctest --test-dir build

# Run specific test
ctest --test-dir build -R test_name

# Verbose test output
ctest --test-dir build --verbose
```

## Troubleshooting

### Common Issues

#### Missing Dependencies

```bash
# Check for missing packages
cmake -B build 2>&1 | grep "Could NOT find"
# Install missing dependencies
sudo apt-get install <missing-package>-dev
```

#### Build Failures

```bash
# Clean and rebuild
rm -rf build && cmake -B build && cmake --build build --verbose
```

#### UPX Not Found (for release builds)

```bash
sudo apt-get install upx-ucl
# Or build release without UPX compression
cmake --build build --target hydrogen_release
```

#### Verbose Debugging

```bash
# Enable detailed output
cmake -B build --debug-output
cmake --build build --verbose
# Show all CMake variables
cmake -B build -LAH
```

## Performance Considerations

### Build Performance

- Use parallel builds: `cmake --build build --parallel $(nproc)`
- Enable ccache: `export CMAKE_C_COMPILER_LAUNCHER=ccache`
- Use Ninja generator: `cmake -B build -G Ninja`

### Runtime Performance

- Use `hydrogen_perf` for maximum performance
- Profile with `perf`: `perf record ./build/hydrogen_perf`
- Analyze with `gprof` (compile with `-pg`)

## Appendix: CMakeLists File Organization

The CMake build system is organized into multiple specialized files, each managing distinct aspects of the build process:

- **CMakeLists.txt**: Main configuration file that defines the project, includes all other CMakeLists files, and orchestrates the overall build structure.

- **CMakeLists-version.cmake**: Manages version numbering using git commit counts and release timestamps.

- **CMakeLists-init.cmake**: Handles fundamental CMake setup including package discovery, compiler flags, library dependencies, and source file enumeration.

- **CMakeLists-base.cmake**: Configures common build optimizations, CPU-specific flags, and parallel build settings.

- **CMakeLists-cache.cmake**: Sets up compiler caching with ccache for improved build performance.

- **CMakeLists-ninja.cmake**: Contains Ninja generator-specific configurations and targets.

- **CMakeLists-regular.cmake**: Defines the regular build target with standard optimizations.

- **CMakeLists-debug.cmake**: Defines the debug build target with AddressSanitizer instrumentation.

- **CMakeLists-valgrind.cmake**: Defines the Valgrind-compatible build target optimized for memory analysis.

- **CMakeLists-perf.cmake**: Defines the performance build target with aggressive optimizations.

- **CMakeLists-coverage.cmake**: Defines the coverage build target with gcov instrumentation for unit testing.

- **CMakeLists-release.cmake**: Defines the release build target with compression and payload embedding.

- **CMakeLists-examples.cmake**: Manages build targets for OIDC client example programs.

- **CMakeLists-unity.cmake**: Handles Unity unit test framework integration and test executable generation.

- **CMakeLists-package.cmake**: Configures packaging and installation targets using CPack.

- **CMakeLists-targets.cmake**: Defines high-level build targets including all_variants, trial, clean operations, and help.

- **CMakeLists-output.cmake**: Provides terminal output formatting and build status display.

This modular organization allows for targeted modifications when specific build aspects require changes.
