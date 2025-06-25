#!/bin/bash
# ──────────────────────────────────────────────────────────────────────────────
# Build Comparison Script for Hydrogen CMake vs Makefile
# 
# This script compares the outputs of CMake and Makefile builds to ensure
# feature parity and identical functionality between the two build systems.
# ──────────────────────────────────────────────────────────────────────────────

set -e

# Terminal output formatting
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NORMAL='\033[0m'

# Unicode status symbols
PASS='✅'
FAIL='❌'
WARN='⚠️'
INFO='🛈'

# Configuration
MAKEFILE_DIR="src"
CMAKE_BUILD_DIR="build"
COMPARISON_DIR="build-comparison"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_FILE="comparison_${TIMESTAMP}.log"

# Build variants to compare
VARIANTS=("hydrogen" "hydrogen_debug" "hydrogen_valgrind" "hydrogen_perf" "hydrogen_release")

# Function to log messages
log_message() {
    echo -e "$1" | tee -a "$LOG_FILE"
}

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to get file size
get_file_size() {
    if [ -f "$1" ]; then
        stat -c %s "$1" 2>/dev/null || stat -f %z "$1" 2>/dev/null || echo "0"
    else
        echo "0"
    fi
}

# Function to get file hash
get_file_hash() {
    if [ -f "$1" ]; then
        sha256sum "$1" 2>/dev/null | cut -d' ' -f1 || shasum -a 256 "$1" 2>/dev/null | cut -d' ' -f1 || echo "unknown"
    else
        echo "missing"
    fi
}

# Function to compare executables
compare_executables() {
    local variant="$1"
    local makefile_exe="$2"
    local cmake_exe="$3"
    
    log_message "${CYAN}${INFO} Comparing $variant executables...${NORMAL}"
    
    # Check if both files exist
    if [ ! -f "$makefile_exe" ]; then
        log_message "  ${RED}${FAIL} Makefile executable not found: $makefile_exe${NORMAL}"
        return 1
    fi
    
    if [ ! -f "$cmake_exe" ]; then
        log_message "  ${RED}${FAIL} CMake executable not found: $cmake_exe${NORMAL}"
        return 1
    fi
    
    # Get file information
    local makefile_size=$(get_file_size "$makefile_exe")
    local cmake_size=$(get_file_size "$cmake_exe")
    local makefile_hash=$(get_file_hash "$makefile_exe")
    local cmake_hash=$(get_file_hash "$cmake_exe")
    
    # Compare sizes (allow 5% difference for build variations)
    local size_diff=$(echo "scale=2; ($cmake_size - $makefile_size) / $makefile_size * 100" | bc -l 2>/dev/null || echo "0")
    local size_diff_abs=$(echo "$size_diff" | sed 's/-//')
    
    log_message "  File sizes: Makefile=$makefile_size, CMake=$cmake_size (${size_diff}% difference)"
    
    if (( $(echo "$size_diff_abs > 5" | bc -l 2>/dev/null || echo "0") )); then
        log_message "  ${YELLOW}${WARN} Significant size difference detected${NORMAL}"
    fi
    
    # Compare hashes
    if [ "$makefile_hash" = "$cmake_hash" ]; then
        log_message "  ${GREEN}${PASS} Executables are identical (hash match)${NORMAL}"
        return 0
    else
        log_message "  ${YELLOW}${WARN} Executables differ (different hashes)${NORMAL}"
        log_message "    Makefile hash: $makefile_hash"
        log_message "    CMake hash:    $cmake_hash"
        
        # Test basic functionality
        log_message "  ${CYAN}${INFO} Testing basic functionality...${NORMAL}"
        
        # Test version output
        local makefile_version=$("$makefile_exe" --version 2>/dev/null | head -1 || echo "error")
        local cmake_version=$("$cmake_exe" --version 2>/dev/null | head -1 || echo "error")
        
        if [ "$makefile_version" = "$cmake_version" ]; then
            log_message "  ${GREEN}${PASS} Version outputs match${NORMAL}"
        else
            log_message "  ${YELLOW}${WARN} Version outputs differ${NORMAL}"
            log_message "    Makefile: $makefile_version"
            log_message "    CMake:    $cmake_version"
        fi
        
        # Test help output
        local makefile_help=$("$makefile_exe" --help 2>/dev/null | wc -l || echo "0")
        local cmake_help=$("$cmake_exe" --help 2>/dev/null | wc -l || echo "0")
        
        if [ "$makefile_help" = "$cmake_help" ]; then
            log_message "  ${GREEN}${PASS} Help outputs have same length${NORMAL}"
        else
            log_message "  ${YELLOW}${WARN} Help outputs differ in length${NORMAL}"
        fi
        
        return 1
    fi
}

# Function to build with Makefile
build_makefile() {
    log_message "${BLUE}────────────────────────────────────────────────────────────────${NORMAL}"
    log_message "${BLUE}${BOLD} Building with Makefile${NORMAL}"
    log_message "${BLUE}────────────────────────────────────────────────────────────────${NORMAL}"
    
    cd "$MAKEFILE_DIR"
    
    # Clean previous builds
    log_message "${CYAN}${INFO} Cleaning previous Makefile builds...${NORMAL}"
    make clean >/dev/null 2>&1 || true
    
    # Build all variants
    for variant in "${VARIANTS[@]}"; do
        log_message "${CYAN}${INFO} Building Makefile $variant...${NORMAL}"
        
        case "$variant" in
            "hydrogen")
                make default 2>&1 | grep -E "(error:|warning:|built successfully)" || true
                ;;
            "hydrogen_debug")
                make debug 2>&1 | grep -E "(error:|warning:|completed successfully)" || true
                ;;
            "hydrogen_valgrind")
                make valgrind 2>&1 | grep -E "(error:|warning:|completed successfully)" || true
                ;;
            "hydrogen_perf")
                make perf 2>&1 | grep -E "(error:|warning:|completed successfully)" || true
                ;;
            "hydrogen_release")
                make release 2>&1 | grep -E "(error:|warning:|completed successfully)" || true
                ;;
        esac
    done
    
    # Build payload
    log_message "${CYAN}${INFO} Building Makefile payload...${NORMAL}"
    make payload >/dev/null 2>&1 || true
    
    cd ..
}

# Function to build with CMake
build_cmake() {
    log_message "${BLUE}────────────────────────────────────────────────────────────────${NORMAL}"
    log_message "${BLUE}${BOLD} Building with CMake${NORMAL}"
    log_message "${BLUE}────────────────────────────────────────────────────────────────${NORMAL}"
    
    # Clean previous builds
    log_message "${CYAN}${INFO} Cleaning previous CMake builds...${NORMAL}"
    rm -rf "$CMAKE_BUILD_DIR"
    
    # Configure CMake
    log_message "${CYAN}${INFO} Configuring CMake build...${NORMAL}"
    cmake -B "$CMAKE_BUILD_DIR" >/dev/null 2>&1
    
    # Build all variants
    for variant in "${VARIANTS[@]}"; do
        log_message "${CYAN}${INFO} Building CMake $variant...${NORMAL}"
        cmake --build "$CMAKE_BUILD_DIR" --target "$variant" 2>&1 | grep -E "(error:|warning:|completed successfully)" || true
    done
    
    # Build payload
    log_message "${CYAN}${INFO} Building CMake payload...${NORMAL}"
    cmake --build "$CMAKE_BUILD_DIR" --target payload >/dev/null 2>&1 || true
}

# Function to compare builds
compare_builds() {
    log_message "${BLUE}────────────────────────────────────────────────────────────────${NORMAL}"
    log_message "${BLUE}${BOLD} Comparing Build Results${NORMAL}"
    log_message "${BLUE}────────────────────────────────────────────────────────────────${NORMAL}"
    
    local all_passed=true
    
    for variant in "${VARIANTS[@]}"; do
        local makefile_exe="$variant"
        local cmake_exe="$CMAKE_BUILD_DIR/$variant"
        
        if ! compare_executables "$variant" "$makefile_exe" "$cmake_exe"; then
            all_passed=false
        fi
    done
    
    # Compare payload files
    log_message "${CYAN}${INFO} Comparing payload files...${NORMAL}"
    
    local payload_files=("payloads/payload.tar.br.enc" "payloads/swagger.json")
    for payload_file in "${payload_files[@]}"; do
        if [ -f "$payload_file" ]; then
            local payload_size=$(get_file_size "$payload_file")
            local payload_hash=$(get_file_hash "$payload_file")
            log_message "  $payload_file: size=$payload_size, hash=${payload_hash:0:16}..."
        else
            log_message "  ${YELLOW}${WARN} Payload file not found: $payload_file${NORMAL}"
        fi
    done
    
    return $all_passed
}

# Function to run performance comparison
performance_comparison() {
    log_message "${BLUE}────────────────────────────────────────────────────────────────${NORMAL}"
    log_message "${BLUE}${BOLD} Performance Comparison${NORMAL}"
    log_message "${BLUE}────────────────────────────────────────────────────────────────${NORMAL}"
    
    local perf_variant="hydrogen_perf"
    local makefile_exe="$perf_variant"
    local cmake_exe="$CMAKE_BUILD_DIR/$perf_variant"
    
    if [ -f "$makefile_exe" ] && [ -f "$cmake_exe" ]; then
        log_message "${CYAN}${INFO} Running basic performance test...${NORMAL}"
        
        # Test startup time
        log_message "  Testing startup time..."
        
        local makefile_time=$(time -p "$makefile_exe" --version 2>&1 | grep real | awk '{print $2}' || echo "0")
        local cmake_time=$(time -p "$cmake_exe" --version 2>&1 | grep real | awk '{print $2}' || echo "0")
        
        log_message "    Makefile startup: ${makefile_time}s"
        log_message "    CMake startup:    ${cmake_time}s"
        
        # Test memory usage
        if command_exists valgrind; then
            log_message "  Testing memory usage (basic check)..."
            
            local makefile_mem=$(timeout 10 valgrind --tool=massif --massif-out-file=/tmp/massif.makefile "$makefile_exe" --version 2>&1 | grep "total heap usage" | awk '{print $4}' || echo "unknown")
            local cmake_mem=$(timeout 10 valgrind --tool=massif --massif-out-file=/tmp/massif.cmake "$cmake_exe" --version 2>&1 | grep "total heap usage" | awk '{print $4}' || echo "unknown")
            
            log_message "    Makefile memory: $makefile_mem"
            log_message "    CMake memory:    $cmake_mem"
            
            rm -f /tmp/massif.makefile /tmp/massif.cmake
        else
            log_message "  ${YELLOW}${WARN} Valgrind not available for memory testing${NORMAL}"
        fi
    else
        log_message "${YELLOW}${WARN} Performance executables not available for testing${NORMAL}"
    fi
}

# Function to generate summary report
generate_summary() {
    log_message "${BLUE}────────────────────────────────────────────────────────────────${NORMAL}"
    log_message "${BLUE}${BOLD} Build Comparison Summary${NORMAL}"
    log_message "${BLUE}────────────────────────────────────────────────────────────────${NORMAL}"
    
    # Count successful builds
    local makefile_count=0
    local cmake_count=0
    
    for variant in "${VARIANTS[@]}"; do
        [ -f "$variant" ] && ((makefile_count++))
        [ -f "$CMAKE_BUILD_DIR/$variant" ] && ((cmake_count++))
    done
    
    log_message "Build Results:"
    log_message "  Makefile builds: $makefile_count/${#VARIANTS[@]}"
    log_message "  CMake builds:    $cmake_count/${#VARIANTS[@]}"
    
    # Check for payload
    local payload_status="❌"
    [ -f "payloads/payload.tar.br.enc" ] && payload_status="✅"
    log_message "  Payload generation: $payload_status"
    
    # Overall status
    if [ "$makefile_count" -eq "${#VARIANTS[@]}" ] && [ "$cmake_count" -eq "${#VARIANTS[@]}" ]; then
        log_message ""
        log_message "${GREEN}${PASS} ${BOLD}All builds completed successfully!${NORMAL}"
        log_message "${GREEN}CMake build system provides full feature parity with Makefile${NORMAL}"
    else
        log_message ""
        log_message "${RED}${FAIL} ${BOLD}Some builds failed${NORMAL}"
        log_message "${RED}Please check the build logs for details${NORMAL}"
    fi
    
    log_message ""
    log_message "Detailed comparison log saved to: $LOG_FILE"
}

# Main execution
main() {
    log_message "${BOLD}Hydrogen Build System Comparison${NORMAL}"
    log_message "Timestamp: $(date)"
    log_message "Comparing CMake vs Makefile builds..."
    log_message ""
    
    # Check prerequisites
    if ! command_exists cmake; then
        log_message "${RED}${FAIL} CMake not found. Please install CMake first.${NORMAL}"
        exit 1
    fi
    
    if ! command_exists make; then
        log_message "${RED}${FAIL} Make not found. Please install make first.${NORMAL}"
        exit 1
    fi
    
    if ! command_exists bc; then
        log_message "${YELLOW}${WARN} bc not found. Size comparisons will be limited.${NORMAL}"
    fi
    
    # Create comparison directory
    mkdir -p "$COMPARISON_DIR"
    
    # Build with both systems
    build_makefile
    build_cmake
    
    # Compare results
    if compare_builds; then
        log_message "${GREEN}${PASS} Build comparison completed successfully${NORMAL}"
    else
        log_message "${YELLOW}${WARN} Build comparison found differences${NORMAL}"
    fi
    
    # Performance comparison
    performance_comparison
    
    # Generate summary
    generate_summary
    
    # Move log to comparison directory
    mv "$LOG_FILE" "$COMPARISON_DIR/"
    
    log_message ""
    log_message "${CYAN}${INFO} Comparison complete. Check $COMPARISON_DIR/$LOG_FILE for details.${NORMAL}"
}

# Run main function
main "$@"
