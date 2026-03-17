#!/bin/bash
#
# Generate Brotli sidecar files for deployable text assets.
# Uses parallel processing with xargs for improved performance.
#
# Usage: ./generate-brotli.sh [TARGET_DIRECTORY]
#   If TARGET_DIRECTORY is not provided, uses $LITHIUM_DEPLOY or $LITHIUM_ROOT/dist
#
# Features:
#   - Parallel compression using xargs -P (auto-detects CPU cores)
#   - Timing measurements for performance tracking
#   - Timestamp-based incremental builds (skips up-to-date files)
#   - Uses system brotli CLI if available (faster), falls back to Node.js
#

set -euo pipefail

# Configuration
readonly ALLOWED_EXTENSIONS="css|html|json|svg|js|map"
readonly COMPRESSION_QUALITY="${BROTLI_QUALITY:-11}"
readonly PARALLEL_JOBS="${BROTLI_JOBS:-$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)}"

# Determine target directory
if [[ -n "${1:-}" ]]; then
    TARGET_DIR="$1"
elif [[ -n "${LITHIUM_DEPLOY:-}" ]]; then
    TARGET_DIR="$LITHIUM_DEPLOY"
elif [[ -n "${LITHIUM_ROOT:-}" ]]; then
    TARGET_DIR="$LITHIUM_ROOT/dist"
else
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    TARGET_DIR="$SCRIPT_DIR/../dist"
fi

# Resolve to absolute path
TARGET_DIR="$(cd "$TARGET_DIR" && pwd)"

# Check target directory exists
if [[ ! -d "$TARGET_DIR" ]]; then
    echo "[brotli] Error: Target directory does not exist: $TARGET_DIR" >&2
    exit 1
fi

# Timing: Record start time
START_TIME=$(date +%s.%N)
START_TIME_FMT=$(date '+%Y-%m-%d %H:%M:%S')
echo "[brotli] =================================================="
echo "[brotli] Brotli Compression Started: $START_TIME_FMT"
echo "[brotli] Target directory: $TARGET_DIR"
echo "[brotli] Parallel jobs: $PARALLEL_JOBS"
echo "[brotli] Compression quality: $COMPRESSION_QUALITY"
echo "[brotli] =================================================="

# Check for system brotli command
HAS_SYSTEM_BROTLI=false
BROTLI_CMD=""
if command -v brotli &> /dev/null; then
    HAS_SYSTEM_BROTLI=true
    BROTLI_CMD="brotli"
    echo "[brotli] Using system brotli CLI for compression"
else
    echo "[brotli] System brotli not found, using Node.js fallback"
fi

# Create temp directory for individual results
TEMP_DIR=$(mktemp -d)
readonly TEMP_DIR
readonly RESULTS_DIR="$TEMP_DIR/results"
mkdir -p "$RESULTS_DIR"

# Cleanup on exit
cleanup() {
    rm -rf "$TEMP_DIR"
}
trap cleanup EXIT

# Function to format bytes
format_bytes() {
    local bytes=$1
    if [[ $bytes -lt 1024 ]]; then
        echo "${bytes} B"
    elif [[ $bytes -lt 1048576 ]]; then
        echo "$(awk "BEGIN {printf \"%.2f\", $bytes/1024}") KB"
    elif [[ $bytes -lt 1073741824 ]]; then
        echo "$(awk "BEGIN {printf \"%.2f\", $bytes/1048576}") MB"
    else
        echo "$(awk "BEGIN {printf \"%.2f\", $bytes/1073741824}") GB"
    fi
}

# Function to check if file is eligible for compression
is_eligible() {
    local file="$1"
    local basename
    basename=$(basename "$file")
    local ext="${basename##*.}"
    
    # Skip existing .br files
    if [[ "$basename" == *.br ]]; then
        return 1
    fi
    
    # Check extension (case insensitive)
    ext_lower=$(echo "$ext" | tr '[:upper:]' '[:lower:]')
    if [[ "$ext_lower" =~ ^($ALLOWED_EXTENSIONS)$ ]]; then
        return 0
    fi
    
    # Special case for .map files
    if [[ "$basename" =~ \.(js|css)\.map$ ]]; then
        return 0
    fi
    
    return 1
}

# Function to compress a single file
# Writes result to individual file to avoid race conditions
compress_file() {
    local source_path="$1"
    local brotli_path="${source_path}.br"
    local result_file="$RESULTS_DIR/$(echo "$source_path" | tr '/' '_').result"
    
    local source_size
    source_size=$(stat -c%s "$source_path" 2>/dev/null || stat -f%z "$source_path" 2>/dev/null)
    
    # Check if brotli file exists and is up-to-date
    if [[ -f "$brotli_path" ]]; then
        local source_mtime brotli_mtime
        source_mtime=$(stat -c%Y "$source_path" 2>/dev/null || stat -f%m "$source_path" 2>/dev/null)
        brotli_mtime=$(stat -c%Y "$brotli_path" 2>/dev/null || stat -f%m "$brotli_path" 2>/dev/null)
        
        if [[ $brotli_mtime -ge $source_mtime ]]; then
            # File is up-to-date, skip
            local brotli_size
            brotli_size=$(stat -c%s "$brotli_path" 2>/dev/null || stat -f%z "$brotli_path" 2>/dev/null)
            echo "SKIPPED:$source_size:$brotli_size" > "$result_file"
            return 0
        fi
    fi
    
    # Compress the file
    local brotli_exists=0
    [[ -f "$brotli_path" ]] && brotli_exists=1
    
    if [[ "$HAS_SYSTEM_BROTLI" == true ]]; then
        # Use system brotli CLI (fastest)
        "$BROTLI_CMD" -q "$COMPRESSION_QUALITY" -f -o "$brotli_path" "$source_path"
    else
        # Fallback to Node.js
        node -e "
            const fs = require('fs');
            const zlib = require('zlib');
            const source = fs.readFileSync('$source_path');
            const compressed = zlib.brotliCompressSync(source, {
                params: {
                    [zlib.constants.BROTLI_PARAM_MODE]: zlib.constants.BROTLI_MODE_TEXT,
                    [zlib.constants.BROTLI_PARAM_QUALITY]: $COMPRESSION_QUALITY,
                    [zlib.constants.BROTLI_PARAM_SIZE_HINT]: source.length
                }
            });
            fs.writeFileSync('$brotli_path', compressed);
        "
    fi
    
    local brotli_size
    brotli_size=$(stat -c%s "$brotli_path" 2>/dev/null || stat -f%z "$brotli_path" 2>/dev/null)
    
    if [[ $brotli_exists -eq 1 ]]; then
        echo "UPDATED:$source_size:$brotli_size" > "$result_file"
    else
        echo "CREATED:$source_size:$brotli_size" > "$result_file"
    fi
}

export -f compress_file is_eligible format_bytes
export TEMP_DIR RESULTS_DIR HAS_SYSTEM_BROTLI BROTLI_CMD COMPRESSION_QUALITY ALLOWED_EXTENSIONS

# Find all eligible files and process in parallel
echo "[brotli] Scanning for files to compress..."

# Build list of files to process
mapfile -t files < <(find "$TARGET_DIR" -type f -print0 2>/dev/null | while IFS= read -r -d '' file; do
    if is_eligible "$file"; then
        echo "$file"
    fi
done)

TOTAL_FILES=${#files[@]}
echo "[brotli] Found $TOTAL_FILES eligible files"

if [[ $TOTAL_FILES -eq 0 ]]; then
    echo "[brotli] No files to compress"
    exit 0
fi

# Process files in parallel using xargs
echo "[brotli] Compressing with $PARALLEL_JOBS parallel jobs..."
printf '%s\n' "${files[@]}" | xargs -P "$PARALLEL_JOBS" -I {} bash -c 'compress_file "$@"' _ {}

# Aggregate results from individual files
CREATED=0
UPDATED=0
SKIPPED=0
TOTAL_SOURCE=0
TOTAL_BROTLI=0

for result_file in "$RESULTS_DIR"/*.result; do
    [[ -f "$result_file" ]] || continue
    
    IFS=':' read -r status source_size brotli_size < "$result_file"
    
    TOTAL_SOURCE=$((TOTAL_SOURCE + source_size))
    TOTAL_BROTLI=$((TOTAL_BROTLI + brotli_size))
    
    case "$status" in
        CREATED) CREATED=$((CREATED + 1)) ;;
        UPDATED) UPDATED=$((UPDATED + 1)) ;;
        SKIPPED) SKIPPED=$((SKIPPED + 1)) ;;
    esac
done

# Timing: Calculate elapsed time
END_TIME=$(date +%s.%N)
END_TIME_FMT=$(date '+%Y-%m-%d %H:%M:%S')
ELAPSED=$(awk "BEGIN {printf \"%.2f\", $END_TIME - $START_TIME}")

# Calculate savings
BYTES_SAVED=$((TOTAL_SOURCE - TOTAL_BROTLI))
if [[ $TOTAL_SOURCE -gt 0 ]]; then
    SAVINGS_PCT=$(awk "BEGIN {printf \"%.2f\", ($BYTES_SAVED / $TOTAL_SOURCE) * 100}")
else
    SAVINGS_PCT="0.00"
fi

# Output results
echo ""
echo "[brotli] =================================================="
echo "[brotli] Brotli Compression Complete: $END_TIME_FMT"
echo "[brotli] Elapsed time: ${ELAPSED}s"
echo "[brotli] --------------------------------------------------"
echo "[brotli] Eligible files: $TOTAL_FILES"
echo "[brotli] Created: $CREATED"
echo "[brotli] Updated: $UPDATED"
echo "[brotli] Skipped (up-to-date): $SKIPPED"
echo "[brotli] --------------------------------------------------"
echo "[brotli] Size before: $(format_bytes $TOTAL_SOURCE) ($TOTAL_SOURCE bytes)"
echo "[brotli] Size after : $(format_bytes $TOTAL_BROTLI) ($TOTAL_BROTLI bytes)"
echo "[brotli] Savings    : $(format_bytes $BYTES_SAVED) ($BYTES_SAVED bytes, ${SAVINGS_PCT}%)"
echo "[brotli] =================================================="

exit 0
