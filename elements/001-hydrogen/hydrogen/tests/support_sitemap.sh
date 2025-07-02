#!/bin/bash

# Markdown link checker script
# Usage: ./github-md-links.sh <markdown_file> [--debug] [--theme <Red|Blue>]
# Version: 0.4.0
# Description: Checks local markdown links in a repository, reports missing links,
#              and identifies orphaned markdown files.

# Application version
declare -r APPVERSION="0.4.1"

# Change history
# 0.4.1 - 2025-06-30 - Enhanced .lintignore exclusion handling, fixed shellcheck warning for command execution
# 0.4.0 - 2025-06-19 - Updated link checking to recognize existing folders as valid links
# 0.3.9 - 2025-06-15 - Fixed repo root detection to use input file's directory, improved path resolution
# 0.3.8 - 2025-06-15 - Fixed find_all_md_files path resolution, reordered -maxdepth, enhanced debug logging
# 0.3.7 - 2025-06-15 - Optimized find_all_md_files with -prune for .git, fixed orphaned files detection
# 0.3.6 - 2025-06-15 - Fixed orphaned files table, empty link counts, added --theme option, added orphaned files to report
# 0.3.5 - 2025-06-15 - Fixed tables.sh invocation to avoid empty argument, improved debug output separation
# 0.3.4 - 2025-06-15 - Fixed debug flag handling to pass --debug to tables.sh only when DEBUG=true
# 0.3.3 - 2025-06-15 - Enhanced find_all_md_files to avoid symlinks, log errors, and use shorter timeout
# 0.3.2 - 2025-06-15 - Improved find_all_md_files with depth limit, timeout, and error handling
# 0.3.1 - 2025-06-15 - Changed to execute tables.sh instead of sourcing, per domain_info.sh
# 0.3.0 - 2025-06-15 - Added --debug flag for verbose logging, improved robustness
# 0.2.0 - 2025-06-15 - Added table output using tables.sh, version display, issue count, missing links table, and orphaned files table
# 0.1.0 - 2025-06-15 - Initial version with basic link checking and summary output

# Parse arguments
DEBUG=false
INPUT_FILE=""
TABLE_THEME="Red"  # Default theme
QUIET=false
NOREPORT=false
HELP=false
while [[ $# -gt 0 ]]; do
    case "$1" in
        --debug) DEBUG=true; shift ;;
        --quiet) QUIET=true; shift ;;
        --noreport) NOREPORT=true; shift ;;
        --help) HELP=true; shift ;;
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
    echo "Usage: $0 <markdown_file> [--debug] [--quiet] [--noreport] [--help] [--theme <Red|Blue>]"
    echo "Options:"
    echo "  --debug      Enable debug logging"
    echo "  --quiet      Display only tables, suppress other output"
    echo "  --noreport   Do not create a report file"
    echo "  --help       Display this help message"
    echo "  --theme      Set table theme to 'Red' or 'Blue' (default: Red)"
    exit 0
fi

# Check if a file parameter is provided
if [[ -z "$INPUT_FILE" ]]; then
    echo "Usage: $0 <markdown_file> [--debug] [--quiet] [--noreport] [--help] [--theme <Red|Blue>]" >&2
    exit 1
fi

# Debug logging function
debug_log() {
    [[ "$DEBUG" == "true" ]] && echo "[DEBUG] $(date '+%Y-%m-%d %H:%M:%S'): $*" >&2
}

debug_log "Starting script with input: $INPUT_FILE, debug: $DEBUG, theme: $TABLE_THEME"

# Check if tables.sh exists
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TABLES_SCRIPT="${SCRIPT_DIR}/support_tables.sh"
if [[ ! -f "$TABLES_SCRIPT" ]]; then
    echo "Error: $TABLES_SCRIPT not found" >&2
    exit 1
fi

debug_log "Tables script: $TABLES_SCRIPT"

# Check for jq dependency
if ! command -v jq >/dev/null 2>&1; then
    echo "Error: jq is required but not installed" >&2
    exit 1
fi

# Check for timeout command
if ! command -v timeout >/dev/null 2>&1; then
    debug_log "Warning: timeout command not found, proceeding without timeout"
    TIMEOUT=""
else
    TIMEOUT="timeout 10s"
fi

# Initialize variables
declare -A checked_files  # Track checked files
declare -A linked_files   # Track files that have been linked to
declare -A file_summary  # Store summary: links_total, links_checked, links_missing, rel_file
declare -a missing_links  # Store missing links: file,link
declare -a orphaned_files  # Store orphaned files for report
declare -a queue=("$INPUT_FILE")  # Queue for files to process
# Store the original working directory at the start of the script
original_dir=$(pwd)
output_file="$original_dir/sitemap_report.txt"
debug_log "Original working directory: $original_dir"
# Set repo root based on input file's directory
# First, resolve the absolute path of the input file relative to the original directory
if [[ "$INPUT_FILE" == /* ]]; then
    input_dir=$(dirname "$INPUT_FILE")
else
    input_dir=$(cd "$original_dir" && cd "$(dirname "$INPUT_FILE")" && pwd)
fi
debug_log "Input file directory: $input_dir"
cd "$input_dir" || { echo "Error: Cannot change to input file directory $input_dir" >&2; exit 1; }
repo_root="$input_dir"
debug_log "Repository root set to input file directory: $repo_root"

# Function to extract local markdown links from a file
extract_links() {
    local file="$1"
    debug_log "Extracting links from $file"
    if [[ ! -r "$file" ]]; then
        debug_log "File $file is not readable"
        return 1
    fi
    # Extract markdown links [text](link) and get just the link part
    # Use non-greedy matching to get individual links
    grep -o '\[[^]]*\]([^)]*)' "$file" 2>/dev/null | sed 's/.*](\([^)]*\)).*/\1/' | sort -u | while read -r link; do
        debug_log "Found link: $link"
        # Handle GitHub URLs
        if [[ "$link" =~ ^https://github.com/[^/]+/[^/]+/blob/main/(.+)$ ]]; then
            relative_path="${BASH_REMATCH[1]}"
            debug_log "GitHub URL detected, extracted relative path: $relative_path"
            # Only include .md files for traversal
            if [[ "$relative_path" =~ \.md($|#) ]]; then
                debug_log "Link is a markdown file, including for traversal: $relative_path"
                echo "$relative_path"
            else
                debug_log "Link is not a markdown file, counting but not traversing: $relative_path"
                # Count non-md GitHub links but don't traverse
                echo "non_md:$relative_path"
            fi
        # Handle relative local paths
        elif [[ ! "$link" =~ ^(#|http|ftp|mailto:) ]]; then
            debug_log "Local relative link detected: $link"
            # Only include .md files for traversal
            if [[ "$link" =~ \.md($|#) ]]; then
                debug_log "Local link is a markdown file, including for traversal: $link"
                echo "$link"
            else
                debug_log "Local link is not a markdown file, counting but not traversing: $link"
                # Count non-md local links but don't traverse
                echo "non_md:$link"
            fi
        else
            debug_log "Link does not match criteria (not GitHub or local), skipping: $link"
        fi
    done
}

# Function to resolve relative path to absolute
resolve_path() {
    local base_dir="$1"
    local rel_path="$2"
    debug_log "Resolving path: $rel_path from $base_dir"
    # Convert relative path to absolute from base_dir
    realpath -m "$base_dir/$rel_path" 2>/dev/null
}

# Function to get path relative to repo root
relative_to_root() {
    local abs_path="$1"
    debug_log "Converting to relative path: $abs_path"
    # Get path relative to repo_root
    if [[ -n "$abs_path" ]]; then
        realpath --relative-to="$repo_root" "$abs_path" 2>/dev/null || echo "$abs_path"
    else
        echo ""
    fi
}

# Function to process a markdown file
process_file() {
    local file="$1"
    debug_log "Processing file: $file"
    local abs_file
    # Resolve absolute path relative to the original directory or repository root
    if [[ "$file" == /* ]]; then
        abs_file="$file"
    else
        abs_file=$(cd "$original_dir" && cd "$(dirname "$file")" && pwd)/$(basename "$file")
    fi
    debug_log "Absolute file path: $abs_file"
    
    # Skip if already checked
    if [[ -n "${checked_files[$abs_file]}" ]]; then
        debug_log "File already checked, skipping"
        return
    fi

    # Check if file should be excluded based on .lintignore patterns
    local rel_file
    rel_file=$(relative_to_root "$abs_file")
    local ignore_file="$repo_root/.lintignore"
    local ignore_patterns=()
    
    if [[ -f "$ignore_file" ]]; then
        debug_log "Checking ignore patterns from $ignore_file for $rel_file"
        while IFS= read -r line; do
            # Skip empty lines and comments
            if [[ -n "$line" && ! "$line" =~ ^[[:space:]]*# ]]; then
                ignore_patterns+=("$line")
            fi
        done < "$ignore_file"
        for pattern in "${ignore_patterns[@]}"; do
            shopt -s extglob
            if [[ "$rel_file" == @($pattern) || "/$rel_file" == @($pattern) ]]; then
                shopt -u extglob
                debug_log "Skipping file due to .lintignore pattern '$pattern': $rel_file"
                return
            fi
            shopt -u extglob
        done
    fi

    # Mark file as checked
    checked_files[$abs_file]=1
    debug_log "Marked file as checked"

    # Skip if file doesn't exist
    if [[ ! -f "$abs_file" ]]; then
        if [[ "$NOREPORT" != "true" ]]; then
            echo "Warning: File not found: $rel_file" >> "$output_file"
        fi
        debug_log "File not found: $abs_file"
        return
    fi

    # Initialize counters for this file
    local links_total=0 links_checked=0 links_missing=0
    local base_dir
    base_dir=$(dirname "$abs_file")
    debug_log "Relative file path: $rel_file"

    # Extract and process links
    while IFS= read -r link; do
        ((links_total++))
        debug_log "Processing link: $link"
        local actual_link="$link"
        # Check if it's a non-md link (counted but not traversed)
        if [[ "$link" =~ ^non_md:(.+)$ ]]; then
            actual_link="${BASH_REMATCH[1]}"
            debug_log "Non-md link, will count but not traverse: $actual_link"
        fi
        # Resolve link to absolute path
        local abs_link
        # For GitHub links, use repo_root as base
        if [[ "$actual_link" =~ ^https://github.com/[^/]+/[^/]+/blob/main/(.+)$ ]]; then
            abs_link=$(resolve_path "$repo_root" "${BASH_REMATCH[1]}")
        # For absolute local paths starting with '/', use repo_root
        elif [[ "$actual_link" =~ ^/ ]]; then
            abs_link=$(resolve_path "$repo_root" "$actual_link")
        # For relative local paths, use the directory of the current file as base
        else
            abs_link=$(resolve_path "$base_dir" "$actual_link")
        fi
        debug_log "Resolved link to: $abs_link"
        # Remove any anchor (#) for file existence check
        local link_file
        link_file="${abs_link%%#*}"
        debug_log "Link file (no anchor): $link_file"

        if [[ -f "$link_file" ]] || [[ -d "$link_file" ]]; then
            ((links_checked++))
            debug_log "Link exists"
            # Mark this file as linked to (for orphan detection)
            if [[ "$link_file" =~ \.md$ ]]; then
                linked_files[$link_file]=1
                debug_log "Marked as linked: $link_file"
            fi
            # Add linked markdown file to queue only if it's a .md file and not marked as non_md
            if [[ "$link_file" =~ \.md$ && ! "$link" =~ ^non_md: ]]; then
                queue+=("$link_file")
                debug_log "Added to queue: $link_file"
            else
                debug_log "Link is not for traversal (non-md or marked as non_md)"
            fi
        else
            ((links_missing++))
            ((missing_count++))
            if [[ "$NOREPORT" != "true" ]]; then
                echo "Missing link in $rel_file: $actual_link" >> "$output_file"
            fi
            missing_links+=("$rel_file:$actual_link")
            debug_log "Missing link: $actual_link in $rel_file"
        fi
    done < <(extract_links "$abs_file")

    # Store summary for this file
    file_summary[$abs_file]="$links_total,$links_checked,$links_missing,$rel_file"
    debug_log "Stored summary: total=$links_total, checked=$links_checked, missing=$links_missing, file=$rel_file"
}

# Function to find all .md files in the repo, honoring .lintignore exclusions
find_all_md_files() {
    debug_log "Finding all .md files in $repo_root"
    # Use timeout, maxdepth, -P, and -prune to avoid symlinks and .git
    local find_output="${temp_dir}/find_output.log"
    local find_errors="${temp_dir}/find_errors.log"
    local ignore_file="$repo_root/.lintignore"
    local ignore_patterns=()
    
    # Read ignore patterns from .lintignore if it exists
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
        debug_log "No .lintignore file found at $ignore_file"
    fi
    
    # Execute find command to get all markdown files, excluding .git directory
    local find_cmd="$TIMEOUT find -P \"$repo_root\" -name .git -prune -o -type f -name \"*.md\" -print"
    debug_log "Executing find command: $find_cmd"
    # Use a here-string to safely execute the command without word splitting issues
    bash -c "$find_cmd" > "$find_output" 2> "$find_errors"
    local find_status=$?
    
    # Log find errors if any
    if [[ -s "$find_errors" && "$DEBUG" == "true" ]]; then
        debug_log "Find errors: $(cat "$find_errors")"
    fi
    
    # Check if find succeeded
    if [[ $find_status -ne 0 ]]; then
        debug_log "Warning: find command failed or timed out, skipping orphaned files"
        echo "[]" > "$orphaned_data_json"
        return 1
    fi
    
    # Check if find output is empty
    if [[ ! -s "$find_output" ]]; then
        debug_log "Warning: No .md files found in $repo_root"
        echo "[]" > "$orphaned_data_json"
        return 1
    fi
    
    # Process find output and filter out ignored files
    while IFS= read -r file; do
        local abs_file
        abs_file=$(realpath "$file" 2>/dev/null)
        if [[ -n "$abs_file" ]]; then
            local rel_file
            rel_file=$(relative_to_root "$abs_file")
            if [[ -n "$rel_file" ]]; then
                local excluded=false
                for pattern in "${ignore_patterns[@]}"; do
                    shopt -s extglob
                    # Ensure pattern matching works for paths with or without leading slash
                    if [[ "$rel_file" == @($pattern) || "/$rel_file" == @($pattern) ]]; then
                        shopt -u extglob
                        debug_log "Excluding file due to .lintignore pattern '$pattern': $rel_file"
                        excluded=true
                        break
                    fi
                    shopt -u extglob
                done
                if [[ "$excluded" == false ]]; then
                    debug_log "Found .md file: $rel_file (abs: $abs_file)"
                    echo "$rel_file"
                fi
            else
                debug_log "Failed to convert $abs_file to relative path"
            fi
        else
            debug_log "Failed to resolve realpath for $file"
        fi
    done < "$find_output" | sort -u
    
    # Debug checked files
    if [[ "$DEBUG" == "true" ]]; then
        debug_log "Checked files:"
        for abs_file in "${!checked_files[@]}"; do
            debug_log "  $abs_file"
        done
    fi
    
    return 0
}

# Function to generate JSON for reviewed files table with sorting
generate_reviewed_files_json() {
    local json_file="$1"
    local temp_file="$temp_dir/reviewed_temp.json"
    debug_log "Generating reviewed files JSON: $json_file"
    echo "[" > "$temp_file"
    local first=true
    for abs_file in "${!file_summary[@]}"; do
        IFS=',' read -r total checked missing rel_file <<< "${file_summary[$abs_file]}"
        debug_log "Adding file to temp JSON: $rel_file, total=$total, checked=$checked, missing=$missing"
        if [[ "$first" == "true" ]]; then
            first=false
        else
            echo "," >> "$temp_file"
        fi
        # Extract the folder path (excluding the filename)
        folder_path=$(dirname "$rel_file")
        if [[ "$folder_path" == "." ]]; then
            folder_path="."
        fi
        jq -nc --arg file "$rel_file" --arg folder "$folder_path" --arg total "$total" --arg checked "$checked" --arg missing "$missing" \
            '{file: $file, folder: $folder, total: ($total | tonumber), checked: ($checked | tonumber), missing: ($missing | tonumber)}' >> "$temp_file" 2>/dev/null
    done
    echo "]" >> "$temp_file"
    debug_log "Sorting reviewed files JSON"
    # Sort the JSON array by a modified path that inserts 'A' before filenames to prioritize files over folders
    jq -c 'sort_by(.file | split("/") | . as $parts | {path: $parts, modified_path: (if ($parts | length > 0 and ($parts[-1] | test("\\."))) then ($parts[:-1] + ["A" + $parts[-1]]) else $parts end | join("/"))} | .modified_path)' "$temp_file" > "$json_file"
    debug_log "Reviewed files JSON generated and sorted"
    [[ "$DEBUG" == "true" ]] && debug_log "JSON content: $(cat "$json_file")"
}

# Function to generate JSON for missing links table with sorting
generate_missing_links_json() {
    local json_file="$1"
    local temp_file="$temp_dir/missing_temp.json"
    debug_log "Generating missing links JSON: $json_file"
    echo "[" > "$temp_file"
    local first=true
    for entry in "${missing_links[@]}"; do
        IFS=':' read -r file link <<< "$entry"
        debug_log "Adding missing link to temp JSON: file=$file, link=$link"
        if [[ "$first" == "true" ]]; then
            first=false
        else
            echo "," >> "$temp_file"
        fi
        jq -nc --arg file "$file" --arg link "$link" '{file: $file, link: $link}' >> "$temp_file" 2>/dev/null
    done
    echo "]" >> "$temp_file"
    debug_log "Sorting missing links JSON"
    # Sort the JSON array by a modified path that inserts 'A' before filenames to prioritize files over folders
    jq -c 'sort_by(.file | split("/") | . as $parts | {path: $parts, modified_path: (if ($parts | length > 0 and ($parts[-1] | test("\\."))) then ($parts[:-1] + ["A" + $parts[-1]]) else $parts end | join("/"))} | .modified_path)' "$temp_file" > "$json_file"
    debug_log "Missing links JSON generated and sorted"
    [[ "$DEBUG" == "true" ]] && debug_log "JSON content: $(cat "$json_file")"
}

# Function to generate JSON for orphaned files table with sorting
generate_orphaned_files_json() {
    local json_file="$1"
    local temp_file="$temp_dir/orphaned_temp.json"
    debug_log "Generating orphaned files JSON: $json_file"
    echo "[" > "$temp_file"
    local first=true
    while IFS= read -r rel_file; do
        local abs_file
        abs_file=$(resolve_path "$repo_root" "$rel_file")
        debug_log "Checking if $rel_file (abs: $abs_file) is orphaned"
        if [[ -z "${checked_files[$abs_file]}" && -z "${linked_files[$abs_file]}" ]]; then
            debug_log "Found orphaned file: $rel_file (not checked and not linked)"
            orphaned_files+=("$rel_file")
            if [[ "$first" == "true" ]]; then
                first=false
            else
                echo "," >> "$temp_file"
            fi
            jq -nc --arg file "$rel_file" '{file: $file}' >> "$temp_file" 2>/dev/null
        else
            if [[ -n "${checked_files[$abs_file]}" ]]; then
                debug_log "$rel_file is checked, not orphaned"
            fi
            if [[ -n "${linked_files[$abs_file]}" ]]; then
                debug_log "$rel_file is linked to, not orphaned"
            fi
        fi
    done < <(find_all_md_files)
    echo "]" >> "$temp_file"
    debug_log "Sorting orphaned files JSON"
    # Sort the JSON array by a modified path that inserts 'A' before filenames to prioritize files over folders
    jq -c 'sort_by(.file | split("/") | . as $parts | {path: $parts, modified_path: (if ($parts | length > 0 and ($parts[-1] | test("\\."))) then ($parts[:-1] + ["A" + $parts[-1]]) else $parts end | join("/"))} | .modified_path)' "$temp_file" > "$json_file"
    debug_log "Orphaned files JSON generated and sorted"
    [[ "$DEBUG" == "true" ]] && debug_log "JSON content: $(cat "$json_file")"
}

# Function to generate layout JSON for reviewed files table
generate_reviewed_layout_json() {
    local json_file="$1"
    debug_log "Generating reviewed layout JSON: $json_file"
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
    debug_log "Reviewed layout JSON generated"
    [[ "$DEBUG" == "true" ]] && debug_log "JSON content: $(cat "$json_file")"
}

# Function to generate layout JSON for missing links table
generate_missing_layout_json() {
    local json_file="$1"
    debug_log "Generating missing layout JSON: $json_file"
    jq -n --arg theme "$TABLE_THEME" '{
        theme: $theme,
        title: "Missing Links",
        columns: [
            {header: "File", key: "file", datatype: "text", "summary":"count"},
            {header: "Missing Link", key: "link", datatype: "text" }
        ]
    }' > "$json_file" 2>/dev/null
    debug_log "Missing layout JSON generated"
    [[ "$DEBUG" == "true" ]] && debug_log "JSON content: $(cat "$json_file")"
}

# Function to generate layout JSON for orphaned files table
generate_orphaned_layout_json() {
    local json_file="$1"
    debug_log "Generating orphaned layout JSON: $json_file"
    jq -n --arg theme "$TABLE_THEME" '{
        theme: $theme,
        title: "Orphaned Markdown Files",
        columns: [
            {header: "File", key: "file", datatype: "text", "summary":"count"}
        ]
    }' > "$json_file" 2>/dev/null
    debug_log "Orphaned layout JSON generated"
    [[ "$DEBUG" == "true" ]] && debug_log "JSON content: $(cat "$json_file")"
}

# Clear or create output file unless noreport is enabled
if [[ "$NOREPORT" != "true" ]]; then
    debug_log "Clearing output file: $output_file"
    : > "$output_file"
else
    debug_log "Report generation skipped due to --noreport option"
fi

# Process queue
debug_log "Starting queue processing with ${#queue[@]} files"
while [ ${#queue[@]} -gt 0 ]; do
    current_file="${queue[0]}"
    queue=("${queue[@]:1}")
    debug_log "Queue size: ${#queue[@]}, processing: $current_file"
    process_file "$current_file"
done
debug_log "Queue processing complete"

# Write summary and orphaned files to output file if noreport is not enabled
if [[ "$NOREPORT" != "true" ]]; then
    debug_log "Writing summary to $output_file"
    {
        echo -e "\n=== Link Check Summary ==="
        for abs_file in "${!file_summary[@]}"; do
            IFS=',' read -r total checked missing rel_file <<< "${file_summary[$abs_file]}"
            echo "File: $rel_file"
            echo "  Total Links: $total"
            echo "  Checked Links: $checked"
            echo "  Missing Links: $missing"
            echo ""
            debug_log "Wrote summary for $rel_file"
        done
        
        debug_log "Writing orphaned files to $output_file"
        echo -e "\n=== Orphaned Markdown Files ==="
        if [[ ${#orphaned_files[@]} -gt 0 ]]; then
            for file in "${orphaned_files[@]}"; do
                echo "File: $file"
            done
        else
            echo "No orphaned files found"
        fi
        echo ""
    } >> "$output_file"
else
    debug_log "Skipping writing summary and orphaned files due to --noreport option"
fi

# Generate temporary JSON files
debug_log "Creating temporary directory"
temp_dir=$(mktemp -d 2>/dev/null) || { echo "Error: Failed to create temporary directory" >&2; exit 1; }
debug_log "Temporary directory: $temp_dir"
reviewed_data_json="$temp_dir/reviewed_data.json"
reviewed_layout_json="$temp_dir/reviewed_layout.json"
missing_data_json="$temp_dir/missing_data.json"
missing_layout_json="$temp_dir/missing_layout.json"
orphaned_data_json="$temp_dir/orphaned_data.json"
orphaned_layout_json="$temp_dir/orphaned_layout.json"

# Generate JSON data and layouts
generate_reviewed_files_json "$reviewed_data_json"
generate_reviewed_layout_json "$reviewed_layout_json"
generate_missing_links_json "$missing_data_json"
generate_missing_layout_json "$missing_layout_json"
generate_orphaned_files_json "$orphaned_data_json"
generate_orphaned_layout_json "$orphaned_layout_json"

# Print version and issue count to stdout unless quiet mode is enabled
if [[ "$QUIET" != "true" ]]; then
    echo "Markdown Link Checker v$APPVERSION"
    echo "Issues found: $missing_count"
    echo ""
fi

# Render tables to stdout, redirecting tables.sh debug output to a separate file
[[ "$DEBUG" == "true" ]] && debug_flag="--debug" || debug_flag=""
tables_debug_log="$temp_dir/tables_debug.log"
debug_log "Tables.sh debug output will be written to $tables_debug_log"

debug_log "Rendering reviewed files table"
bash "$TABLES_SCRIPT" "$reviewed_layout_json" "$reviewed_data_json" ${debug_flag:+"$debug_flag"} 2>> "$tables_debug_log"
if [[ ${#missing_links[@]} -gt 0 ]]; then
    debug_log "Rendering missing links table"
    bash "$TABLES_SCRIPT" "$missing_layout_json" "$missing_data_json" ${debug_flag:+"$debug_flag"} 2>> "$tables_debug_log"
fi
if [[ ${#orphaned_files[@]} -gt 0 ]]; then
    debug_log "Rendering orphaned files table"
    bash "$TABLES_SCRIPT" "$orphaned_layout_json" "$orphaned_data_json" ${debug_flag:+"$debug_flag"} 2>> "$tables_debug_log"
else
    debug_log "No orphaned files, skipping table rendering"
fi

# Log tables.sh debug output if any
if [[ -s "$tables_debug_log" && "$DEBUG" == "true" ]]; then
    debug_log "Tables.sh debug output: $(cat "$tables_debug_log")"
fi

# Clean up temporary files
debug_log "Cleaning up temporary directory: $temp_dir"
rm -rf "$temp_dir" 2>/dev/null

# Final message unless quiet mode is enabled
if [[ "$QUIET" != "true" ]]; then
    echo ""
    if [[ "$NOREPORT" != "true" ]]; then
        echo "Link check complete. Detailed report written to $output_file"
    else
        echo "Link check complete. Report generation skipped due to --noreport option"
    fi
fi
# Calculate total issues for return code
total_issues=$((missing_count + ${#orphaned_files[@]}))
debug_log "Script completed with $total_issues issues (missing: $missing_count, orphaned: ${#orphaned_files[@]})"
exit $total_issues
