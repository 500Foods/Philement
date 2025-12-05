#!/usr/bin/env bash

# CHANGELOG
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
    exit 1
fi

echo "$(date +%H:%M:%S.%3N || true) - Unity Build"

# Build Unity tests and check for errors
UNITY_BUILD_OUTPUT=$(cmake --build ../build --target unity_tests 2>&1)
UNITY_ERRORS=$(echo "${UNITY_BUILD_OUTPUT}" | grep -B 2 -E "error:|warning:|undefined reference|collect2:|ld returned" || true)
if [[ -n "${UNITY_ERRORS}" ]]; then
    echo "${UNITY_ERRORS}"
    exit 1
fi

# Remove hydrogen_naked if it exists (byproduct of release builds)
if [[ -f "hydrogen_naked" ]]; then
    rm -f "hydrogen_naked"
fi


# Check if build was successful
if (echo "${UNITY_BUILD_OUTPUT}" | grep -q "completed successfully" || echo "${UNITY_BUILD_OUTPUT}" | grep -q "no work to do") && [[ -z "${ERRORS}" ]]; then
    echo "$(date +%H:%M:%S.%3N || true) - Build Successful"

    # Return to project root for final checks
    popd >/dev/null 2>&1 || exit 1

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
    echo "$(date +%H:%M:%S.%3N || true) - Trial Build Complete"
else
    echo "❌ Build failed"
    exit 1
fi
