#!/bin/bash

# Markdown link checker script - xargs Parallel Processing Version
# Usage: ./github-sitemap-xargs.sh <markdown_file> [--debug] [--theme <Red|Blue>] [--profile] [--jobs <N>]
# Version: 0.7.0
# Description: Checks local markdown links in a repository using xargs parallel processing

# Application version
declare -r APPVERSION="0.7.0"

# Auto-detect optimal number of jobs
detect_cpu_count() {
    local cpu_count
    if command -v nproc >/dev/null 2>&1; then
        cpu_count=$(nproc)
    elif [[ -r /proc/cpuinfo ]]; then
        cpu_count=$(grep -c ^processor /proc/cpuinfo)
    elif command -v sysctl >/dev/null 2>&1; then
        cpu_count=$(sysctl -n hw.ncpu 2>/dev/null || echo 1)
    else
        cpu_count=1
    fi
    
    # Use CPU count but cap at 8 for reasonable performance
    if [[ "$cpu_count" -gt 8 ]]; then
        echo 8
    else
        echo "$cpu_count"
    fi
}

# Performance timing functions
declare -A timing_data
timing_start() {
    local name="$1"
    timing_data["${name}_start"]=$(date +%s%N)
}

timing_end() {
    local name="$1"
    local start_time="${timing_data["${name}_start"]}"
    if [[ -n "$start_time" ]]; then
        local end_time
        end_time=$(date +%s%N)
        local duration=$(( (end_time - start_time) / 1000000 ))
        timing_data["${name}_duration"]=$duration
        [[ "$PROFILE" == "true" ]] && echo "[PROFILE] $name: ${duration}ms" >&2
    fi
}

# Parse arguments
DEBUG=false
INPUT_FILE=""
IGNORE_FILE=""
TABLE_THEME="Red"
QUIET=false
NOREPORT=false
HELP=false
PROFILE=false
JOBS=""  # Will be set later if not specified

while [[ $# -gt 0 ]]; do
    case "$1" in
        --debug) DEBUG=true; shift ;;
        --quiet) QUIET=true; shift ;;
        --noreport) NOREPORT=true; shift ;;
        --help) HELP=true; shift ;;
        --profile) PROFILE=true; shift ;;
        --ignore)
            if [[ -n "$2" ]]; then
                IGNORE_FILE="$2"
                shift 2
            else
                echo "Error: --ignore requires a file path" >&2
                exit 1
            fi
            ;;
        --jobs)
            if [[ -n "$2" && "$2" =~ ^[0-9]+$ ]]; then
                JOBS="$2"
                shift 2
            else
                echo "Error: --jobs requires a number" >&2
                exit 1
            fi
            ;;
        --theme)
            if [[ -n "$2" && "$2" =~ ^(Red|Blue)$ ]]; then
                TABLE_THEME="$2"
                shift 2
            else
                echo "Error: --theme requires 'Red' or 'Blue'" >&2
                exit 1
            fi
            ;;
        *) if [[ -z "$INPUT_FILE" ]]; then INPUT_FILE="$1"; shift; else echo "Error: Unknown option: $1" >&2; exit 1; fi ;;
    esac
done

# Check if help is requested
if [[ "$HELP" == "true" ]]; then
    echo "Usage: $0 <markdown_file> [--debug] [--quiet] [--noreport] [--help] [--theme <Red|Blue>] [--profile] [--jobs <N>] [--ignore <file>]"
    echo "Options:"
    echo "  --debug      Enable debug logging"
    echo "  --quiet      Display only tables, suppress other output"
    echo "  --noreport   Do not create a report file"
    echo "  --help       Display this help message"
    echo "  --theme      Set table theme to 'Red' or 'Blue' (default: Red)"
    echo "  --profile    Enable performance profiling"
    echo "  --jobs       Number of parallel jobs (default: auto-detected $(detect_cpu_count))"
    echo "  --ignore     Specify a file with ignore patterns (like .lintignore)"
    exit 0
fi

# Check if a file parameter is provided
if [[ -z "$INPUT_FILE" ]]; then
    echo "Usage: $0 <markdown_file> [options]" >&2
    exit 1
fi

# Debug logging function
debug_log() {
    [[ "$DEBUG" == "true" ]] && echo "[DEBUG] $(date '+%Y-%m-%d %H:%M:%S'): $*" >&2
}

timing_start "total_execution"
timing_start "initialization"

if [[ "$DEBUG" == "true" ]]; then
    debug_log "Starting xargs parallel script with \"$JOBS\" jobs (auto-detected: $(detect_cpu_count) CPUs)"
fi

# Check dependencies
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TABLES_SCRIPT="${SCRIPT_DIR}/tables.sh"
if [[ ! -f "$TABLES_SCRIPT" ]]; then
    echo "Error: $TABLES_SCRIPT not found" >&2
    exit 1
fi

if ! command -v jq >/dev/null 2>&1; then
    echo "Error: jq is required but not installed" >&2
    exit 1
fi

if ! command -v xargs >/dev/null 2>&1; then
    echo "Error: xargs is required but not found" >&2
    exit 1
fi

# Initialize variables
declare -A file_exists_cache
declare -A checked_files
declare -A file_summary
declare -a missing_links
declare -a orphaned_files
missing_count=0

# Store the original working directory
declare original_dir
original_dir=$(pwd)
output_file="$original_dir/sitemap_report.txt"

# Set repo root based on input file's directory
declare input_dir
if [[ "$INPUT_FILE" == /* ]]; then
    input_dir=$(dirname "$INPUT_FILE")
else
    input_dir=$(cd "$original_dir" && realpath -m "$(dirname "$INPUT_FILE")" 2>/dev/null)
fi
if [[ -z "$input_dir" ]]; then
    echo "Error: Cannot resolve input file directory for $INPUT_FILE" >&2
    exit 1
fi
if ! cd "$input_dir" 2>/dev/null; then
    echo "Error: Cannot change to input file directory $input_dir for $INPUT_FILE" >&2
    exit 1
fi
repo_root="$input_dir"
debug_log "Set repo root to $repo_root for input file $INPUT_FILE"

timing_end "initialization"

# Build file existence cache with optimized find
timing_start "build_file_cache"
build_file_cache() {
    debug_log "Building file existence cache"
    local cache_count=0
    local ignore_patterns=()
    
    # Read ignore patterns from specified ignore file or default to .lintignore in same dir as input file
    local ignore_file=""
    if [[ -n "$IGNORE_FILE" ]]; then
        if [[ "$IGNORE_FILE" == /* ]]; then
            ignore_file="$IGNORE_FILE"
        else
            ignore_file="$original_dir/$IGNORE_FILE"
        fi
    else
        # Default to .lintignore in the same directory as the input markdown file
        ignore_file="$repo_root/.lintignore"
    fi
    
    if [[ -f "$ignore_file" ]]; then
        debug_log "Reading ignore patterns from $ignore_file"
        while IFS= read -r line; do
            # Skip empty lines and comments
            if [[ -n "$line" && ! "$line" =~ ^[[:space:]]*# ]]; then
                ignore_patterns+=("$line")
                debug_log "Added ignore pattern: $line"
            fi
        done < "$ignore_file"
    else
        debug_log "No ignore file found at $ignore_file"
    fi
    
    # Single find command with optimized output
    while IFS= read -r -d '' entry; do
        local path="${entry%:*}"
        local type="${entry#*:}"
        local rel_path=""
        if [[ "$path" == "$repo_root"* ]]; then
            rel_path="${path#"$repo_root"/}"
        else
            rel_path="$path"
        fi
        
        local excluded=false
        for pattern in "${ignore_patterns[@]}"; do
            shopt -s extglob
            if [[ "$rel_path" == @($pattern) || "/$rel_path" == @($pattern) ]]; then
                shopt -u extglob
                debug_log "Excluding file due to ignore pattern '$pattern': $rel_path"
                excluded=true
                break
            fi
            shopt -u extglob
        done
        
        if [[ "$excluded" == false ]]; then
            file_exists_cache["$path"]="$type"
            ((cache_count++))
        fi
    done < <(find "$repo_root" -name .git -prune -o \( -type f -printf '%p:f\0' \) -o \( -type d -printf '%p:d\0' \) 2>/dev/null)
    
    debug_log "Built file existence cache with $cache_count entries"
}

build_file_cache
timing_end "build_file_cache"

# Optimized file processing function for xargs
process_file_batch() {
    local cache_file="$1"
    local repo_root="$2"
    local original_dir="$3"
    local results_file="$4"
    
    # Load cache once per batch
    declare -A local_cache
    declare -A processed_files
    while IFS=':' read -r path type; do
        local_cache["$path"]="$type"
    done < "$cache_file"
    
    # Fast file existence check
    file_exists() {
        local path="$1"
        # Check exact path first
        [[ -n "${local_cache[$path]}" ]] && return 0
        
        # If path ends with /, try without the trailing slash (for directory links)
        if [[ "$path" == */ ]]; then
            local path_no_slash="${path%/}"
            [[ -n "${local_cache[$path_no_slash]}" ]] && return 0
        fi
        
        # If path doesn't end with /, try with trailing slash (for directory links)
        if [[ "$path" != */ ]]; then
            local path_with_slash="$path/"
            [[ -n "${local_cache[$path_with_slash]}" ]] && return 0
        fi
        
        return 1
    }
    
    # Check if a path should be ignored based on patterns
    should_ignore_path() {
        local path="$1"
        local rel_path=""
        if [[ "$path" == "$repo_root"* ]]; then
            rel_path="${path#"$repo_root"/}"
        else
            rel_path="$path"
        fi
        
        # Read ignore patterns from the same file used in main script
        local ignore_file=""
        if [[ -n "$IGNORE_FILE" ]]; then
            if [[ "$IGNORE_FILE" == /* ]]; then
                ignore_file="$IGNORE_FILE"
            else
                ignore_file="$original_dir/$IGNORE_FILE"
            fi
        else
            ignore_file="$repo_root/.lintignore"
        fi
        
        if [[ -f "$ignore_file" ]]; then
            while IFS= read -r line; do
                # Skip empty lines and comments
                if [[ -n "$line" && ! "$line" =~ ^[[:space:]]*# ]]; then
                    shopt -s extglob
                    if [[ "$rel_path" == @($line) || "/$rel_path" == @($line) || "$rel_path/" == @($line) || "/$rel_path/" == @($line) ]]; then
                        shopt -u extglob
                        return 0  # Should ignore
                    fi
                    shopt -u extglob
                fi
            done < "$ignore_file"
        fi
        
        return 1  # Should not ignore
    }
    
    # Optimized link extraction
    extract_links() {
        local file="$1"
        [[ ! -r "$file" ]] && return 1
        
        # Ultra-fast link extraction using optimized grep pipeline
        grep -oE '\[([^]]*)\]\(([^)]+)\)' "$file" 2>/dev/null | \
        sed -n 's/.*(\([^)]*\)).*/\1/p' | \
        awk '!seen[$0]++' | \
        while IFS= read -r link; do
            # Skip external links quickly
            case "$link" in
                \#*|http*|ftp*|mailto:*) continue ;;
            esac
            
            # Process GitHub and local links
            if [[ "$link" =~ ^https://github.com/[^/]+/[^/]+/blob/main/(.+)$ ]]; then
                local relative_path="${BASH_REMATCH[1]}"
                if [[ "$relative_path" =~ \.md($|#) ]]; then
                    echo "$relative_path"
                else
                    echo "non_md:$relative_path"
                fi
            else
                if [[ "$link" =~ \.md($|#) ]]; then
                    echo "$link"
                else
                    echo "non_md:$link"
                fi
            fi
        done
    }
    
    # Process each file from stdin
    while IFS= read -r file; do
        [[ -z "$file" ]] && continue
        
        # Resolve absolute path efficiently
        local abs_file
        if [[ "$file" == /* ]]; then
            abs_file="$file"
        else
            abs_file=$(cd "$original_dir" && cd "$(dirname "$file")" && pwd)
            abs_file="$abs_file/$(basename "$file")"
        fi
        
        # Skip if file doesn't exist
        file_exists "$abs_file" || continue
        
        local base_dir
        base_dir=$(dirname "$abs_file")
        local rel_file
        if [[ "$abs_file" == "$repo_root"* ]]; then
            rel_file="${abs_file#"$repo_root"/}"
        else
            rel_file="$abs_file"
        fi
        
        # Skip if file is already processed
        if [[ -n "${processed_files[$abs_file]}" ]]; then
            debug_log "File already processed, skipping: $rel_file"
            continue
        fi
        
        local links_total=0 links_checked=0 links_missing=0
        local linked_files=()
        local missing_list=()
        
        # Process all links
        while IFS= read -r link; do
            ((links_total++))
            local actual_link="$link"
            local traverse=true
            
            if [[ "$link" =~ ^non_md:(.+)$ ]]; then
                actual_link="${BASH_REMATCH[1]}"
                traverse=false
            fi
            
            # Resolve link path with optimized logic
            local abs_link
            if [[ "$actual_link" =~ ^https://github.com/[^/]+/[^/]+/blob/main/(.+)$ ]]; then
                abs_link="$repo_root/${BASH_REMATCH[1]}"
            elif [[ "$actual_link" == /* ]]; then
                abs_link="$repo_root$actual_link"
            else
                abs_link="$base_dir/$actual_link"
            fi
            
            # Normalize path efficiently (only when needed)
            case "$abs_link" in
                */./*|*/../*) abs_link=$(realpath -m "$abs_link" 2>/dev/null) ;;
            esac
            
            local link_file="${abs_link%%#*}"
            
            if file_exists "$link_file"; then
                ((links_checked++))
                if [[ "$traverse" == "true" && "$link_file" =~ \.md$ ]]; then
                    linked_files+=("$link_file")
                fi
            else
                # Check if the missing link should be ignored
                if should_ignore_path "$link_file"; then
                    # Treat ignored missing links as checked to avoid reporting them
                    ((links_checked++))
                else
                    ((links_missing++))
                    missing_list+=("$rel_file:$actual_link")
                fi
            fi
        done < <(extract_links "$abs_file")
        
        # Output results in structured format
        echo "SUMMARY:$abs_file:$links_total:$links_checked:$links_missing:$rel_file" >> "$results_file"
        for missing in "${missing_list[@]}"; do
            echo "MISSING:$missing" >> "$results_file"
        done
        for linked in "${linked_files[@]}"; do
            echo "LINKED:$linked" >> "$results_file"
        done
    done
}

export -f process_file_batch

# Clear output file if needed
if [[ "$NOREPORT" != "true" ]]; then
    : > "$output_file"
fi

# Create temporary files for parallel processing
timing_start "parallel_setup"
temp_dir=$(mktemp -d) || { echo "Error: Failed to create temporary directory" >&2; exit 1; }
cache_file="$temp_dir/file_cache.txt"
results_file="$temp_dir/results.txt"
files_to_process="$temp_dir/files_list.txt"

# Initialize results file
: > "$results_file"

# Export cache to file
for path in "${!file_exists_cache[@]}"; do
    echo "$path:${file_exists_cache[$path]}"
done > "$cache_file"

debug_log "Parallel setup complete"
timing_end "parallel_setup"

# Process files in parallel using xargs
timing_start "parallel_processing"

# Set JOBS if not specified by user
if [[ -z "$JOBS" ]]; then
    JOBS=$(detect_cpu_count)
fi

debug_log "Starting parallel processing with $JOBS jobs using xargs"

# Start with input file and iteratively discover linked files
declare -A processed_files
echo "$INPUT_FILE" > "$files_to_process"
processed_files["$INPUT_FILE"]=1

iteration=0
while [[ -s "$files_to_process" ]]; do
    ((iteration++))
    debug_log "Processing iteration $iteration with $(wc -l < "$files_to_process") files"
    
    # Process current batch in parallel using xargs
    xargs -P "$JOBS" -I {} bash -c "process_file_batch \"\$1\" \"\$2\" \"\$3\" \"\$4\" <<< \"\$5\"" _ "$cache_file" "$repo_root" "$original_dir" "$results_file" {} < "$files_to_process"
    
    # Extract newly discovered files from results
    new_files_temp="$temp_dir/new_files_$iteration.txt"
    : > "$new_files_temp"
    
    while IFS= read -r line; do
        if [[ "$line" =~ ^LINKED:(.+)$ ]]; then
            linked_file="${BASH_REMATCH[1]}"
            if [[ -z "${processed_files[$linked_file]}" ]]; then
                processed_files["$linked_file"]=1
                echo "$linked_file" >> "$new_files_temp"
            fi
        fi
    done < "$results_file"
    
    # Update files to process for next iteration
    if [[ -s "$new_files_temp" ]]; then
        mv "$new_files_temp" "$files_to_process"
        debug_log "Discovered $(wc -l < "$files_to_process") new files"
    else
        : > "$files_to_process"
        debug_log "No new files discovered, processing complete"
    fi
done

timing_end "parallel_processing"

# Process results
timing_start "process_results"
debug_log "Processing parallel results"

while IFS= read -r line; do
    if [[ "$line" =~ ^SUMMARY:([^:]+):([^:]+):([^:]+):([^:]+):(.+)$ ]]; then
        abs_file="${BASH_REMATCH[1]}"
        total="${BASH_REMATCH[2]}"
        checked="${BASH_REMATCH[3]}"
        missing="${BASH_REMATCH[4]}"
        rel_file="${BASH_REMATCH[5]}"
        
        file_summary["$abs_file"]="$total,$checked,$missing,$rel_file"
        checked_files["$abs_file"]=1
        ((missing_count += missing))
        
    elif [[ "$line" =~ ^MISSING:(.+)$ ]]; then
        missing_links+=("${BASH_REMATCH[1]}")
        if [[ "$NOREPORT" != "true" ]]; then
            IFS=':' read -r file link <<< "${BASH_REMATCH[1]}"
            echo "Missing link in $file: $link" >> "$output_file"
        fi
    fi
done < "$results_file"

# Find orphaned files
for path in "${!file_exists_cache[@]}"; do
    if [[ "$path" == *.md && "${file_exists_cache[$path]}" == "f" ]]; then
        if [[ -z "${checked_files[$path]}" ]]; then
            rel_file=""
            if [[ "$path" == "$repo_root"* ]]; then
                rel_file="${path#"$repo_root"/}"
            else
                rel_file="$path"
            fi
            orphaned_files+=("$rel_file")
        fi
    fi
done

timing_end "process_results"

# Generate JSON and render tables
timing_start "json_generation"

# Generate JSON functions (optimized)
generate_reviewed_files_json() {
    local json_file="$1"
    local json_data=""
    for abs_file in "${!file_summary[@]}"; do
        IFS=',' read -r total checked missing rel_file <<< "${file_summary[$abs_file]}"
        folder_path=$(dirname "$rel_file")
        [[ "$folder_path" == "." ]] && folder_path="."
        
        if [[ -n "$json_data" ]]; then
            json_data+=","
        fi
        json_data+="{\"file\":\"$rel_file\",\"folder\":\"$folder_path\",\"total\":$total,\"checked\":$checked,\"missing\":$missing}"
    done
    
    echo "[$json_data]" | jq -c 'sort_by(.file | split("/") | . as $parts | {path: $parts, modified_path: (if ($parts | length > 0 and ($parts[-1] | test("\\."))) then ($parts[:-1] + ["A" + $parts[-1]]) else $parts end | join("/"))} | .modified_path)' > "$json_file"
}

generate_missing_links_json() {
    local json_file="$1"
    local json_data=""
    for entry in "${missing_links[@]}"; do
        IFS=':' read -r file link <<< "$entry"
        if [[ -n "$json_data" ]]; then
            json_data+=","
        fi
        link_escaped="${link//\"/\\\"}"
        json_data+="{\"file\":\"$file\",\"link\":\"$link_escaped\"}"
    done
    
    echo "[$json_data]" | jq -c 'sort_by(.file)' > "$json_file"
}

generate_orphaned_files_json() {
    local json_file="$1"
    local json_data=""
    for rel_file in "${orphaned_files[@]}"; do
        if [[ -n "$json_data" ]]; then
            json_data+=","
        fi
        json_data+="{\"file\":\"$rel_file\"}"
    done
    
    echo "[$json_data]" | jq -c 'sort_by(.file)' > "$json_file"
}

# Layout generation functions
generate_reviewed_layout_json() {
    local json_file="$1"
    jq -n --arg theme "$TABLE_THEME" '{
        theme: $theme,
        title: "Reviewed Files Summary",
        columns: [
            {header: "File", key: "file", datatype: "text", "summary":"count"},
            {header: "Folder", key: "folder", datatype: "text", "visible": false, "break": true},
            {header: "Total Links", key: "total", datatype: "int", justification: "right", "summary":"sum"},
            {header: "Found Links", key: "checked", datatype: "int", justification: "right", "summary":"sum"},
            {header: "Missing Links", key: "missing", datatype: "int", justification: "right","summary":"sum"}
        ]
    }' > "$json_file" 2>/dev/null
}

generate_missing_layout_json() {
    local json_file="$1"
    jq -n --arg theme "$TABLE_THEME" '{
        theme: $theme,
        title: "Missing Links",
        columns: [
            {header: "File", key: "file", datatype: "text", "summary":"count"},
            {header: "Missing Link", key: "link", datatype: "text" }
        ]
    }' > "$json_file" 2>/dev/null
}

generate_orphaned_layout_json() {
    local json_file="$1"
    jq -n --arg theme "$TABLE_THEME" '{
        theme: $theme,
        title: "Orphaned Markdown Files",
        columns: [
            {header: "File", key: "file", datatype: "text", "summary":"count"}
        ]
    }' > "$json_file" 2>/dev/null
}

# Generate JSON files
reviewed_data_json="$temp_dir/reviewed_data.json"
reviewed_layout_json="$temp_dir/reviewed_layout.json"
missing_data_json="$temp_dir/missing_data.json"
missing_layout_json="$temp_dir/missing_layout.json"
orphaned_data_json="$temp_dir/orphaned_data.json"
orphaned_layout_json="$temp_dir/orphaned_layout.json"

generate_reviewed_files_json "$reviewed_data_json"
generate_reviewed_layout_json "$reviewed_layout_json"
generate_missing_links_json "$missing_data_json"
generate_missing_layout_json "$missing_layout_json"
generate_orphaned_files_json "$orphaned_data_json"
generate_orphaned_layout_json "$orphaned_layout_json"

timing_end "json_generation"

# Output results
if [[ "$QUIET" != "true" ]]; then
    echo "Markdown Link Checker v$APPVERSION (xargs Parallel Mode - $JOBS jobs)"
    echo "Issues found: $missing_count"
    echo ""
fi

# Render tables
timing_start "table_rendering"
[[ "$DEBUG" == "true" ]] && debug_flag="--debug" || debug_flag=""

bash "$TABLES_SCRIPT" "$reviewed_layout_json" "$reviewed_data_json" ${debug_flag:+"$debug_flag"}
if [[ ${#missing_links[@]} -gt 0 ]]; then
    bash "$TABLES_SCRIPT" "$missing_layout_json" "$missing_data_json" ${debug_flag:+"$debug_flag"}
fi
if [[ ${#orphaned_files[@]} -gt 0 ]]; then
    bash "$TABLES_SCRIPT" "$orphaned_layout_json" "$orphaned_data_json" ${debug_flag:+"$debug_flag"}
fi
timing_end "table_rendering"

# Write summary to report file
if [[ "$NOREPORT" != "true" ]]; then
    {
        echo -e "\n=== Link Check Summary ==="
        echo "Files processed: ${#checked_files[@]}"
        echo "Total missing links: $missing_count"
        echo "Orphaned files: ${#orphaned_files[@]}"
        echo ""
        
        if [[ ${#orphaned_files[@]} -gt 0 ]]; then
            echo "=== Orphaned Markdown Files ==="
            for file in "${orphaned_files[@]}"; do
                echo "File: $file"
            done
            echo ""
        fi
    } >> "$output_file"
fi

# Clean up
rm -rf "$temp_dir" 2>/dev/null

# Final message
if [[ "$QUIET" != "true" ]]; then
    echo ""
    if [[ "$NOREPORT" != "true" ]]; then
        echo "Link check complete. Detailed report written to $output_file"
    else
        echo "Link check complete. Report generation skipped due to --noreport option"
    fi
fi

timing_end "total_execution"

# Performance summary
if [[ "$PROFILE" == "true" ]]; then
    echo "" >&2
    echo "[PROFILE] Performance Summary (xargs Parallel Mode - $JOBS jobs):" >&2
    echo "[PROFILE] Total execution: ${timing_data[total_execution_duration]}ms" >&2
    echo "[PROFILE] Initialization: ${timing_data[initialization_duration]}ms" >&2
    echo "[PROFILE] Build file cache: ${timing_data[build_file_cache_duration]}ms" >&2
    echo "[PROFILE] Parallel setup: ${timing_data[parallel_setup_duration]}ms" >&2
    echo "[PROFILE] Parallel processing: ${timing_data[parallel_processing_duration]}ms" >&2
    echo "[PROFILE] Process results: ${timing_data[process_results_duration]}ms" >&2
    echo "[PROFILE] JSON generation: ${timing_data[json_generation_duration]}ms" >&2
    echo "[PROFILE] Table rendering: ${timing_data[table_rendering_duration]}ms" >&2
fi

# Exit with issue count
total_issues=$((missing_count + ${#orphaned_files[@]}))
exit $total_issues
