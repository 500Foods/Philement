# Unity ASAN Plan

## Purpose

Add a separate AddressSanitizer (ASAN) build variant of the Unity unit-test
suite (`unity_asan`) so that memory-safety bugs in unit-testable code are
caught automatically, without disturbing the existing gcov coverage build.

This plan was prompted by a heap-use-after-free / double-free bug class in the
wschat API handlers: `api_send_json_response()` (in
`/elements/001-hydrogen/hydrogen/src/api/api_utils.c`) unconditionally
`json_decref()`s its `json_obj` argument on every return path, so any caller
that decrefs the same object afterward causes a use-after-free. That bug was
fixed in the conduit and wschat handlers, but no blackbox test reaches those
chat endpoints (they require a live AI provider), and the gcov Unity build does
not catch double-free at runtime. ASAN over Unity is what makes that bug class
a hard failure.

## Background and Rationale

### Why existing tests do not cover this

- Test 11 (`test_11_leaks_like_a_sieve.sh`) runs `hydrogen_debug` (ASAN) but
  only starts the server, hits `/api/system/health` three times, then sends
  SIGTERM. It is a startup/shutdown leak check and exercises almost no request
  handlers.
- Test 41 (`test_41_exercise_asan.sh`) runs `hydrogen_debug` (ASAN) with 500 auth
  requests across 6 database engines plus post-shutdown LeakSanitizer analysis.
  Strong, but only on auth + DB-queue happy paths. Native RSS is test 44.
- Test 42 (`test_42_oidc_rp.sh`) is broad on OIDC RP logic but selects its
  binary via `find_hydrogen_binary()`; with coverage/release present it does
  not run under ASAN. It is a functional/coverage test, not an ASAN gate.

All three are blackbox, server-running tests. They can only reach a handler if
the subsystem is enabled in config AND a real backing service exists (DB engine,
IdP, AI chat provider). The wschat chat handlers need a live AI engine, so no
blackbox test reaches them.

### What `unity_asan` is expected to uncover

This is a disjoint surface from Tests 11/41/42, not a duplicate:

1. Code unreachable from blackbox tests because it needs external services
   (entire wschat chat handler family and any provider-gated handler).
2. Error / cleanup paths that running servers rarely trigger:
   - `malloc`/`calloc`-failure branches (deterministically hit with
     `mock_system_set_malloc_failure`).
   - Validation/error responses ("payload too large", "engines array must
     contain 1-10", parse failures, "no healthy engines", etc.).
3. Ownership / double-free contracts like `api_send_json_response()` — invisible
   to gcov (tests pass green even with a double-free), but a hard ASAN failure.

Concretely, a first `unity_asan` run is expected to surface:

- Use-after-free / double-free in API handlers that follow (or used to follow)
  the `api_send_json_response()` + `json_decref()` pattern — grep the whole
  `src/api` tree, not just wschat.
- Heap-buffer-overflows in pure helper functions (hex/compress/hash/media
  storage helpers) where off-by-one writes pass silently under gcov.
- A large number of LeakSanitizer "leaks" that are mostly test-fixture leaks,
  NOT product bugs (existing Unity tests `calloc` fixtures inside `if` blocks
  and skip frees on assert-fail paths). This is the cleanup tax.

## Scope Decision

ASAN-over-Unity is warranted as a memory-safety gate (use-after-free,
double-free, heap-buffer-overflow), NOT as a full leak gate. Full LeakSanitizer
on Unity overlaps Test 11/41 at the integration level and would drown the suite
in fixture-leak noise.

Therefore:

- Start with ASAN and `detect_leaks=0`. This catches the high-value bug class
  (the wschat double-free WOULD have been caught) for a fraction of the cleanup
  effort.
- Graduating to `detect_leaks=1` is a separate, optional, later push that
  requires a fixture-leak cleanup pass across the existing Unity tests.

Do NOT add `-fsanitize=address` to the existing gcov Unity build. Reasons:

- LeakSanitizer would flood the suite with fixture-leak failures.
- If ASAN aborts on first error, `__gcov_flush` never runs and that test's
  `.gcda` is lost/truncated, corrupting the Test 10 / Test 99 coverage
  reconciliation described in
  [`/docs/H/tests/TESTING_UNITY.md`](/docs/H/tests/TESTING_UNITY.md).
- ASAN is ~2x slower and ~3x memory; the gcov Unity run is meant to be fast for
  dev iteration.

The clean approach mirrors the app's own `hydrogen` / `hydrogen_coverage` /
`hydrogen_debug` split: a separate parallel object tree that never shares object
files or `.gcda` with the gcov tree.

## Current Build Facts (Reference)

All paths relative to `/elements/001-hydrogen/hydrogen/`.

- ASAN flags already used by the debug target (in
  `cmake/CMakeLists-debug.cmake`):
  - compile: `-g -fsanitize=address,leak -fno-omit-frame-pointer`
  - link: `-lasan -fsanitize=address,leak -no-pie`
- Unity tests are defined in `cmake/CMakeLists-unity.cmake`:
  - Per-test source compiles with `-O0 -g -fno-omit-frame-pointer --coverage
    -fprofile-arcs -ftest-coverage` (around lines 504-525), output to
    `build/unity/src/<dir>/<name>.o`.
  - `hydrogen_unity` is a STATIC lib of all instrumented source objects
    (line 287); shared object libs `unity_framework`, `unity_mocks`,
    `unity_print_mocks`.
  - Each test executable links `hydrogen_unity unity_framework unity_mocks
    unity_print_mocks ${HYDROGEN_BASE_LIBS} --coverage -lgcov` with
    `-Wl,--gc-sections`; output to `build/unity/src/<dir>/<name>`.
  - `add_test(NAME hydrogen_unity_tests ...)` near line 619; `unity_tests`
    custom target builds the suite with coverage.
- `mkt` runs `extras/make-trial.sh` (also does the Dependency Check that forbids
  `.c` includes in `tests/unity/src` and `tests/unity/mocks`).
- `mka` runs `extras/make-all.sh`; Test 01 (`tests/test_01_compilation.sh`)
  builds `cmake --build ../build --preset default --target all_variants` and
  verifies `hydrogen`, `hydrogen_debug`, `hydrogen_coverage`, release, naked,
  examples, and a Unity payload test executable.
- Test 10 (`tests/test_10_unity.sh`) discovers executables under
  `build/unity/src` matching `*_test*` and runs them (CTest-style), caching
  results.
- `api_send_json_response` is NOT mocked by `mock_api_utils`; handler tests link
  the real one, which is exactly what makes ASAN catch the double-free. The
  existing reference handler test is
  `tests/unity/src/api/conduit/status/status_test_handle_conduit_status_request.c`.
  Two new wschat handler tests already exist and pass on the gcov build:
  - `tests/unity/src/api/wschat/auth_chat/auth_chat_test_handle_auth_chat_request.c`
  - `tests/unity/src/api/wschat/auth_chats/auth_chats_test_handle_auth_chats_request.c`

## Implementation Phases

### Phase 1 — `unity_asan` CMake variant

- Add `cmake/CMakeLists-unity-asan.cmake` (model on
  `cmake/CMakeLists-coverage.cmake` and the per-test section of
  `cmake/CMakeLists-unity.cmake`).
- Compile the SAME Unity sources and the SAME `src/` sources into a parallel
  `build/unity_asan/` tree with:
  - `-O0 -g -fsanitize=address -fno-omit-frame-pointer` and NO `--coverage`,
    NO `-fprofile-arcs`, NO `-ftest-coverage`.
  - link with `-fsanitize=address` (and `-lasan` if required to match the debug
    target), NO `--coverage`, NO `-lgcov`.
- Build a parallel static lib (e.g. `hydrogen_unity_asan`) plus asan variants of
  `unity_framework` / `unity_mocks` / `unity_print_mocks`, or reuse object libs
  recompiled with ASAN. Keep object files entirely separate from
  `build/unity/`.
- Add a `unity_asan_tests` custom target that builds all asan test executables
  into `build/unity_asan/src/<dir>/<name>`.
- Wire the new cmake file into `cmake/CMakeLists.txt` (include) but keep it OUT
  of the default `all_variants` unless the build-time cost is acceptable; prefer
  an explicit target so `mka` is not slowed by default.
- Ensure `mkt`'s Dependency Check still passes (no `.c` includes introduced).

### Phase 2 — Test harness (new Test, e.g. `test_11`-adjacent)

- Add a new blackbox test script (pick an unused number; coordinate with the
  list in [`/docs/H/INSTRUCTIONS.md`](/docs/H/INSTRUCTIONS.md) and
  [`/docs/H/tests/TESTING.md`](/docs/H/tests/TESTING.md)) that:
  - Verifies the `unity_asan` executables exist (and have `__asan` symbols via
    `readelf -s`, mirroring Test 11's ASAN-presence check).
  - Runs each `build/unity_asan/src/**/*_test*` executable with
    `ASAN_OPTIONS="detect_leaks=0:halt_on_error=1:abort_on_error=1"`.
  - Captures each test's stdout/stderr to a per-test log under the standard
    `build/tests/logs/` location.
  - FAILS the subtest if the log matches
    `AddressSanitizer|heap-use-after-free|heap-buffer-overflow|double-free|stack-buffer-overflow`
    OR if the executable exits non-zero / crashes.
  - Follows the project Bash coding requirements (shellcheck clean, `[[ ]]`,
    braced `${vars}`, `jq` for any JSON, CHANGELOG + TEST_VERSION header).
- Reuse `tests/lib/` helpers where possible; model lifecycle/logging on
  `tests/test_10_unity.sh` and `tests/test_11_leaks_like_a_sieve.sh`.

### Phase 3 — Triage first run (memory-safety only)

- Run the suite under ASAN with `detect_leaks=0` and triage every
  use-after-free / double-free / buffer-overflow as a REAL product bug.
- Grep `src/api` broadly for the `api_send_json_response(...)` followed by
  `json_decref(<same object>)` pattern and fix any remaining sites (conduit and
  wschat are already fixed; verify others such as system endpoints, auth, oidc).
- Fix genuine overflows surfaced in pure helpers (storage hex/compress/hash/
  media, etc.).
- For each real fix, follow the C coding requirements and re-run `mkt`, `mkp`,
  and the affected Unity tests.

### Phase 4 (optional, later) — Leak gate

- Add a fixture-leak cleanup pass across `tests/unity/src` (free fixtures in
  `tearDown`, avoid leak-on-assert paths).
- Flip the harness to `detect_leaks=1` once the suite is clean, or run a second
  pass that only enforces leaks on a curated subset.

## Integration / Documentation Touchpoints

When the new test number is assigned, update (per the renumber/rename rules in
[`/docs/H/tests/TESTING.md`](/docs/H/tests/TESTING.md)):

- [`/docs/H/INSTRUCTIONS.md`](/docs/H/INSTRUCTIONS.md) — TESTS list.
- [`/docs/H/tests/TESTING.md`](/docs/H/tests/TESTING.md) — test script list.
- [`/docs/H/STRUCTURE.md`](/docs/H/STRUCTURE.md) — new files (cmake + test +
  doc).
- [`/docs/H/SITEMAP.md`](/docs/H/SITEMAP.md) — new test documentation page.
- Add a `tests/docs/test_NN_*.md` page for the new test.
- [`/RELEASES.md`](/RELEASES.md) — note the new ASAN unit-test gate.
- Run `tests/test_04_check_links.sh` after adding docs.

## Validation Checklist

- [ ] `mkt` passes (no `.c`-include regressions, Dependency Check green).
- [ ] `mkp` clean (cppcheck) for any C changes.
- [ ] `unity_asan` target builds; executables contain `__asan` symbols.
- [ ] Existing gcov Unity build (`build/unity/`) is byte-for-byte unaffected;
      Test 10 and Test 99 coverage reconciliation unchanged.
- [ ] New ASAN test runs all unit tests under `detect_leaks=0` with ZERO
      AddressSanitizer errors after fixes.
- [ ] `mka` still succeeds and is not unacceptably slowed (ASAN variant gated
      behind an explicit target if needed).
- [ ] Docs / links updated; `test_04_check_links.sh` green.

## Constraints (from INSTRUCTIONS.md)

- `src/hydrogen.h` included first in all `.c` files; function prototypes for all
  functions; grouped/commented includes; no `goto`; use `log_this` for log
  output; `#include <src/folder/...>` not relative paths.
- Bash: shellcheck clean, `[[ ]]`, braced quoted `${vars}`, `jq` for JSON,
  update CHANGELOG + TEST_VERSION on every script change.
- Only create new test scripts when explicitly asked (this plan authorizes the
  one new ASAN test script).
