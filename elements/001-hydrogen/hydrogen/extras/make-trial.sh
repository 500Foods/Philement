#!/usr/bin/env bash

# CHANGELOG
# 2026-08-30: Trial Build Complete now reports total elapsed build time (e.g. "Trial Build Complete (2m 26s)")
# 2026-08-30: Replaced cppcheck-based dead-code analysis with a linker-based check: builds the "deadcode" CMake target (-O0, no LTO, hydrogen_release-shaped --gc-sections/--dynamic-list/--print-gc-sections link) and reports functions unreachable from main(), filtered to global symbols in our own objects and tests/.deadcode-baseline.txt; output condensed to 3 lines (count checked, count found, path to full list) with the list written to build/deadcode/dead_functions.txt instead of printed inline
# 2026-08-30: Fixed dead-code cppcheck config parsing: include= now maps to -I (directory paths) not --include= (file force-include); only .c files passed as TUs, not headers; --quiet option skipped so unusedFunction output is visible; build dir cleaned each run
# 2026-08-30: Added dead-code (unused function) analysis via cppcheck --enable=unusedFunction, with tests/.deadcode-baseline.txt for known false positives
# 2026-07-15: Report build error counts and ensure non-zero exit propagates to callers (e.g. mkt && mka chains)
# 2026-07-15: Added static-function gate to block NEW static functions in src/ (Unity tests cannot call static fns)
# 2025-12-05: Added dependency check to prevent .c file includes in Unity tests and mocks, resolving gcov regeneration issue
# 2025-09-24: Added QUICK parameter to skip cleaning and cmake configuration
# 2025-07-15: Added Unity test compilation to catch errors in test code during trial builds

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

pushd . >/dev/null 2>&1 || exit 1

# Check if QUICK parameter is supplied
QUICK_MODE=false
for arg in "$@"; do
    if [[ "${arg}" == "QUICK" ]]; then
        QUICK_MODE=true
        break
    fi
done

START_TIME=$(date +%s)

if [[ "${QUICK_MODE}" != "true" ]]; then
    echo "$(date +%H:%M:%S.%3N || true) - Cleaning Build Directory"
    rm -rf build/*
    rm -rf hydrogen_*

    echo "$(date +%H:%M:%S.%3N || true) - Configuring CMake"
    cd cmake && cmake -S . -B ../build --preset default >/dev/null 2>&1
else
    cd cmake || exit 1
fi

# Dependency Check - Ensure no .c files are included in Unity tests and mocks
echo "$(date +%H:%M:%S.%3N || true) - Dependency Check"
popd >/dev/null 2>&1 || exit 1

# Check for improper .c includes in Unity tests
UNITY_C_INCLUDES=$(grep -r "\.c" tests/unity/src 2>/dev/null | grep -i include || true)
MOCKS_C_INCLUDES=$(grep -r "\.c" tests/unity/mocks 2>/dev/null | grep -i include || true)

if [[ -n "${UNITY_C_INCLUDES}" ]]; then
    echo "❌ Found improper .c includes in Unity tests:"
    echo "${UNITY_C_INCLUDES}"
    exit 1
fi

if [[ -n "${MOCKS_C_INCLUDES}" ]]; then
    echo "❌ Found improper .c includes in Unity mocks:"
    echo "${MOCKS_C_INCLUDES}"
    exit 1
fi

# Static-function gate (INSTRUCTIONS.md "NO static FUNCTIONS" rule).
# Unity Framework tests link directly against the project's object files
# and CANNOT call static functions, so a static helper is impossible to
# unit-test. We grandfather the static functions that already exist
# (listed in tests/.static-baseline.txt) and FAIL THE BUILD on any NEW
# static function definition in src/. This is the automated enforcement
# of the rule - a doc note did not stop mailrelay/scripting from
# shipping ~200 static helpers that later had to be de-static'd by hand.
STATIC_BASELINE="tests/.static-baseline.txt"
if [[ ! -f "${STATIC_BASELINE}" ]]; then
    echo "⚠️  Static baseline missing (${STATIC_BASELINE}); regenerating from current src/"
    rg -N --no-heading -n '^\s*static\b.*\w+\s*\(' -g '*.c' -g '*.h' src \
        | rg -o 'static\s+[\w\s\*]*?(\w+)\s*\(' -r '$1' | sort -u > "${STATIC_BASELINE}" || true
fi

# Collect static FUNCTION DEFINITIONS (lines ending in '{', i.e. not
# forward declarations which end in ';'). Report any whose name is not
# in the baseline.
NEW_STATIC=$(rg -N --no-heading -n '^\s*static\b.*\w+\s*\(' -g 'src/**/*.c' -g 'src/**/*.h' \
    | grep -E '\)\s*\{?\s*$' \
    | rg -o 'static\s+[\w\s\*]*?(\w+)\s*\(' -r '$1' \
    | sort -u \
    | comm -23 - <(sort -u "${STATIC_BASELINE}") || true)

if [[ -n "${NEW_STATIC}" ]]; then
    echo "❌ NEW static functions found in src/ (Unity tests cannot call static functions):"
    echo "${NEW_STATIC}" | while read -r fn; do
        [[ -n "${fn}" ]] && echo "  - ${fn}"
    done
    echo "Remove 'static' and add a header declaration, OR (if genuinely"
    echo "module-private state) regenerate the baseline with: rg -N --no-heading"
    printf '%s\n' "  -n '^\\s*static\\b.*\\w+\\s*\\(' -g '*.c' -g '*.h' src | rg -o"
    printf '%s\n' "  'static\\s+[\\w\\s\\*]*?(\\w+)\\s*\\(' -r '\$1' | sort -u > tests/.static-baseline.txt"
    exit 1
fi

echo "$(date +%H:%M:%S.%3N || true) - Dependency Check Passed"
cd cmake || exit 1

# Build mku completions mechanism
project_root=$(cd ".." && pwd 2>/dev/null) || exit 0
unity_src_dir="${project_root}/tests/unity/src"
mapfile -t tests < <(find "${unity_src_dir}" -name "*.c" -exec basename {} .c \; | sort -u || true)

# Generate/update the completion file
completion_file="${HOME}/.mku-complete.bash"
cat > "${completion_file}" << EOF
# Auto-generated mku completions on $(date || true)
complete -W "${tests[*]}" mku
EOF

# Count for logging
num_tests=${#tests[@]}
echo "$(date +%H:%M:%S.%3N || true) - Updating ${num_tests} Completions" 
export MKU_NUM_TESTS=${num_tests}

# Print eval command for immediate reload in current shell
# (User pastes/runs this once; it sources the new file)
# cat << 'EOT'
# # To apply now in this terminal: eval "$(cat << 'RELOAD_EOF'
# source ~/.mku-complete.bash
# #RELOAD_EOF
# )"
# EOT

zsh_file="${HOME}/.mku-zsh-complete.zsh"
cat > "${zsh_file}" << EOS
# Dynamic mku completions for zsh (generated on $(date || true))
_mku() {
    # Only proceed if we're actually completing (not during shell startup)
    [[ -z "\$compstate" ]] && return 0

    local cur="\${words[CURRENT]}"
    local project_root unity_src_dir cache_file

    # Use cached project root path to avoid repeated directory checks
    if [[ -z "\$_MKU_PROJECT_ROOT" ]]; then
        _MKU_PROJECT_ROOT="${HYDROGEN_ROOT}"
        [[ ! -d "\$_MKU_PROJECT_ROOT" ]] && return 1
    fi

    unity_src_dir="\$_MKU_PROJECT_ROOT/tests/unity/src"
    cache_file="\${HOME}/.mku_cache"

    # Fast cache check - only rebuild if cache doesn't exist or is outdated
    if [[ ! -f "\$cache_file" || "\$unity_src_dir" -nt "\$cache_file" ]]; then
        # Use find with -printf for faster execution when available
        if find "\$unity_src_dir" -name "*.c" -printf "%f\\n" >/dev/null 2>&1; then
            _mku_tests=(\$(find "\$unity_src_dir" -name "*.c" -printf "%f\\n" | sed 's/\\.c$//' | sort -u))
        else
            _mku_tests=(\$(find "\$unity_src_dir" -name "*.c" -exec basename {} .c \\; | sort -u))
        fi
        # Store in global array and cache file
        print -l "\${_mku_tests[@]}" >| "\$cache_file"
    else
        # Load from cache
        _mku_tests=(\$(< "\$cache_file"))
    fi

    # Only complete first argument (CURRENT = 1 for first arg, 2 for second, etc.)
    [[ \$CURRENT -gt 1 ]] && return 0

    # Use compadd for better performance
    compadd -a _mku_tests
}

# Register completion (lazy-loaded)
compdef _mku mku

# Pre-cache on first use to avoid delay during completion
_mku_precache() {
    [[ -f "\${HOME}/.mku_cache" ]] && return
    _mku 2>/dev/null >/dev/null
}
autoload -Uz _mku_precache

# Embedded count for reload feedback
export MKU_NUM_TESTS=${num_tests}
EOS

echo "$(date +%H:%M:%S.%3N || true) - Default Build"

# Build main project with payload and check for errors
BUILD_OUTPUT=$(cmake --build ../build --target hydrogen 2>&1)
ERRORS=$(echo "${BUILD_OUTPUT}" | grep -B 2 -E "error:|warning:|undefined reference|collect2:|ld returned" || true)
if [[ -n "${ERRORS}" ]]; then
    echo "${ERRORS}"
    ERROR_COUNT=$(echo "${BUILD_OUTPUT}" | grep -c -E "error:" || true)
    echo "❌ Default build failed with ${ERROR_COUNT} error(s)"
    exit 1
fi

echo "$(date +%H:%M:%S.%3N || true) - Unity Build"

# Build Unity tests and check for errors
UNITY_BUILD_OUTPUT=$(cmake --build ../build --target unity_tests 2>&1)
UNITY_ERRORS=$(echo "${UNITY_BUILD_OUTPUT}" | grep -B 2 -E "error:|warning:|undefined reference|collect2:|ld returned" || true)
if [[ -n "${UNITY_ERRORS}" ]]; then
    echo "${UNITY_ERRORS}"
    UNITY_ERROR_COUNT=$(echo "${UNITY_BUILD_OUTPUT}" | grep -c -E "error:" || true)
    echo "❌ Unity build failed with ${UNITY_ERROR_COUNT} error(s)"
    exit 1
fi

# Remove hydrogen_naked if it exists (byproduct of release builds)
if [[ -f "hydrogen_naked" ]]; then
    rm -f "hydrogen_naked"
fi


# Check if build was successful
if (echo "${UNITY_BUILD_OUTPUT}" | grep -q "completed successfully" || echo "${UNITY_BUILD_OUTPUT}" | grep -q "no work to do") && [[ -z "${ERRORS}" ]]; then
    echo "$(date +%H:%M:%S.%3N || true) - Build Successful"

    # Return to project root for final checks (do NOT rely on the dirstack:
    # the dependency-check popd at the top already consumed the pushd, so the
    # stack is empty and popd here would be a no-op, leaving CWD in cmake/).
    cd "${HYDROGEN_ROOT}" >/dev/null 2>&1 || true

    # Binary is already created in root directory by hydrogen target

    # Run shutdown test
    echo "$(date +%H:%M:%S.%3N || true) - Running Shutdown Test"

    if [[ -f "tests/test_16_shutdown.sh" ]]; then
        "tests/test_16_shutdown.sh" >/dev/null 2>&1 && echo "$(date +%H:%M:%S.%3N || true) - Shutdown Test Passed" || echo "❌ Shutdown test failed"
    else
        echo "⚠️  Shutdown test not found"
    fi
    
    # Analyze unused files
    MAP_FILE="build/regular/regular.map"
    if [[ -f "${MAP_FILE}" ]]; then
        echo "$(date +%H:%M:%S.%3N || true) - Checking Source Files"
        
        # Extract object files that are actually linked
        USED_OBJS=$(awk '/^LOAD.*\.o$/ {print $2}' "${MAP_FILE}" | grep -v "^/usr" | sort -u || true)
        
        # Convert object files back to source file paths
        USED_SRCS=""
        for obj in ${USED_OBJS}; do
            # Extract the source path from the object file path
            # Object files are like: regular/src/config/config.o
            if [[ "${obj}" == *"/src/"* ]]; then
                src_path=$(echo "${obj}" | sed 's|^.*/src/|./|' | sed 's|\.o$|.c|' || true)
                USED_SRCS="${USED_SRCS}\n${src_path}"
            fi
        done
        USED_SRCS=$(echo -e "${USED_SRCS}" | grep -v "^$" | sort -u || true)
        
        # Find all source files relative to src directory
        ALL_SRCS=$(find "src" -name "*.c" | sed "s|^src/|./|" | sort || true)
        
        # Read ignored files (already in ./ format)
        IGNORED_SRCS=""
        if [[ -f ".trial-ignore" ]]; then
            IGNORED_SRCS=$(grep -v '^#' ".trial-ignore" | grep -v '^$' | sort || true)
        fi
        
        # Find unused sources
        UNUSED_SRCS=$(comm -23 <(echo "${ALL_SRCS}") <(echo "${USED_SRCS}"))
        REPORTABLE_SRCS=$(comm -23 <(echo "${UNUSED_SRCS}") <(echo "${IGNORED_SRCS}"))
        
        if [[ -z "${REPORTABLE_SRCS}" ]]; then
            echo "$(date +%H:%M:%S.%3N || true) - Source Files Checked"
        else
            echo "$(date +%H:%M:%S.%3N || true) - Source Files Checked"
            echo "⚠️  Unused source files:"
            echo "${REPORTABLE_SRCS}" | while read -r file; do
                if [[ -n "${file}" ]]; then
                    echo "  ${file}"
                fi
            done
        fi
    else
        echo "⚠️  Map file not found, skipping unused file analysis"
    fi

    # Dead Code Analysis — find functions unreachable from main() in a
    # hydrogen_release-shaped link.
    #
    # This replaced an earlier cppcheck --enable=unusedFunction (source-level
    # name-use scan) approach, which is a heuristic prone to both false
    # positives (library-shaped registration surfaces) and false negatives
    # (missed dlsym/table references) on this codebase, and a still-earlier
    # nm+objdump-on-.o approach, which was abandoned because -O2 inlining
    # removes call-site relocations for functions called once, making them
    # look dead when they are not.
    #
    # This approach builds the "deadcode" CMake target (see
    # cmake/CMakeLists-deadcode.cmake): all of src/ at -O0 (no inlining, so no
    # lost relocations - see that file's comments for why -O2 would also work
    # but -O0 is faster and equally correct here) linked exactly the way
    # hydrogen_release is (--gc-sections + --dynamic-list=lua_export.list, NOT
    # -rdynamic, which defeats --gc-sections - see CMakeLists-release.cmake)
    # plus --print-gc-sections. The linker then reports, by name, every
    # function section it discarded because nothing reachable from main()
    # referenced it - a direct answer to "is this function ever called in the
    # program we ship", not a heuristic. Because only src/*.c files are
    # objects in this link, a function called exclusively from Unity tests is
    # correctly still reported dead (tests/ isn't part of what we ship).
    #
    # The raw gc-sections output also includes discarded static/static-inline
    # symbols from third-party headers (e.g. jansson's inline json_decref,
    # curl's _curl_easy_setopt_err_* type-checkers) that coincidentally share
    # names across translation units. Intersecting with the global ('T', not
    # local 't') symbols nm reports for our own objects removes that noise,
    # since every project function is non-static (INSTRUCTIONS.md "NO static
    # FUNCTIONS" rule) and therefore globally unique.
    #
    # Note: dlsym-by-name lookups and weak symbols still cannot be detected —
    # add those to tests/.deadcode-baseline.txt as needed.
    DEADCODE_BUILD_OUTPUT=$(cmake --build build --target deadcode 2>&1)
    DEADCODE_LINK_ERRORS=$(echo "${DEADCODE_BUILD_OUTPUT}" | grep -E "error:|undefined reference|collect2:|ld returned" || true)
    if [[ -n "${DEADCODE_LINK_ERRORS}" ]]; then
        echo "${DEADCODE_LINK_ERRORS}"
        echo "❌ Dead code analysis build failed"
        exit 1
    fi

    # Remove the byproduct binary - only the linker's --print-gc-sections
    # output (already captured above) is needed.
    rm -f "hydrogen_deadcode"

    DEADCODE_OBJ_DIR="build/deadcode/src"
    if [[ -d "${DEADCODE_OBJ_DIR}" ]]; then
        # Global (externally-linked) symbols defined in our own objects: used
        # both as the "checking N functions" count and to filter the
        # gc-sections report down to real src/ functions.
        GLOBAL_SYMS=$(find "${DEADCODE_OBJ_DIR}" -name "*.o" -print0 \
            | xargs -0 nm --defined-only 2>/dev/null \
            | grep ' T ' | awk '{print $3}' | sort -u || true)
        FUNC_COUNT=$(echo "${GLOBAL_SYMS}" | grep -c . || true)

        # Sections the linker discarded from our own objects (paths are
        # relative to build/, e.g. "deadcode/src/..."), intersected with
        # GLOBAL_SYMS to drop third-party header inline-symbol noise.
        REMOVED_SECTIONS=$(echo "${DEADCODE_BUILD_OUTPUT}" \
            | grep "removing unused section '\.text\." \
            | grep "deadcode/src/" \
            | sed -E "s/.*section '\.text\.([^']+)'.*/\1/" \
            | sort -u || true)
        DEAD_FUNCS=$(comm -12 <(echo "${REMOVED_SECTIONS}") <(echo "${GLOBAL_SYMS}") || true)
    else
        FUNC_COUNT="unknown"
        DEAD_FUNCS=""
    fi

    # Filter with baseline (known false positives: entry points, test seams,
    # dlsym-looked-up by name, etc.)
    BASELINE_FILE="tests/.deadcode-baseline.txt"
    if [[ -f "${BASELINE_FILE}" ]]; then
        REPORTABLE_DEAD=$(comm -23 <(echo "${DEAD_FUNCS}") <(sort -u "${BASELINE_FILE}") || true)
    else
        REPORTABLE_DEAD="${DEAD_FUNCS}"
    fi
    DEAD_COUNT=$(echo "${REPORTABLE_DEAD}" | grep -c . || true)

    DEADCODE_LIST_FILE="build/deadcode/dead_functions.txt"
    printf '%s\n' "${REPORTABLE_DEAD}" | grep -v '^$' > "${DEADCODE_LIST_FILE}" || true

    echo "$(date +%H:%M:%S.%3N || true) - Checking ${FUNC_COUNT} Functions for Dead Code"
    echo "$(date +%H:%M:%S.%3N || true) - Found ${DEAD_COUNT} Dead Functions"
    echo "$(date +%H:%M:%S.%3N || true) - Full list: ${DEADCODE_LIST_FILE}"
    ELAPSED=$(( $(date +%s) - START_TIME ))
    ELAPSED_FMT=$(printf '%dm %ds' $(( ELAPSED / 60 )) $(( ELAPSED % 60 )))
    echo "$(date +%H:%M:%S.%3N || true) - Trial Build Complete (${ELAPSED_FMT})"
else
    echo "❌ Build failed"
    FINAL_ERROR_COUNT=$( (echo "${BUILD_OUTPUT}"; echo "${UNITY_BUILD_OUTPUT}") | grep -c -E "error:" || true)
    if [[ "${FINAL_ERROR_COUNT}" -gt 0 ]]; then
        echo "❌ Build failed with ${FINAL_ERROR_COUNT} error(s)"
    fi
    exit 1
fi
