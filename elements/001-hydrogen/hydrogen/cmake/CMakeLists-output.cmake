# Terminal Output Formatting
#
# This file contains terminal output formatting and status symbols.

# Terminal output formatting for better readability
# Note: CMake handles escape sequences differently, so we use string literals
string(ASCII 27 ESC)
set(GREEN "${ESC}[0;32m")
set(RED "${ESC}[0;31m")
set(YELLOW "${ESC}[0;33m")
set(BLUE "${ESC}[0;34m")
set(CYAN "${ESC}[0;36m")
set(MAGENTA "${ESC}[0;35m")
set(BOLD "${ESC}[1m")
set(NORMAL "${ESC}[0m")

# Unicode status symbols for visual feedback
set(PASS "✅")
set(FAIL "❌")
set(WARN "⚠️")
set(INFO "🛈")

# Print configuration summary
message(STATUS "")
message(STATUS "Hydrogen Build Configuration Summary:")
message(STATUS "  Version: ${HYDROGEN_VERSION}")
message(STATUS "  Release: ${HYDROGEN_RELEASE}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")
if(UPX_EXECUTABLE)
    message(STATUS "  UPX Available: ${UPX_EXECUTABLE}")
else()
    message(STATUS "  UPX Available: No")
endif()
if(VALGRIND_EXECUTABLE)
    message(STATUS "  Valgrind Available: ${VALGRIND_EXECUTABLE}")
else()
    message(STATUS "  Valgrind Available: No")
endif()
message(STATUS "")