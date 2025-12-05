#!/usr/bin/env bash

# About this Script

# Hydrogen Clean Script
# Performs comprehensive cleaning of build artifacts while preserving release variants

# - Removes build/ and build_unity_tests/ directories completely
# - Removes all hydrogen variants except hydrogen_release
# - Removes example executables  
# - Removes map files and other build artifacts
# - Cleans test directories (results, logs, diagnostics)

# Check for required HYDROGEN_ROOT environment variable
if [[ -z "${HYDROGEN_ROOT:-}" ]]; then
    echo "❌ Error: HYDROGEN_ROOT environment variable is not set"
    echo "Please set HYDROGEN_ROOT to the Hydrogen project's root directory"
    exit 1
fi                         

# Check for required HELIUM_ROOT environment variable
if [[ -z "${HELIUM_ROOT:-}" ]]; then
    echo "❌ Error: HELIUM_ROOT environment variable is not set"
    echo "Please set HELIUM_ROOT to the Helium project's root directory"
    exit 1
fi

# Get the directory where this script is located
HYDROGEN_DIR="${HYDROGEN_ROOT}"

# Change to hydrogen directory
cd "${HYDROGEN_DIR}" || exit 1

echo "=== Hydrogen Clean Script ==="
echo "🛈  Starting comprehensive clean..."

# Remove build directories
echo "🛈  Removing build directories..."
if [[ -d "build" ]]; then
    rm -rf build
    echo "✅ Removed build/ directory"
else
    echo "🛈  build/ directory not found"
fi

if [[ -d "build_unity_tests" ]]; then
    rm -rf build_unity_tests
    echo "✅ Removed build_unity_tests/ directory"
else
    echo "🛈  build_unity_tests/ directory not found"
fi

# Remove hydrogen variants (except release)
echo "🛈  Removing hydrogen variants (preserving release variants)..."
variants_removed=0

for variant in hydrogen hydrogen_debug hydrogen_valgrind hydrogen_perf hydrogen_cleanup hydrogen_naked hydrogen_coverage; do
    if [[ -f "${variant}" ]]; then
        rm -f "${variant}"
        echo "✅ Removed ${variant}"
        ((variants_removed++))
    fi
done

if [[ "${variants_removed}" -eq 0 ]]; then
    echo "🛈  No hydrogen variants found to remove"
else
    echo "✅ Removed ${variants_removed} hydrogen variants"
fi

# Preserve release variants
preserved_count=0
if [[ -f "hydrogen_release" ]]; then
    echo "🛈  Preserved hydrogen_release"
    ((preserved_count++))
fi

if [[ "${preserved_count}" -gt 0 ]]; then
    echo "✅ Preserved ${preserved_count} release variants"
fi

# Remove example executables
echo "🛈  Removing example executables..."
examples_removed=0

if [[ -d "examples/C" ]]; then
    cd examples/C || exit 1
    
    for example in auth_code_flow client_credentials password_flow auth_code_flow_debug client_credentials_debug password_flow_debug; do
        if [[ -f "${example}" ]]; then
            rm -f "${example}"
            echo "✅ Removed examples/C/${example}"
            ((examples_removed++))
        fi
    done
    
    cd "${HYDROGEN_DIR}" || exit 1
fi

if [[ "${examples_removed}" -eq 0 ]]; then
    echo "🛈  No example executables found to remove"
else
    echo "✅ Removed ${examples_removed} example executables"
fi

# Remove map files and other build artifacts
echo "🛈  Removing map files and build artifacts..."
artifacts_removed=0

# Remove map files
if ls -- *.map 1> /dev/null 2>&1; then
    for mapfile in *.map; do
        rm -f "${mapfile}"
        echo "✅ Removed ${mapfile}"
        ((artifacts_removed++))
    done
fi

# Remove any remaining build artifacts in cmake directory
if [[ -d "cmake" ]]; then
    cd cmake || exit 1
    
    # Remove any build artifacts that might be in cmake directory
    if ls -- *.map 1> /dev/null 2>&1; then
        for mapfile in *.map; do
            rm -f "${mapfile}"
            echo "✅ Removed cmake/${mapfile}"
            ((artifacts_removed++))
        done
    fi
    
    if [[ -f "CMakeCache.txt" ]]; then
        rm -f "CMakeCache.txt"
        echo "✅ Removed cmake/CMakeCache.txt"
        ((artifacts_removed++))
    fi
    
    cd "${HYDROGEN_DIR}" || exit 1
fi

if [[ "${artifacts_removed}" -eq 0 ]]; then
    echo "🛈  No additional build artifacts found to remove"
else
    echo "✅ Removed ${artifacts_removed} build artifacts"
fi

# Clean test directories
echo "🛈  Cleaning test directories..."
test_dirs_cleaned=0

for test_dir in tests/results tests/logs tests/diagnostics; do
    if [[ -d "${test_dir}" ]]; then
        rm -rf "${test_dir}"
        echo "✅ Removed ${test_dir}/ directory"
        ((test_dirs_cleaned++))
    fi
done

if [[ "${test_dirs_cleaned}" -eq 0 ]]; then
    echo "🛈  No test directories found to remove"
else
    echo "✅ Removed ${test_dirs_cleaned} test directories"
fi

echo ""
echo "🛈  Clean operation completed successfully!"
echo "🛈  Preserved release variants: hydrogen_release (if present)"
echo "🛈  To rebuild, use: ./extras/make-all.sh or ./extras/make-trial.sh"
echo ""
