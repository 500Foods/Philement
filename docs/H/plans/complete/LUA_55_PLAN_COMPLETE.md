<!-- markdownlint-disable MD007 MD024 -->
# LUA_55 — Upgrade Embedded Lua 5.4 → 5.5

## Purpose

Move Hydrogen’s embedded Lua from **5.4** to **5.5** in a single-version cutover.
No dual-ABI support. No Helium language rewrite expected. Small C API fixes,
pkg-config/soname pin, then scripting + migration regression.

Upstream: Lua **5.5.0** (2025-12-22), current **5.5.1** (2026-08-03).
Manual incompatibilities: [§8](https://www.lua.org/manual/5.5/manual.html#8).

## How To Use This Document

- Work **one phase at a time**, top to bottom.
- Each phase: **Goal**, **Dependencies**, **Entry gate**, **Work items**,
  **Exit gate / validation**, **Status**, **Lessons learned**.
- Mark `[x]` only when verification passed. Defer with `[~]` + one-line reason.
- After each phase: fill **Status**, append **Working Log**, stop for review.
- Build aliases: `zsh -ic 'mkt'`, `mku <base>`, `mkp`, `mka` (C); `mks` (shell).
  See [INSTRUCTIONS.md](/docs/H/INSTRUCTIONS.md).

### Testing policy

| Layer | When | What |
|-------|------|------|
| **Unity** | After C API changes (Phase 2) | Scripting suite, especially dump/require/hook/gc |
| **Blackbox** | Phase 3 | test_43, test_46; migration 32–38 (or agreed subset) |
| **Lint** | After C / shell / docs | `mkp`, `mks`, test_98 as needed |
| **Done** | Phase 4 | Docs + version probe show 5.5; no 5.4 pin left in build |

## Resuming Work

**CURRENT PAUSE POINT (as of 2026-08-09):** Plan **complete**. Moved to
`complete/LUA_55_PLAN_COMPLETE.md`. Embed Lua **5.5.1**; migrations need
`brotli.so` + `-rdynamic`.

### Carry forward to Lua 5.6+ (host install)

When bumping embed past 5.5 (e.g. **5.6** plan), re-apply these host/link rules
or `mka` / test_01 will fail the same way:

1. **Static `liblua.a` must be built with `-fPIC`** if any Hydrogen target links
   PIE. Coverage uses `-fPIE`/`-pie` (GDB core dumps). Upstream
   `make linux` / `make linux-readline` defaults are **not** PIC → linker error:
   `R_X86_64_32S ... can not be used when making a PIE object` against
   `liblua.a(lapi.o)` (e.g. `luaT_typenames_`).
2. **Install command (source tarball):**  
   `make linux MYCFLAGS='-fPIC' && sudo make install`  
   (plus `lua.pc` / `PKG_CONFIG_PATH` so CMake does not pick an older distro
   `liblua.so`).
3. **Non-coverage binaries** use `-no-pie -rdynamic` (regular/debug/perf/valgrind/
   release). `-rdynamic` still required so C rocks (`brotli.so`) resolve Lua
   symbols from the static embed.
4. **CMake:** `link_directories(${LUA_LIBRARY_DIRS})` after `pkg_check_modules`
   so `-L` is not dropped; version gate stays “require N.N only”.
5. **Validate:** `objdump -r $prefix/lib/liblua.a | grep R_X86_64_32S` should be
   empty; then `mka` must produce `hydrogen_coverage` and `hydrogen_release`.

### Session checklist

1. Read **CURRENT PAUSE POINT** and last **Working Log** entries.
2. Confirm previous phase **Status** is complete.
3. Re-read next phase Goal + Exit gate only.
4. Implement → verify gates → update this doc → stop.

---

## Goals

1. **Single embed version** — Hydrogen links and runs Lua **5.5.x** only.
2. **C API correctness** — `lua_newstate` seed, `lua_dump` writer end-call, any
   compile breaks fixed under `-Werror`.
3. **No Helium rewrite** — migrations stay source-compatible; fix only if a
   real §8 language break appears.
4. **Regression green** — scripting Unity + blackbox 43/46 + migration path.
5. **Docs match reality** — H docs say 5.5; He authoring docs stop advertising
   5.1 as the runtime story.

### Non-goals

- Dual support for 5.4 and 5.5 in one binary or CMake matrix.
- Adopting new 5.5 syntax (`global` declarations, named vararg) in product Lua.
- Refactoring sandbox to `luaL_openselectedlibs` (optional later).
- Changing Helium migration patterns or SQL macros.
- Shipping luarocks C modules against a different ABI than the embed.

---

## Problem Statement

Hydrogen embeds Lua via `pkg_check_modules(LUA REQUIRED lua)`
([CMakeLists-init.cmake](/elements/001-hydrogen/hydrogen/cmake/CMakeLists-init.cmake)).
Host today resolves **5.4**. Lua 5.5 is released with a short §8 break list; the
embed surface is small but non-trivial:

| Risk | Where |
|------|--------|
| `lua_newstate` third arg (seed) | `lua_context.c`, `database/migration/lua.c` |
| `lua_dump` extra final writer call + stack rules | `bytecode_dump_writer` / scoreboard cache |
| Bytecode major incompatibility | In-process source_cache dumps only (restart clears) |
| Soname / pkg-config drift | `utils_dependency.c`, test_14, distro `lua.pc` |
| Hook/GC timing soft flakes | `lua_hook.c` COUNT/STEP samples |

Helium (~322 migrations) and DB-backed scripts are ordinary 5.x Lua; expected
language breakage is near zero (no `global` identifier; for-loop vars not mutated).

---

## Current Observed State (2026-08-09)

### Present

| Piece | Notes |
|-------|--------|
| Link | `pkg_check_modules(LUA REQUIRED lua)` → 5.4.0 on this host |
| States | `lua_newstate(lua_mmap_alloc, NULL)` ×2 |
| Sandbox | `luaL_openlibs` then null dangerous globals |
| Job ctx | `lua_getextraspace` |
| Limits | `lua_sethook` COUNT + `lua_gc` COUNT/STEP/COLLECT/ISRUNNING |
| Bytecode cache | `lua_dump` + `bytecode_dump_writer` |
| Tests | ~101 Unity under `tests/unity/src/scripting/`; BB 43, 46 |
| Docs | H: 5.4; He SETUP/LUA_INTRO: 5.1 authoring |

### Locked decisions (Phase 0)

Filled in Phase 0. Defaults below are the intended lock if Phase 0 agrees:

| Topic | Decision |
|-------|----------|
| Target | Lua **5.5.x** (prefer distro latest patch, e.g. 5.5.1) |
| Support window | **5.5 only** — drop 5.4 |
| CMake | Prefer versioned module if available (`lua5.5` / `lua-5.5`); else `lua` after host upgrade; fail configure if `LUA_VERSION_NUM < 505` |
| Seed | `luaL_makeseed(NULL)` (or documented equivalent) passed to `lua_newstate` |
| Dump writer | Tolerate `sz == 0` end marker; no stack use of `L` in writer (already) |
| GC API | Keep COUNT/STEP/COLLECT/ISRUNNING only; do not adopt `LUA_GCPARAM` unless needed |
| Helium | No proactive migration edits; fix only on proven runtime/parse failure |
| Optional libs | Skip `luaL_openselectedlibs` rewrite in this plan |
| Docs | H → 5.5; He → “matches Hydrogen embed (5.5)” + optional CLI note |
| Static lib PIC | **Always** build/install embed `liblua.a` with `MYCFLAGS='-fPIC'` (coverage is PIE). Copy this row into any future LUA_56+ plan. |

---

## Phase Groups

| Group | Phases | Theme |
|-------|--------|--------|
| A | 0 | Design lock + host readiness |
| B | 1 | Build / link / dependency probe |
| C | 2 | C API + Unity |
| D | 3 | Blackbox + migrations |
| E | 4 | Docs + closeout |

---

## Phases

### Phase 0: Design Lock + Host Readiness

- **Goal:** Confirm decisions above; prove 5.5 headers/libs exist on the build host.
- **Dependencies:** None.
- **Entry gate:** This document exists; assessment agreed.

#### Work items

- [x] **0.1** Confirm locked decisions table (edit if host forces different pc name).
- [x] **0.2** Inventory host: `pkg-config --modversion lua` / `lua5.5`; headers; `.so` soname.
- [x] **0.3** If 5.5 missing: install distro `lua5.5` / `liblua5.5-dev` (or build from source once) and record exact package names in Working Log.
- [x] **0.4** Note CI/image implication in Working Log (same packages).

#### Exit gate

- Locked table final.
- `pkg-config` (or equivalent) can compile a one-file `luaL_newstate` against **5.5**.
- Working Log lists package names and `LUA_VERSION` string.

#### Status

**Complete (2026-08-09).** Fedora 43 has no lua 5.5 RPM; upstream 5.5.1 installed
to `/usr/local` (bin + static `liblua.a` + headers + `lua.pc`). System Fedora
5.4.8 RPMs left in place.

#### Lessons learned

_(empty)_

---

### Phase 1: CMake + Dependency Probe

- **Goal:** Configure and link Hydrogen exclusively against Lua 5.5.
- **Dependencies:** Phase 0.
- **Entry gate:** 5.5 dev packages available.

#### Work items

- [x] **1.1** Update [CMakeLists-init.cmake](/elements/001-hydrogen/hydrogen/cmake/CMakeLists-init.cmake): find 5.5; reject `< 505`.
- [x] **1.2** Update [utils_dependency.c](/elements/001-hydrogen/hydrogen/src/utils/utils_dependency.c) soname/path list (`liblua.so.5.5` etc.); keep `LUA_VERSION` reporting accurate.
- [x] **1.3** Update test_14 / cmake README library lists if they hardcode 5.4. (test_14 had no 5.4 hardcode; expected version bumped in dependency table.)
- [x] **1.4** `zsh -ic 'mkt'` green (compile only is enough for this phase exit if link succeeds).

#### Exit gate

- Trial build links 5.5.
- Startup/dependency log (or unit path) reports Lua 5.5, not 5.4.
- No remaining intentional 5.4 link flags.

#### Status

**Complete (2026-08-09).** CMake rejects Lua &lt; 5.5; `link_directories(LUA_LIBRARY_DIRS)`
so static `/usr/local/lib/liblua.a` wins over Fedora `liblua.so.5.4`. Regular
binary contains `lua_newstate` / `luaL_makeseed` and string `Lua 5.5`.

#### Lessons learned

_(empty)_

---

### Phase 2: C API + Unity

- **Goal:** Fix §8.3 API breaks; scripting Unity green.
- **Dependencies:** Phase 1.
- **Entry gate:** `mkt` links 5.5.

#### Work items

- [x] **2.1** `lua_newstate` → three-arg form with seed in:
  - [lua_context.c](/elements/001-hydrogen/hydrogen/src/scripting/lua_context.c)
  - [lua.c](/elements/001-hydrogen/hydrogen/src/database/migration/lua.c)
  - Any Unity helpers that call `lua_newstate` directly if signatures break.
    (Unity uses `luaL_newstate` only — no direct `lua_newstate`.)
- [x] **2.2** Harden [bytecode_dump_writer](/elements/001-hydrogen/hydrogen/src/scripting/scripting_api_scoreboard.c): `sz == 0` is success no-op; confirm cache still loads via existing require tests.
- [x] **2.3** Grep for `LUA_GCINC`, `LUA_GCGEN`, `lua_resetthread`, `lua_setcstacklimit` — expect none; fix if found.
- [x] **2.4** `zsh -ic 'mkt'` then scripting Unity batch (or `mku` on dump/require/hook/gc/context tests at minimum; prefer full scripting tree).
- [x] **2.5** `zsh -ic 'mkp'` (cppcheck) clean for touched C.

#### Exit gate

- All targeted Unity tests pass.
- No new cppcheck issues on touched files.
- Working Log notes any instruction_count or GC timing adjustments.

#### Status

**Complete (2026-08-09).** All **101** scripting Unity tests PASS; `mkp` clean.
No instruction_count rebaseline needed.

#### Lessons learned

_(empty)_

---

### Phase 3: Blackbox + Migrations

- **Goal:** Runtime proof: scripting lifecycle, conduit script, migrations.
- **Dependencies:** Phase 2.
- **Entry gate:** Unity scripting green on 5.5.

#### Work items

- [x] **3.1** Blackbox [test_43_scripting](/docs/H/tests/test_43_scripting.md).
- [x] **3.2** Blackbox [test_46_conduit_script](/docs/H/tests/test_46_conduit_script.md).
- [x] **3.3** Migrations: at least one primary engine end-to-end (prefer Postgres or Yugabyte path used daily) plus DB2 if reverse-symmetry recently touched; full 32–38 if time allows — record exact set run.
- [x] **3.4** If Helium parse/runtime error: minimal fix only; document file + §8 cause.
- [x] **3.5** Shell changes: `zsh -ic 'mks'` if any test scripts touched. (none)

#### Exit gate

- 43 + 46 pass.
- Agreed migration set pass.
- No open SEGV or payload/Lua init failures.

#### Status

**Complete (2026-08-09) with noted variances.**

- **test_46:** 18/18 PASS (SQLite + DB2 full fixture; others skip missing Echo seed).
- **test_43:** 12/14 engine variants full lifecycle; **DB2×2** incomplete ticks due to
  missing `LITHIUM.SCRIPTS` on live DB (schema/seed), not Lua 5.5 — Orchestrator
  still started and ran iterations. Non-DB2 engines green.
- **test_32 Postgres + test_34 SQLite:** PASS after Helium `database.lua` fix +
  `brotli.so` + `-rdynamic` (299 migrations reversed each).

#### Lessons learned

_(empty)_

---

### Phase 4: Docs + Closeout

- **Goal:** Documentation and indexes match 5.5; plan completable.
- **Dependencies:** Phase 3.
- **Entry gate:** Blackbox gates green.

#### Work items

- [x] **4.1** H docs: [LUA_GUIDE.md](/docs/H/LUA_GUIDE.md), [LUA_FEATURES.md](/docs/H/LUA_FEATURES.md), comments in `lua_context.*` / LUA_PLAN references that assert “5.4 only”.
- [x] **4.2** He docs: [SETUP.md](/docs/He/SETUP.md), [LUA_INTRO.md](/docs/He/LUA_INTRO.md), [TESTING_GUIDE.md](/docs/He/TESTING_GUIDE.md) — authoring CLI may stay flexible; state **runtime = Hydrogen embed 5.5**.
- [x] **4.3** cmake README / test_98 luacheck std note if still `lua51` — bump or document “std is lint baseline, runtime is 5.5”. (left luacheck std; runtime documented in He SETUP)
- [~] **4.4** [RELEASES.md](/RELEASES.md) one-liner if project convention requires. — AI batch notes; skip one-off.
- [x] **4.5** Move this plan to `complete/LUA_55_PLAN_COMPLETE.md`; update [plans/README.md](/docs/H/plans/README.md), [SITEMAP.md](/docs/H/SITEMAP.md), run `mkl` / test_04 as required.
- [~] **4.6** Optional: full `mka` once before close. — `mkt` sufficient for cutover.

#### Exit gate

- No H doc still claiming embed is 5.4.
- Plan in `complete/`; indexes updated.
- CURRENT PAUSE POINT → done.

#### Status

**Complete (2026-08-09).**

#### Lessons learned

_(empty)_

---

## Optional follow-ups (out of plan)

| Item | Why defer |
|------|-----------|
| `luaL_openselectedlibs` sandbox | Behavior-preserving cleanup only |
| Adopt `global` in new scripts | Style; not required |
| `table.create` in hot Lua | Micro-opt |
| External strings for payload buffers | Needs design |

---

## Definition of Done

- [x] Configure fails or refuses Lua &lt; 5.5.
- [x] Binary reports / links Lua 5.5.
- [x] Phase 2 Unity + Phase 3 blackbox gates recorded green in Working Log.
- [x] Docs + plan complete move done.

---

## Working Log

### 2026-08-09 — Plan authored

- Assessment: medium-low overall; C surface small; packaging + regression are the real cost.
- Primary code touches expected: `lua_newstate` ×2, dump writer tolerance, CMake, `utils_dependency` paths, docs.
- Helium: no proactive edits.
- Start: Phase 0 host inventory.

### 2026-08-09 — Phase 0–1 + C API

- **Host:** Fedora 43 — no `lua` 5.5 RPM (`lua-5.4.8`). Built upstream **5.5.1** into
  `/usr/local` (`make install INSTALL_TOP=/usr/local` + `lua.pc`). Left Fedora
  5.4 RPMs installed.
- **Verify:** `lua -v` → 5.5.1; `pkg-config --modversion lua` → 5.5.1;
  `nm` on hydrogen shows `luaL_makeseed`; strings → `Lua 5.5`.
- **CMake pitfall:** `pkg_check_modules` put 5.5 in `CFLAGS` but link used bare
  `-llua` → Fedora 5.4 `.so`. Fix: `link_directories(${LUA_LIBRARY_DIRS})`.
- **API:** `lua_newstate(alloc, ud, luaL_makeseed(NULL))`; `luaL_openlibs` is a
  macro to `luaL_openselectedlibs` on 5.5 (no source change needed once linked
  correctly). Dump writer tolerates `sz==0`.

### 2026-08-09 — Phase 2–4 execution

- **Unity:** 101/101 scripting PASS; **mkp** PASS.
- **§8.1 Helium:** `for line in ... do line = ...` → `attempt to assign to const
  variable 'line'`. Fixed in all four `database.lua` (acuranzo/helium/gaius/glm)
  via local `stripped_line`. **Must regenerate payload** after Helium edits
  (`payloads/payload-generate.sh` + embed); `mkt` alone does not rebuild payload.
- **Static Lua + C rocks:** `require("brotli")` failed until hydrogen linked with
  **`-rdynamic`** (export Lua API to `brotli.so`). Build `lua-brotli` against
  5.5 headers; place `brotli.so` on cpath (`./brotli.so` or
  `/usr/local/lib/lua/5.5/`). luarocks 5.5 header detect may fail — manual make OK.
- **Migrations:** test_32 Postgres + test_34 SQLite full reverse green (299 each).
- **test_43 DB2:** incomplete lifecycle = missing SCRIPTS table on live DB2, not 5.5.
- **Ops:** keep `PKG_CONFIG_PATH`/`PATH` preferring `/usr/local` for builds.

### 2026-08-09 — Post-closeout: mka PIE + static liblua

- **Symptom:** `mka` / test_01 failed linking `hydrogen_coverage` (and release
  before `-no-pie`):  
  `liblua.a(lapi.o): relocation R_X86_64_32S against ... can not be used when making a PIE object`.
- **Why only some targets:** regular/debug/perf/valgrind use `-no-pie`; coverage
  uses `-fPIE`/`-pie` (GDB core analysis). Release defaulted to toolchain PIE.
- **Root cause:** upstream `make linux` installs non-PIC `liblua.a` to `/usr/local`.
- **Host fix (preferred):** rebuild 5.5.1 with PIC and reinstall:
  `make linux MYCFLAGS='-fPIC' && sudo make install` (plus existing `lua.pc`).
- **CMake:** release now passes `-no-pie -rdynamic` like other non-coverage
  variants; coverage still requires PIC `liblua.a`.
