# Migration Guide: Makefile to CMake

## Overview

This guide provides step-by-step instructions for migrating from the traditional Makefile-based build system to the new CMake build system for the Hydrogen project.

## Why Migrate?

### Benefits of CMake

- **Cross-platform compatibility**: Works on Linux, macOS, Windows
- **IDE integration**: Native support for VS Code, CLion, Visual Studio
- **Better dependency management**: Automatic library detection and linking
- **Modern tooling**: Integration with package managers, static analyzers
- **Parallel builds**: Improved build performance
- **Standardized**: Industry-standard build system

### Maintained Compatibility

- All original Makefile targets have CMake equivalents
- Same build variants and optimizations
- Identical output executables
- Compatible with existing scripts and workflows

## Prerequisites

### System Requirements

Ensure you have CMake 3.16 or later installed:

```bash
# Check CMake version
cmake --version

# Install CMake (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install cmake

# Install CMake (from source if needed)
wget https://github.com/Kitware/CMake/releases/download/v3.28.0/cmake-3.28.0-linux-x86_64.tar.gz
tar -xzf cmake-3.28.0-linux-x86_64.tar.gz
sudo cp -r cmake-3.28.0-linux-x86_64/* /usr/local/
```

### Dependencies

The same dependencies are required as with the Makefile:

```bash
# Install build dependencies
sudo apt-get install build-essential pkg-config \
    libjansson-dev libmicrohttpd-dev libwebsockets-dev \
    libbrotli-dev libssl-dev

# Optional for release builds
sudo apt-get install upx-ucl valgrind
```

## Migration Steps

### Step 1: Backup Current Setup

```bash
# Navigate to hydrogen directory
cd elements/001-hydrogen/hydrogen

# Backup current build artifacts (optional)
tar -czf backup-makefile-build-$(date +%Y%m%d).tar.gz \
    hydrogen* build* *.map 2>/dev/null || true

# Clean existing build artifacts
make clean 2>/dev/null || true
```

### Step 2: Verify CMake Configuration

```bash
# Test CMake configuration
cmake -B build-test

# Check for any configuration issues
echo "CMake configuration status: $?"

# Clean test build
rm -rf build-test
```

### Step 3: Create Build Directory

```bash
# Create dedicated build directory
mkdir -p build
cd build

# Or use out-of-source build
cmake -B build
```

### Step 4: Configure Build

```bash
# Basic configuration
cmake -B build

# Or with specific options
cmake -B build \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

### Step 5: Test Basic Build

```bash
# Build default target
cmake --build build --target hydrogen

# Verify executable
./build/hydrogen --version

# Compare with Makefile build
make clean && make
./hydrogen --version
```

## Command Migration Reference

### Basic Commands

| Makefile Command | CMake Equivalent | Notes |
|------------------|------------------|-------|
| `make` | `cmake --build build --target hydrogen` | Default build |
| `make clean` | `cmake --build build --target clean_all` | Complete cleanup |
| `make cleanish` | `cmake --build build --target cleanish` | Preserve release files |

### Build Variants

| Makefile Command | CMake Equivalent | Description |
|------------------|------------------|-------------|
| `make debug` | `cmake --build build --target hydrogen_debug` | AddressSanitizer build |
| `make valgrind` | `cmake --build build --target hydrogen_valgrind` | Valgrind-compatible |
| `make perf` | `cmake --build build --target hydrogen_perf` | Performance optimized |
| `make release` | `cmake --build build --target hydrogen_release` | Production build |
| `make all` | `cmake --build build --target all_variants` | All variants |

### Special Targets

| Makefile Command | CMake Equivalent | Description |
|------------------|------------------|-------------|
| `make trial` | `cmake --build build --target trial` | Diagnostic build |
| `make payload` | `cmake --build build --target payload` | Generate payload |
| N/A | `cmake --build build --target release_enhanced` | UPX + payload |
| N/A | `cmake --build build --target help` | Show available targets |

## Workflow Migration

### Development Workflow

#### Old Makefile Workflow

```bash
# Edit source files
vim src/hydrogen.c

# Build and test
make clean
make debug
./hydrogen_debug

# Check for memory issues
make valgrind
valgrind ./hydrogen_valgrind

# Performance testing
make perf
./hydrogen_perf

# Release build
make release
```

#### New CMake Workflow

```bash
# Edit source files
vim src/hydrogen.c

# Build and test
cmake --build build --target clean_all
cmake --build build --target hydrogen_debug
./build/hydrogen_debug

# Check for memory issues
cmake --build build --target hydrogen_valgrind
valgrind ./build/hydrogen_valgrind

# Performance testing
cmake --build build --target hydrogen_perf
./build/hydrogen_perf

# Release build
cmake --build build --target hydrogen_release
```

#### Optimized CMake Workflow (Recommended)

```bash
# Edit source files
vim src/hydrogen.c

# Incremental builds (faster)
cmake --build build --target hydrogen_debug
./build/hydrogen_debug

# Use presets for common configurations
cmake --preset debug
cmake --build --preset debug
```

### CI/CD Migration

#### Old Makefile CI Script

```bash
#!/bin/bash
set -e

# Build all variants
make clean
make all

# Run tests
make trial
./tests/test_00_all.sh

# Create release
make release
```

#### New CMake CI Script

```bash
#!/bin/bash
set -e

# Configure build
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build all variants
cmake --build build --target all_variants

# Run tests
cmake --build build --target trial
./tests/test_00_all.sh

# Create release
cmake --build build --target release_enhanced
```

## Advanced Migration Topics

### IDE Integration

#### VS Code Setup

Create `.vscode/settings.json`:

```json
{
    "cmake.buildDirectory": "${workspaceFolder}/build",
    "cmake.configureOnOpen": true,
    "C_Cpp.default.compileCommands": "${workspaceFolder}/build/compile_commands.json"
}
```

#### CLion Setup

CLion automatically detects CMakeLists.txt and configures the project.

### Custom Build Scripts

#### Migrating Custom Scripts

If you have custom build scripts using the Makefile:

```bash
# Old script
#!/bin/bash
make clean
make debug
./hydrogen_debug --config test.json

# New script
#!/bin/bash
cmake --build build --target clean_all
cmake --build build --target hydrogen_debug
./build/hydrogen_debug --config test.json
```

### Environment Variables

#### Makefile Environment Variables

```bash
export CC=clang
export CFLAGS="-O3 -march=native"
make
```

#### CMake Environment Variables

```bash
export CMAKE_C_COMPILER=clang
cmake -B build -DCMAKE_C_FLAGS="-O3 -march=native"
cmake --build build
```

## Verification and Testing

### Functional Verification

```bash
# Build both versions
make clean && make
cmake --build build --target clean_all
cmake --build build --target hydrogen

# Compare executables
ls -la hydrogen build/hydrogen
file hydrogen build/hydrogen

# Test functionality
./hydrogen --version
./build/hydrogen --version

# Compare outputs
./hydrogen --help > makefile_help.txt
./build/hydrogen --help > cmake_help.txt
diff makefile_help.txt cmake_help.txt
```

### Performance Verification

```bash
# Build performance variants
make perf
cmake --build build --target hydrogen_perf

# Basic performance test
time ./hydrogen_perf --benchmark
time ./build/hydrogen_perf --benchmark

# Memory usage comparison
valgrind --tool=massif ./hydrogen_perf
valgrind --tool=massif ./build/hydrogen_perf
```

### Build System Verification

```bash
# Compare build artifacts
make all 2>&1 | tee makefile_build.log
cmake --build build --target all_variants 2>&1 | tee cmake_build.log

# Verify all targets exist
ls -la hydrogen*
ls -la build/hydrogen*

# Check map files
ls -la *.map
ls -la build/*.map
```

## Troubleshooting Migration Issues

### Common Problems

#### 1. Missing Dependencies

**Problem**: CMake can't find required libraries

```text
Could NOT find PkgConfig (missing: PKG_CONFIG_EXECUTABLE)
```

**Solution**:

```bash
sudo apt-get install pkg-config
cmake -B build --fresh
```

#### 2. Different Compiler Behavior

**Problem**: CMake uses different compiler flags

**Solution**: Check and adjust compiler settings

```bash
# View current compiler flags
cmake -B build -DCMAKE_VERBOSE_MAKEFILE=ON
cmake --build build --verbose

# Compare with Makefile
make VERBOSE=1
```

#### 3. Path Issues

**Problem**: Executables not found in expected location

**Solution**: Update scripts to use `build/` prefix

```bash
# Old path
./hydrogen

# New path
./build/hydrogen
```

#### 4. Build Directory Confusion

**Problem**: Mixing in-source and out-of-source builds

**Solution**: Always use consistent build directory

```bash
# Clean everything
rm -rf build CMakeCache.txt CMakeFiles/
cmake -B build
```

### Debug Migration Issues

#### Enable Verbose Output

```bash
# CMake configuration debug
cmake -B build --debug-output

# Build process debug
cmake --build build --verbose

# Show all CMake variables
cmake -B build -LAH
```

#### Compare Build Processes

```bash
# Trace Makefile execution
make -n > makefile_trace.txt

# Trace CMake execution
cmake --build build --verbose > cmake_trace.txt

# Compare processes
diff makefile_trace.txt cmake_trace.txt
```

## Rollback Plan

If you need to rollback to the Makefile system:

### Quick Rollback

```bash
# Remove CMake artifacts
rm -rf build/ CMakeCache.txt CMakeFiles/

# Return to Makefile
make clean
make
```

### Preserve CMake Setup

```bash
# Rename CMake files
mv CMakeLists.txt CMakeLists.txt.backup
mv cmake/ cmake.backup/

# Continue with Makefile
make clean
make
```

## Post-Migration Checklist

### ✅ Verification Steps

- [ ] All build variants compile successfully
- [ ] Executables produce identical output
- [ ] Performance characteristics are maintained
- [ ] All tests pass
- [ ] CI/CD pipeline updated
- [ ] Documentation updated
- [ ] Team members trained on new commands

### ✅ Cleanup Steps

- [ ] Remove old build artifacts
- [ ] Update build scripts
- [ ] Update documentation
- [ ] Archive Makefile (optional)

### ✅ Optimization Steps

- [ ] Set up CMake presets
- [ ] Configure IDE integration
- [ ] Enable compilation database
- [ ] Set up ccache for faster builds

## Getting Help

### Resources

- [CMake Documentation](https://cmake.org/documentation/)
- [CMake Tutorial](https://cmake.org/cmake/help/latest/guide/tutorial/)
- [Modern CMake](https://cliutils.gitlab.io/modern-cmake/)

### Project-Specific Help

- Check `CMAKE_BUILD.md` for detailed build system documentation
- Run `cmake --build build --target help` for available targets
- Use `cmake --build build --verbose` for detailed build output

### Support

If you encounter issues during migration:

1. Check this guide's troubleshooting section
2. Compare with working Makefile build
3. Enable verbose output for debugging
4. Consult CMake documentation for specific errors
