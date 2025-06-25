# CMake Build System Documentation

## Overview

The Hydrogen project uses a comprehensive CMake build system that provides multiple build variants optimized for different use cases. This system mirrors the functionality of the original Makefile while leveraging CMake's modern features and cross-platform capabilities.

## Build Variants

### Available Targets

| Target | Description | Optimization | Use Case |
|--------|-------------|--------------|----------|
| `hydrogen` | Default build | `-O2 -g` | Development and general use |
| `hydrogen_debug` | Debug build | AddressSanitizer | Memory debugging |
| `hydrogen_valgrind` | Valgrind-compatible | `-O0` | Memory analysis |
| `hydrogen_perf` | Performance build | `-O3 -march=native` | Maximum performance |
| `hydrogen_release` | Release build | `-s -DNDEBUG -flto` | Production deployment |

### Special Targets

| Target | Description |
|--------|-------------|
| `payload` | Generates OpenAPI specs and payload contents |
| `all_variants` | Builds all variants and generates payload |
| `trial` | Runs diagnostic build with warnings/errors only |
| `release_enhanced` | Creates UPX-compressed release with embedded payload |
| `clean_all` | Removes all build artifacts |
| `cleanish` | Cleans while preserving release executables |
| `help` | Displays available targets and usage |

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

## Build Configuration

### CMake Options

The build system supports several configuration options:

```bash
# Set build type
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Enable verbose output
cmake --build build --verbose

# Parallel builds
cmake --build build --parallel 4
```

### Environment Variables

| Variable | Description | Default |
|----------|-------------|---------|
| `CMAKE_BUILD_TYPE` | Build configuration | `RelWithDebInfo` |
| `CMAKE_C_COMPILER` | C compiler to use | `gcc` |
| `CMAKE_INSTALL_PREFIX` | Installation directory | `/usr/local` |

## Detailed Target Information

### Default Build (`hydrogen`)

- **Optimization**: `-O2` with debug symbols
- **Purpose**: Balanced performance and debuggability
- **Output**: `build/hydrogen`

```bash
cmake --build build --target hydrogen
```

### Debug Build (`hydrogen_debug`)

- **Features**: AddressSanitizer, leak detection
- **Optimization**: Standard with frame pointers
- **Purpose**: Memory error detection
- **Output**: `build/hydrogen_debug`

```bash
cmake --build build --target hydrogen_debug
# Run with ASAN_OPTIONS for detailed output
ASAN_OPTIONS=verbosity=1:halt_on_error=1 ./build/hydrogen_debug
```

### Valgrind Build (`hydrogen_valgrind`)

- **Optimization**: `-O0` for accurate analysis
- **Purpose**: Memory profiling with Valgrind
- **Output**: `build/hydrogen_valgrind`

```bash
cmake --build build --target hydrogen_valgrind
valgrind --tool=memcheck --leak-check=full ./build/hydrogen_valgrind
```

### Performance Build (`hydrogen_perf`)

- **Optimization**: `-O3 -march=native -ffast-math`
- **Features**: LTO, aggressive inlining
- **Purpose**: Maximum runtime performance
- **Output**: `build/hydrogen_perf`

```bash
cmake --build build --target hydrogen_perf
```

### Release Build (`hydrogen_release`)

- **Optimization**: `-s -DNDEBUG -flto`
- **Features**: Stripped, LTO, stack protection
- **Purpose**: Production deployment
- **Output**: `build/hydrogen_release`

```bash
cmake --build build --target hydrogen_release
```

### Enhanced Release (`release_enhanced`)

- **Features**: UPX compression, embedded payload
- **Dependencies**: Requires `payload` target
- **Output**: `build/hydrogen_release` (with payload), `build/hydrogen_naked` (compressed only)

```bash
cmake --build build --target release_enhanced
```

## Payload System

The payload system packages OpenAPI specifications and other resources:

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

Performs diagnostic compilation showing only warnings and errors:

```bash
cmake --build build --target trial
```

Features:

- Filters compilation output
- Detects unused source files
- Runs shutdown tests
- Provides build diagnostics

### Map File Analysis

Each build variant generates a map file for analysis:

```bash
# View symbol layout
less build/hydrogen.map

# Analyze unused symbols
grep "DISCARD" build/hydrogen.map
```

### Unused File Detection

The build system can detect source files not linked into the final executable:

```bash
# Files listed in .trial-ignore are expected to be unused
cat .trial-ignore

# Trial build reports unexpected unused files
cmake --build build --target trial
```

## Build Artifacts

### Generated Files

| File | Description |
|------|-------------|
| `build/hydrogen*` | Executable variants |
| `build/*.map` | Linker map files |
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
rm -rf build
cmake -B build
cmake --build build --verbose
```

#### UPX Not Found

```bash
# Install UPX for release builds
sudo apt-get install upx-ucl

# Or skip UPX compression (warning will be shown)
cmake --build build --target hydrogen_release
```

### Debug Information

#### Verbose Output

```bash
# CMake configuration
cmake -B build --debug-output

# Build process
cmake --build build --verbose

# Show all CMake variables
cmake -B build -LAH
```

#### Environment Debugging

```bash
# Show CMake variables
cmake -B build -LAH

# Show compiler information
cmake -B build --system-information
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

## Migration from Makefile

### Command Equivalents

| Makefile | CMake |
|----------|-------|
| `make` | `cmake --build build --target hydrogen` |
| `make debug` | `cmake --build build --target hydrogen_debug` |
| `make valgrind` | `cmake --build build --target hydrogen_valgrind` |
| `make perf` | `cmake --build build --target hydrogen_perf` |
| `make release` | `cmake --build build --target hydrogen_release` |
| `make all` | `cmake --build build --target all_variants` |
| `make trial` | `cmake --build build --target trial` |
| `make payload` | `cmake --build build --target payload` |
| `make clean` | `cmake --build build --target clean_all` |
| `make cleanish` | `cmake --build build --target cleanish` |

### Key Differences

- CMake requires explicit build directory
- Configuration and build are separate steps
- Better dependency tracking
- Cross-platform compatibility
- IDE integration support

## Best Practices

### Development Workflow

1. Use `hydrogen_debug` for development
2. Run `trial` target regularly
3. Test with `hydrogen_valgrind` for memory issues
4. Use `hydrogen_perf` for performance testing
5. Create `hydrogen_release` for deployment

### Continuous Integration

```bash
# CI build script
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target all_variants
cmake --build build --target trial
ctest --test-dir build
```

### Release Process

```bash
# Complete release build
cmake --build build --target payload
cmake --build build --target release_enhanced
# Verify release executable
./build/hydrogen_release --version
```
