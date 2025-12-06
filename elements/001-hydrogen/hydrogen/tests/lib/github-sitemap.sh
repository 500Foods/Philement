#!/usr/bin/env bash

# Markdown link checker script - xargs Parallel Processing Version
# Usage: ./github-sitemap-xargs.sh <markdown_file> [--debug] [--theme <Red|Blue>]
# Description: Checks local markdown links in a repository using xargs parallel processing

# CHANGELOG
# 1.4.4 - 2025-10-01 - Restored parallel processing with optimal batching (8 workers, 40 files/batch)
# 1.4.3 - 2025-10-01 - Fixed find to prune major directories: build/ (~46k), node_modules (~2k), unity/framework (~500 files)
# 1.4.2 - 2025-10-01 - Attempted xargs fix (but accidentally broke parallelization)
# 1.4.1 - 2025-10-01 - Added pure bash path normalization to eliminate all realpath subprocess calls
# 1.4.0 - 2025-10-01 - Eliminated double-pass link processing (50% speedup), optimized path normalization
# 1.3.0 - 2025-08-08 - Removed cksum deduplication (added overhead for repositories with few duplicates)
# 1.2.0 - 2025-08-08 - Added cksum-based within-run deduplication (10x faster than md5sum)
# 1.1.0 - 2025-08-08 - Fork reduction optimizations: replaced wc -l and dirname with bash builtins
# 0.9.0 - 2025-08-08 - Single-pass awk parsing optimization (replaced grep|sed|awk pipelines)
# 0.8.0 - 2025-08-08 - Performance optimizations following md5sum batching pattern from test_92_shellcheck.sh
# 0.7.1 - 2025-08-03 - Removed extraneous command -v calls

set -euo pipefail

# Application version
declare -r APPVERSION="1.4.4"

# Global variables
declare -a global_ignore_patterns=()
declare -A checked_files=()

# Common utilities - use GNU versions if available (eg: homebrew on macOS)
PRINTF=$(command -v gprintf 2>/dev/null || command -v printf)
DATE=$(command -v date) 
XARGS=$(command -v gxargs 2>/dev/null || command -v xargs)
FIND=$(command -v gfind 2>/dev/null || command -v find)
AWK=$(command -v gawk 2>/dev/null || command -v awk)
REALPATH=$(command -v grealpath 2>/dev/null || command -v realpath)
DIRNAME=$(command -v gdirname 2>/dev/null || command -v dirname)
export PRINTF DATE XARGS FIND AWK REALPATH DIRNAME

# Parse arguments
DEBUG="false"
INPUT_FILE=""
IGNORE_FILE=""
TABLE_THEME="Red"
QUIET=false
NOREPORT=false
HELP=false
WEBROOT=""
declare -a INCLUDE_DIRS=()

while [[ $# -gt 0 ]]; do
    case "$1" in
        --debug) DEBUG=true; shift ;;
        --quiet) QUIET=true; shift ;;
        --noreport) NOREPORT=true; shift ;;
        --help) HELP=true; shift ;;
        --ignore)
            if [[ -n "$2" ]]; then
                IGNORE_FILE="$2"
                shift 2
            else
                echo "Error: --ignore requires a file path" >&2
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
        --webroot)
            if [[ -n "$2" ]]; then
                WEBROOT="$2"
                shift 2
            else
                echo "Error: --webroot requires a path" >&2
                exit 1
            fi
            ;;
        --include)
            if [[ -n "$2" ]]; then
                INCLUDE_DIRS+=("$2")
                shift 2
            else
                echo "Error: --include requires a directory path" >&2
                exit 1
            fi
            ;;
        *) if [[ -z "${INPUT_FILE}" ]]; then INPUT_FILE="$1"; shift; else echo "Error: Unknown option: $1" >&2; exit 1; fi ;;
    esac
done

# Check if help is requested
if [[ "${HELP}" == "true" ]]; then
    echo "Usage: $0 <markdown_file> [--debug] [--quiet] [--noreport] [--help] [--theme <Red|Blue>] [--ignore <file>] [--webroot <path>] [--include <dir>]..." || true
    echo "Options:"
    echo "  --debug      Enable debug logging"
    echo "  --quiet      Display only tables, suppress other output"
    echo "  --noreport   Do not create a report file"
    echo "  --help       Display this help message"
    echo "  --theme      Set table theme to 'Red' or 'Blue' (default: Red)"
    echo "  --ignore     Specify a file with ignore patterns (like .lintignore)"
    echo "  --webroot    Set the web root path for absolute link resolution"
    echo "  --include    Include directory in search (can be specified multiple times)"
    exit 0
fi

# Check if a file parameter is provided
if [[ -z "${INPUT_FILE}" ]]; then
    echo "Usage: $0 <markdown_file> [options]" >&2
    exit 1
fi

# Debug logging function
debug_log() {
    if [[ "${DEBUG}" == "true" ]]; then
        echo "[DEBUG] $("${DATE}" '+%Y-%m-%d %H:%M:%S' || true): $*" >&2
    fi
}

# Check dependencies
TABLES=$(command -v tables)
if [[ ! -f "${TABLES}" ]]; then
    echo "Error: ${TABLES} not found" >&2
    exit 1
fi

# Initialize variables
declare -A file_exists_cache
declare -A file_summary
declare -a missing_links=()
declare -a orphaned_files=()
missing_count=0

# Store the original working directory
declare original_dir
original_dir=$(pwd)
output_file="${original_dir}/sitemap_report.txt"

# Set repo root based on input file's directory
declare input_dir
if [[ "${INPUT_FILE}" == /* ]]; then
    input_dir=$("${DIRNAME}" "${INPUT_FILE}")
else
    input_dir=$(cd "${original_dir}" && "${REALPATH}" -m "$("${DIRNAME}" "${INPUT_FILE}" || true)" 2>/dev/null)
fi
if [[ -z "${input_dir}" ]]; then
    echo "Error: Cannot resolve input file directory for ${INPUT_FILE}" >&2
    exit 1
fi
if ! cd "${input_dir}" 2>/dev/null; then
    echo "Error: Cannot change to input file directory ${input_dir} for ${INPUT_FILE}" >&2
    exit 1
fi
repo_root="${input_dir}"
if [[ -n "${WEBROOT}" ]]; then
    repo_root="${WEBROOT}"
elif [[ -n "${PHILEMENT_ROOT:-}" ]]; then
    repo_root="${PHILEMENT_ROOT}"
fi
debug_log "Set repo root to ${repo_root} for input file ${INPUT_FILE}"

# Load ignore patterns once and export for parallel processes
load_ignore_patterns() {
    debug_log "Loading ignore patterns"
    
    # Read ignore patterns from specified ignore file or default to .lintignore in same dir as input file
    local ignore_file=""
    if [[ -n "${IGNORE_FILE}" ]]; then
        if [[ "${IGNORE_FILE}" == /* ]]; then
            ignore_file="${IGNORE_FILE}"
        else
            ignore_file="${original_dir}/${IGNORE_FILE}"
        fi
    else
        # Default to .lintignore in the same directory as the input markdown file
        ignore_file="${repo_root}/.lintignore"
    fi
    
    if [[ -f "${ignore_file}" ]]; then
        debug_log "Reading ignore patterns from ${ignore_file}"
        while IFS= read -r line; do
            # Skip empty lines and comments
            if [[ -n "${line}" && ! "${line}" =~ ^[[:space:]]*# ]]; then
                global_ignore_patterns+=("${line}")
                debug_log "Added ignore pattern: ${line}"
            fi
        done < "${ignore_file}"
    else
        debug_log "No ignore file found at ${ignore_file}"
    fi
    
    # Export patterns as delimited string for parallel processes
    local IFS='|'
    export IGNORE_PATTERNS_SERIALIZED="${global_ignore_patterns[*]}"
    debug_log "Exported ${#global_ignore_patterns[@]} ignore patterns"
}

load_ignore_patterns

# Build file existence cache with optimized find
build_file_cache() {
    debug_log "Building file existence cache"
    local cache_count=0

    # Determine find paths
    local find_paths=()
    if [[ ${#INCLUDE_DIRS[@]} -gt 0 ]]; then
        for dir in "${INCLUDE_DIRS[@]}"; do
            if [[ "${dir}" == /* ]]; then
                find_paths+=("${dir}")
            else
                find_paths+=("${repo_root}/${dir}")
            fi
        done
        debug_log "Using include directories: ${find_paths[*]}"
    elif [[ -n "${HYDROGEN_ROOT:-}" && -n "${HYDROGEN_DOCS_ROOT:-}" ]]; then
        find_paths=("${HYDROGEN_ROOT}" "${HYDROGEN_DOCS_ROOT}")
        debug_log "Using specific roots: ${HYDROGEN_ROOT} and ${HYDROGEN_DOCS_ROOT}"
    else
        find_paths=("${repo_root}")
        debug_log "Using repo root: ${repo_root}"
    fi

    # Single find command with optimized output
    while IFS= read -r -d '' entry; do
        local path="${entry%:*}"
        local type="${entry#*:}"
        local rel_path=""
        if [[ "${path}" == "${repo_root}"* ]]; then
            rel_path="${path#"${repo_root}"/}"
        else
            rel_path="${path}"
        fi

        local excluded=false
        for pattern in "${global_ignore_patterns[@]}"; do
            shopt -s extglob
            if [[ "${rel_path}" == @(${pattern}) || "/${rel_path}" == @(${pattern}) ]]; then
                shopt -u extglob
                debug_log "Excluding file due to ignore pattern '${pattern}': ${rel_path}"
                excluded=true
                break
            fi
            shopt -u extglob
        done

        if [[ "${excluded}" == false ]]; then
            file_exists_cache["${path}"]="${type}"
            cache_count=$(( cache_count + 1 ))
        fi
    done < <("${FIND}" "${find_paths[@]}" \( -name .git -o -name build -o -name node_modules -o -path '*/tests/logs' -o -path '*/tests/results' -o -path '*/tests/diagnostics' -o -path '*/tests/unity/framework' \) -prune -o \( -type f -printf '%p:f\0' \) -o \( -type d -printf '%p:d\0' \) 2>/dev/null || true)

    debug_log "Built file existence cache with ${cache_count} entries"
}

build_file_cache

# Optimized file processing function for xargs
# shellcheck disable=SC2317 # This function is designed to be run in parallel with xargs, so we disable SC2317
process_file_batch() {
    local cache_file="$1"
    local repo_root="$2"
    local original_dir="$3"
    local results_file="$4"
    
    # Load cache once per batch
    declare -A local_cache
    declare -A processed_files
    while IFS=':' read -r path type; do
        local_cache["${path}"]="${type}"
    done < "${cache_file}"
    
    
    # Fast file existence check
    file_exists() {
        local path="$1"
        # Check exact path first
        if [[ -n "${local_cache[${path}]}" ]]; then
            return 0
        fi
        
        # If path ends with /, try without the trailing slash (for directory links)
        if [[ "${path}" == */ ]]; then
            local path_no_slash="${path%/}"
            [[ -n "${local_cache[${path_no_slash}]}" ]] && return 0
        fi
        
        # If path doesn't end with /, try with trailing slash (for directory links)
        if [[ "${path}" != */ ]]; then
            local path_with_slash="${path}/"
            [[ -n "${local_cache[${path_with_slash}]}" ]] && return 0
        fi
        
        return 1
    }
    
    # Check if a path should be ignored based on patterns
    should_ignore_path() {
        local path="$1"
        local rel_path=""
        if [[ "${path}" == "${repo_root}"* ]]; then
            rel_path="${path#"${repo_root}"/}"
        else
            rel_path="${path}"
        fi
        
        # Use centralized ignore patterns from environment variable
        if [[ -n "${IGNORE_PATTERNS_SERIALIZED}" ]]; then
            # Split the serialized patterns properly
            local patterns_string="${IGNORE_PATTERNS_SERIALIZED}"
            local pattern
            shopt -s extglob
            while IFS='|' read -r -d '|' pattern || [[ -n "${pattern}" ]]; do
                [[ -z "${pattern}" ]] && continue
                if [[ "${rel_path}" == @(${pattern}) || "/${rel_path}" == @(${pattern}) || "${rel_path}/" == @(${pattern}) || "/${rel_path}/" == @(${pattern}) ]]; then
                    return 0  # Should ignore
                fi
            done <<< "${patterns_string}|"
            shopt -u extglob
        fi
        
        return 1  # Should not ignore
    }
    
    # Pure bash path normalization - eliminates realpath subprocess calls
    normalize_path() {
        local path="$1"
        
        # Handle absolute paths
        local prefix=""
        if [[ "${path}" == /* ]]; then
            prefix="/"
            path="${path#/}"
        fi
        
        # Split into parts and normalize
        local -a parts=()
        local IFS='/'
        for part in ${path}; do
            case "${part}" in
                ""|".") continue ;;
                "..")
                    if [[ ${#parts[@]} -gt 0 ]]; then
                        unset 'parts[-1]'
                    fi
                    ;;
                *) parts+=("${part}") ;;
            esac
        done
        
        # Rebuild path
        if [[ ${#parts[@]} -eq 0 ]]; then
            echo "${prefix:-.}"
        else
            local IFS='/'
            echo "${prefix}${parts[*]}"
        fi
    }
    
    # Optimized link extraction - single awk command instead of grep|sed|awk pipeline
    extract_links() {
        local file="$1"
        [[ ! -r "${file}" ]] && return 1
        
        # Single awk command optimization (replaces grep | sed | awk pipeline)
        AWK=$(command -v gawk 2>/dev/null || command -v awk)
        # shellcheck disable=SC2016 # Awk command uses single quotes on purpose to avoid escaping issues
        "${AWK}" '
        {
            # Extract all markdown links from the line
            while (match($0, /\[([^]]*)\]\(([^)]+)\)/, arr)) {
                link = arr[2]
                
                # Skip external links
                if (link ~ /^(#|http|ftp|mailto:)/) {
                    $0 = substr($0, RSTART + RLENGTH)
                    continue
                }
                
                # Process unique links only
                if (!seen[link]++) {
                    # Process GitHub and local links
                    if (match(link, /^https:\/\/github\.com\/[^\/]+\/[^\/]+\/blob\/main\/(.+)$/, github_arr)) {
                        relative_path = github_arr[1]
                        if (relative_path ~ /\.md($|#)/) {
                            print relative_path
                        } else {
                            print "non_md:" relative_path
                        }
                    } else {
                        if (link ~ /\.md($|#)/) {
                            print link
                        } else {
                            print "non_md:" link
                        }
                    }
                }
                
                # Continue processing rest of line
                $0 = substr($0, RSTART + RLENGTH)
            }
        }
        ' "${file}" 2>/dev/null || true
    }
    
    # Process each file from stdin
    while IFS= read -r file; do
        [[ -z "${file}" ]] && continue
        
        # Resolve absolute path efficiently
        local abs_file
        if [[ "${file}" == /* ]]; then
            abs_file="${file}"
        else
            abs_file=$(cd "${original_dir}" && cd "$("${DIRNAME}" "${file}" || true)" && pwd)
            abs_file="${abs_file}/$(basename "${file}")"
        fi
        
        # Skip if file doesn't exist
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        file_exists "${abs_file}" || continue
        
        local base_dir
        base_dir=$("${DIRNAME}" "${abs_file}")
        local rel_file
        if [[ "${abs_file}" == "${repo_root}"* ]]; then
            rel_file="${abs_file#"${repo_root}"/}"
        else
            rel_file="${abs_file}"
        fi
        
        # Skip if file is already processed
        if [[ -n "${processed_files[${abs_file}]}" ]]; then
            debug_log "File already processed, skipping: ${rel_file}"
            continue
        fi
        
        local links_total=0 links_checked=0 links_missing=0
        local linked_files=()
        local missing_list=()
        
        # Single-pass link processing with inline normalization
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        while IFS= read -r link; do
            links_total=$(( links_total + 1 ))
            local actual_link="${link}"
            local traverse=true
            
            if [[ "${link}" =~ ^non_md:(.+)$ ]]; then
                actual_link="${BASH_REMATCH[1]}"
                traverse=false
            fi
            
            # Resolve link path with optimized logic
            local abs_link
            if [[ "${actual_link}" =~ ^https://github.com/[^/]+/[^/]+/blob/main/(.+)$ ]]; then
                abs_link="${repo_root}/${BASH_REMATCH[1]}"
            elif [[ "${actual_link}" == /* ]]; then
                abs_link="${repo_root}${actual_link}"
            else
                abs_link="${base_dir}/${actual_link}"
            fi
            
            # Normalize inline only if needed (pure bash - no subprocess calls)
            case "${abs_link}" in
                */./*|*/../*)
                    abs_link=$(normalize_path "${abs_link}")
                    ;;
                *)
                    # Path doesn't need normalization
                    ;;
            esac
            
            local link_file="${abs_link%%#*}"
            
            # shellcheck disable=SC2310 # We want to continue even if the test fails
            if file_exists "${link_file}"; then
                links_checked=$(( links_checked + 1 ))
                if [[ "${traverse}" == "true" && "${link_file}" =~ \.md$ ]]; then
                    linked_files+=("${link_file}")
                fi
            else
                # Check if the missing link should be ignored
                # shellcheck disable=SC2310 # We want to continue even if the test fails
                if should_ignore_path "${link_file}"; then
                    # Treat ignored missing links as checked to avoid reporting them
                    links_checked=$(( links_checked + 1 ))
                else
                    links_missing=$(( links_missing + 1 ))
                    missing_list+=("${rel_file}:${actual_link}")
                fi
            fi
        done < <(extract_links "${abs_file}" || true)
        
        # Output results in structured format (batch write - fork reduction)
        {
            echo "SUMMARY:${abs_file}:${links_total}:${links_checked}:${links_missing}:${rel_file}"
            for missing in "${missing_list[@]}"; do
                echo "MISSING:${missing}"
            done
            for linked in "${linked_files[@]}"; do
                echo "LINKED:${linked}"
            done
        } >> "${results_file}"
        
    done
}

export -f process_file_batch

# Clear output file if needed
if [[ "${NOREPORT}" != "true" ]]; then
    true > "${output_file}"
fi

# Create temporary files for parallel processing
temp_dir=$(mktemp -d) || { echo "Error: Failed to create temporary directory" >&2; exit 1; }
cache_file="${temp_dir}/file_cache.txt"
results_file="${temp_dir}/results.txt"
files_to_process="${temp_dir}/files_list.txt"

# Initialize results file
true > "${results_file}"

# Export cache to file (batch write - fork reduction)
{
    for path in "${!file_exists_cache[@]}"; do
        echo "${path}:${file_exists_cache[${path}]}"
    done
} > "${cache_file}"

debug_log "Parallel setup complete"

# Process files in parallel using xargs

# Start with input file and iteratively discover linked files
declare -A processed_files
echo "${INPUT_FILE}" > "${files_to_process}"
processed_files["${INPUT_FILE}"]=1

iteration=0
while [[ -s "${files_to_process}" ]]; do
    iteration=$(( iteration + 1 ))
    # Use bash builtin counting instead of wc -l (fork reduction)
    file_count=0
    while IFS= read -r; do file_count=$(( file_count + 1 )); done < "${files_to_process}"
    debug_log "Processing iteration ${iteration} with ${file_count} files"
    
    # Process files in parallel batches (split into groups for parallel processing)
    # shellcheck disable=SC2016 # Single quotes intentional - variables passed as positional params
    "${XARGS}" -P 8 -n 40 bash -c '
        for file; do
            echo "$file"
        done | process_file_batch "$1" "$2" "$3" "$4"
    ' _ "${cache_file}" "${repo_root}" "${original_dir}" "${results_file}" < "${files_to_process}"
    # Extract newly discovered files from results
    new_files_temp="${temp_dir}/new_files_${iteration}.txt"
    true > "${new_files_temp}"
    
    while IFS= read -r line; do
        if [[ "${line}" =~ ^LINKED:(.+)$ ]]; then
            linked_file="${BASH_REMATCH[1]}"
            if [[ -z "${processed_files[${linked_file}]:-}" ]]; then
                processed_files["${linked_file}"]=1
                echo "${linked_file}" >> "${new_files_temp}"
            fi
        fi
    done < "${results_file}"
    
    # Update files to process for next iteration
    if [[ -s "${new_files_temp}" ]]; then
        mv "${new_files_temp}" "${files_to_process}"
        # Use bash builtin counting instead of wc -l (fork reduction)
        new_file_count=0
        while IFS= read -r; do new_file_count=$(( new_file_count + 1 )); done < "${files_to_process}"
        debug_log "Discovered ${new_file_count} new files"
    else
        true > "${files_to_process}"
        debug_log "No new files discovered, processing complete"
    fi
done


# Process results
debug_log "Processing parallel results"

while IFS= read -r line; do
    if [[ "${line}" =~ ^SUMMARY:([^:]+):([^:]+):([^:]+):([^:]+):(.+)$ ]]; then
        abs_file="${BASH_REMATCH[1]}"
        total="${BASH_REMATCH[2]}"
        checked="${BASH_REMATCH[3]}"
        missing="${BASH_REMATCH[4]}"
        rel_file="${BASH_REMATCH[5]}"
        
        file_summary["${abs_file}"]="${total},${checked},${missing},${rel_file}"
        checked_files["${abs_file}"]=1
        missing_count=$(( missing_count + missing ))
        
    elif [[ "${line}" =~ ^MISSING:(.+)$ ]]; then
        missing_links+=("${BASH_REMATCH[1]}")
        if [[ "${NOREPORT}" != "true" ]]; then
            IFS=':' read -r file link <<< "${BASH_REMATCH[1]}"
            echo "Missing link in ${file}: ${link}" >> "${output_file}"
        fi
    fi
done < "${results_file}"

# Find orphaned files
for path in "${!file_exists_cache[@]}"; do
    if [[ "${path}" == *.md && "${file_exists_cache[${path}]}" == "f" ]]; then
        if [[ -z "${checked_files[${path}]:-}" ]]; then
            rel_file=""
            if [[ "${path}" == "${repo_root}"* ]]; then
                rel_file="${path#"${repo_root}"/}"
            else
                rel_file="${path}"
            fi
            orphaned_files+=("${rel_file}")
        fi
    fi
done

# Generate JSON and render tables

# Generate JSON functions (optimized)
generate_reviewed_files_json() {
    local json_file="$1"
    local json_data=""
    for abs_file in "${!file_summary[@]}"; do
        IFS=',' read -r total checked missing rel_file <<< "${file_summary[${abs_file}]}"
        # Use bash parameter expansion instead of dirname (fork reduction)
        folder_path="${rel_file%/*}"
        [[ "${folder_path}" == "." ]] && folder_path="."
        
        if [[ -n "${json_data}" ]]; then
            json_data+=","
        fi
        json_data+="{\"file\":\"${rel_file}\",\"folder\":\"${folder_path}\",\"total\":${total},\"checked\":${checked},\"missing\":${missing}}"
    done
    
    echo "[${json_data}]" | jq -c 'sort_by(.file | split("/") | . as $parts | {path: $parts, modified_path: (if ($parts | length > 0 and ($parts[-1] | test("\\."))) then ($parts[:-1] + ["A" + $parts[-1]]) else $parts end | join("/"))} | .modified_path)' > "${json_file}"
}

generate_missing_links_json() {
    local json_file="$1"
    local json_data=""
    for entry in "${missing_links[@]}"; do
        IFS=':' read -r file link <<< "${entry}"
        if [[ -n "${json_data}" ]]; then
            json_data+=","
        fi
        link_escaped="${link//\"/\\\"}"
        json_data+="{\"file\":\"${file}\",\"link\":\"${link_escaped}\"}"
    done
    
    echo "[${json_data}]" | jq -c 'sort_by(.file)' > "${json_file}"
}

generate_orphaned_files_json() {
    local json_file="$1"
    local json_data=""
    for rel_file in "${orphaned_files[@]}"; do
        if [[ -n "${json_data}" ]]; then
            json_data+=","
        fi
        json_data+="{\"file\":\"${rel_file}\"}"
    done
    
    echo "[${json_data}]" | jq -c 'sort_by(.file)' > "${json_file}"
}

# Layout generation functions
generate_reviewed_layout_json() {
    local json_file="$1"
    jq -n --arg theme "${TABLE_THEME}" '{
        theme: $theme,
        title: "Reviewed Files Summary",
        columns: [
            {header: "File", key: "file", datatype: "text", "summary":"count"},
            {header: "Folder", key: "folder", datatype: "text", "visible": false, "break": true},
            {header: "Total Links", key: "total", datatype: "int", justification: "right", "summary":"sum"},
            {header: "Found Links", key: "checked", datatype: "int", justification: "right", "summary":"sum"},
            {header: "Missing Links", key: "missing", datatype: "int", justification: "right","summary":"sum"}
        ]
    }' > "${json_file}" 2>/dev/null
}

generate_missing_layout_json() {
    local json_file="$1"
    jq -n --arg theme "${TABLE_THEME}" '{
        theme: $theme,
        title: "Missing Links",
        columns: [
            {header: "File", key: "file", datatype: "text", "summary":"count"},
            {header: "Missing Link", key: "link", datatype: "text" }
        ]
    }' > "${json_file}" 2>/dev/null
}

generate_orphaned_layout_json() {
    local json_file="$1"
    jq -n --arg theme "${TABLE_THEME}" '{
        theme: $theme,
        title: "Orphaned Markdown Files",
        columns: [
            {header: "File", key: "file", datatype: "text", "summary":"count"}
        ]
    }' > "${json_file}" 2>/dev/null
}

# Generate JSON files (optimized temp file usage)
reviewed_data_json="${temp_dir}/reviewed_data.json"
reviewed_layout_json="${temp_dir}/reviewed_layout.json"

# Always generate reviewed files JSON (main table)
generate_reviewed_files_json "${reviewed_data_json}"
generate_reviewed_layout_json "${reviewed_layout_json}"

# Generate missing links JSON only if needed
if [[ -v missing_links && ${#missing_links[@]} -gt 0 ]]; then
    missing_data_json="${temp_dir}/missing_data.json"
    missing_layout_json="${temp_dir}/missing_layout.json"
    generate_missing_links_json "${missing_data_json}"
    generate_missing_layout_json "${missing_layout_json}"
fi

# Generate orphaned files JSON only if needed
if [[ -v orphaned_files && ${#orphaned_files[@]} -gt 0 ]]; then
    orphaned_data_json="${temp_dir}/orphaned_data.json"
    orphaned_layout_json="${temp_dir}/orphaned_layout.json"
    generate_orphaned_files_json "${orphaned_data_json}"
    generate_orphaned_layout_json "${orphaned_layout_json}"
fi


# Output results
if [[ "${QUIET}" != "true" ]]; then
    echo "Markdown Link Checker v${APPVERSION}"
    echo "Issues found: ${missing_count}"
    echo ""
fi

# Render tables
[[ "${DEBUG}" == "true" ]] && debug_flag="--debug" || debug_flag=""

"${TABLES}" "${reviewed_layout_json}" "${reviewed_data_json}" ${debug_flag:+"${debug_flag}"}
if [[ ${#missing_links[@]} -gt 0 && -n "${missing_data_json:-}" ]]; then
    "${TABLES}" "${missing_layout_json}" "${missing_data_json}" ${debug_flag:+"${debug_flag}"}
fi
if [[ ${#orphaned_files[@]} -gt 0 && -n "${orphaned_data_json:-}" ]]; then
    "${TABLES}" "${orphaned_layout_json}" "${orphaned_data_json}" ${debug_flag:+"${debug_flag}"}
fi

# Write summary to report file
if [[ "${NOREPORT}" != "true" ]]; then
    {
        echo -e "\n=== Link Check Summary ==="
        echo "Files processed: ${#checked_files[@]}"
        echo "Total missing links: ${missing_count}"
        echo "Orphaned files: ${#orphaned_files[@]}"
        echo ""
        
        if [[ ${#orphaned_files[@]} -gt 0 ]]; then
            echo "=== Orphaned Markdown Files ==="
            for file in "${orphaned_files[@]}"; do
                echo "File: ${file}"
            done
            echo ""
        fi
    } >> "${output_file}"
fi

# Clean up
rm -rf "${temp_dir}" 2>/dev/null

# Final message
if [[ "${QUIET}" != "true" ]]; then
    echo ""
    if [[ "${NOREPORT}" != "true" ]]; then
        echo "Link check complete. Detailed report written to ${output_file}"
    else
        echo "Link check complete. Report generation skipped due to --noreport option"
    fi
fi

# Exit with issue count
total_issues=$((missing_count + ${#orphaned_files[@]}))
exit "${total_issues}"
