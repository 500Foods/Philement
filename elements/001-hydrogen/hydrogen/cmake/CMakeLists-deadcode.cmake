# Dead Code Analysis Build Configuration for Hydrogen
#
# This file contains a build variant used exclusively by extras/make-trial.sh
# to detect functions in src/ that are unreachable from main() in a
# hydrogen_release-shaped link. It is not intended for running - only for
# feeding its link output to a linker-based reachability analysis.
#
# Why this shape:
# - -O0 disables inlining, so every non-static (project convention forbids
#   static helpers - see INSTRUCTIONS.md) function still gets a call-site
#   relocation to its out-of-line body. At -O2 the same holds (GCC always
#   keeps out-of-line bodies for externally-linked functions unless LTO
#   proves otherwise), but -O0 keeps compilation fast for this throwaway
#   analysis target and avoids any doubt.
# - No -flto: LTO performs its own dead-code elimination during IR merging,
#   before section-level GC ever runs, so --print-gc-sections reports almost
#   nothing under LTO even though functions really were eliminated. Since
#   hydrogen_release links with -flto=auto, its own build can't be introspected
#   this way - this variant reproduces release's *linker* behavior (gc-sections
#   + dynamic-list, not -rdynamic) without LTO so the removed sections are
#   individually named.
# - -Wl,--dynamic-list=lua_export.list (not -rdynamic): matches hydrogen_release.
#   -rdynamic exports every Hydrogen symbol to .dynsym, which defeats
#   --gc-sections (see CMakeLists-release.cmake) and would hide real dead code.
# - -Wl,--print-gc-sections: makes the linker log every discarded section by
#   name; extras/make-trial.sh parses "removing unused section '.text.<fn>'"
#   lines that reference our own build/deadcode/src/*.o files.
#
# Usage:
# cmake --build . --target deadcode : Build for dead-code reachability analysis
hydrogen_add_executable_target(deadcode "Dead-Code-Analysis"
    "-O0 -g0 -DNDEBUG"
    "-no-pie -Wl,--dynamic-list=${CMAKE_CURRENT_SOURCE_DIR}/scripts/lua_export.list -Wl,--print-gc-sections"
)
