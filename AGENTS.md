# AGENTS.md — Philement repo guide for automated agents

This is the first file an automated coding agent should read when dropped onto a fresh clone of `500Foods/Philement`. It points you at the elements that actually do work, the docs that matter, the build/test workflow, and the conventions the project enforces. If a pointer below conflicts with a more specific per-element `AGENTS.md` or guide, the specific document wins (Hydrogen's AI guide is [`docs/H/INSTRUCTIONS.md`]; Lithium's is [`elements/003-lithium/AGENTS.md`](/elements/003-lithium/AGENTS.md)).

## What is Philement, in 30 seconds

Philement is a **monorepo of small "elements"**, each named after a periodic-table element, that together aim to be a local, scriptable appliance runtime for 3D printing (Klipper + Moonraker-style) and beyond. The host you will touch most is **Hydrogen** (element 001): a compact, multithreaded C server (web, WebSocket, OIDC, database clients, MCP). Almost all extensibility is **Lua** (sandboxed host API `H.*`) or **data/migrations** — there is no plugin folder and no `pip install` into the host. The C is intended to change in normal patches, never hacks bolted on the side.

> The repo is large and lopsided on purpose: Hydrogen + Helium carry almost everything; many later-numbered elements are ideas or stubs. Do not assume work there is "in progress" — check the element README's status first.

## Repo layout (as of 2026-08-30)

```text
Philement/
├── AGENTS.md                # you are here — repo-wide agent orientation
├── README.md                # human-facing repo overview + element table
├── RELEASES.md              # release history / upgrade instructions
├── LICENSE.md
├── .gitignore / .gitattributes
├── .github/workflows/       # CI (main.yml refreshes the cloc badge)
├── releases/                # tagged release artifacts (2024-07 … 2026-08)
├── docs/                    # all documentation lives under docs/, namespaced per element
│   ├── README.md            # MASTER index across all elements
│   ├── H/                   # Hydrogen (the big one)
│   ├── He/                  # Helium (Lua migrations)
│   ├── Li/                  # Lithium (JS SPA)
│   └── V/                   # Vanadium (fonts)
└── elements/                # source trees, named 001-…-name
    ├── 001-hydrogen/        # C server + its own AGENTS-like AI guide
    │   └── hydrogen/        # the actual project (src/, tests/, extras/, payloads/)
    ├── 002-helium/          # Lua migrations + SchemaTool/SchemaHelper
    ├── 003-lithium/         # JS SPA (has its OWN AGENTS.md — start there)
    ├── 004-beryllium …      # gcode / 3D-printer-specific elements
    └── 023-vanadium … 026-iron
```

Docs use **absolute links** rooted at the repo (`/docs/H/...`, `/elements/...`). Do not invent relative links like `../../../` — Hydrogen's markdownlint (Test 90) flags them.

## The elements

Elements are grouped by readiness. **Check each element's `README.md`** before touching it — most have a status line.

| Element | Path | Maturity | Docs root | Notes |
|---|---|---|---|---|
| **001 Hydrogen** | `elements/001-hydrogen/hydrogen/` | active / primary | [`docs/H/README.md`](/docs/H/README.md) | C server; AI guide: [`docs/H/INSTRUCTIONS.md`](/docs/H/INSTRUCTIONS.md) |
| **002 Helium** | `elements/002-helium/` | active (data layer) | [`docs/He/README.md`](/docs/He/README.md) | Lua migrations for multiple DB engines |
| **003 Lithium** | `elements/003-lithium/` | active (UI) | [`docs/Li/README.md`](/docs/Li/README.md) | Vanilla JS SPA; **[`elements/003-lithium/AGENTS.md`](/elements/003-lithium/AGENTS.md) is element-specific and authoritative for UI work** |
| 004 Beryllium | `elements/004-beryllium/` | 🏆 usable | [`elements/004-beryllium/README.md`](/elements/004-beryllium/README.md) | gcode handling |
| 005 Boron … 026 Iron | `elements/005-*` … `elements/026-iron` | 💡🔨 ideas/stubs | per-element `README.md` | Read the README before assuming anything is "live" |

Key: 💡 Idea/Planning, 🔨 Working on it, 🏆 Usable but incomplete.

> **This guide is Hydrogen-centric.** Hydrogen (001) is where the active plans and most conventions live. Other elements have their own conventions, toolchains, and (where present) their own `AGENTS.md` files; their docs may reference aliases/conventions that differ from the Hydrogen ones below, so check an element's `README.md` and docs first. Today only Lithium ships an element-specific `AGENTS.md`; Helium follows Hydrogen's payload/build/test flow but has its own Lua authoring rules.

## Where work is organized: plans and the backlog

There is one canonical action backlog and phased plans.

- **Active backlog:** [`docs/H/TODO.md`](/docs/H/TODO.md) — prioritized incomplete Hydrogen work only (effort/done/remaining/code paths). Start here for "what should I do next."
- **Plan index:** [`docs/H/plans/README.md`](/docs/H/plans/README.md). Active plans include **CHAT_FINALE** (P0), **AUTH_FINALE** (P0), **MAILRELAY_PLAN**, **SCHEMAHELPER**, **UNITY_ASAN**; completed plans live in `plans/complete/` as `*_COMPLETE.md`.
- **Completed plans are history:** files in [`docs/H/plans/complete/`](/docs/H/plans/complete/) and any `*_SUPERSEDED.md` are archives. Implement against the active plan file named as the only current one, not the archives.
- **All repo docs:** [`docs/H/SITEMAP.md`](/docs/H/SITEMAP.md) (markdown index), [`docs/H/STRUCTURE.md`](/docs/H/STRUCTURE.md) (file index). Use these to avoid reinventing a file that already exists.

### How plans are run (critical for agents)

Plans are **phase-gated and each phase is its own conversation**:

1. **Confirm the prior phase is actually done** — re-read its Status block; do not trust memory of a previous session.
2. **Discuss the current phase first** — read its Goal + Work items + Done means + Exit gate; grep/read code **before** writing anything.
3. **Get explicit approval before editing source** when work is under an active phased plan (`docs/H/plans/`, e.g. [`CHAT_FINALE`](/docs/H/plans/CHAT_FINALE.md)). For a small isolated fix with no plan file in play, follow [`INSTRUCTIONS.md`](/docs/H/INSTRUCTIONS.md) and the tests below. Migration packets are handed to the user (see item 7), not applied here.
4. **Ask questions as they arise** rather than guessing at ambiguous requirements.
5. **Mark work items `[x]` and phase Status "complete" only after the named verification command actually passed** (`mkt`/`mku <base>`/`mkp`/`mks`/the named blackbox test). "Intent to verify" is not verification.
6. **Update the phase's Status block and Working Log** at major milestones (not only at the end), and record lessons learned.
7. **Never apply a database migration.** If a phase needs a Helium seed/schema/migration packet, prepare/generate it and hand it to the user to apply; do not run `schematool`/`schemahelper` apply steps yourself.

These rules are baked into [`docs/H/plans/CHAT_FINALE.md`](/docs/H/plans/CHAT_FINALE.md) and apply repo-wide to phased work.

## Environment & build tooling

Aliases live in the developer's `~/.zshrc` and are **not in the repo** — on a fresh clone they are absent. They wrap scripts in `elements/001-hydrogen/hydrogen/extras/` (`make-trial.sh` → `mkt`, `make-all.sh` → `mka`, `run-unity-test.sh <name>` → `mku`, `mku_completion.zsh`) and `elements/001-hydrogen/hydrogen/tests/` (`test_91_cppcheck.sh` → `mkp`, `test_92_shellcheck.sh` → `mks`). On a clean box, export `PHILEMENT_ROOT`, `HYDROGEN_ROOT`, and `HELIUM_ROOT` and call those scripts directly.

To run the same aliases non-interactively (e.g. from an agent shell):

```bash
zsh -ic 'mkt'        # trial build (C)
zsh -ic 'mkp'        # cppcheck (C lint, Test 91)
zsh -ic 'mks'        # shellcheck (Bash lint, Test 92)
zsh -ic 'mka'        # build-all (Test 01 compilation)
zsh -ic 'mku beryllium_test_analyze_gcode'   # build + run one Unity test
zsh -ic 'mkl'        # markdown link check (Test 04)
```

All Hydrogen build scripts require `HYDROGEN_ROOT` and `HELIUM_ROOT` to be set. The working directory for Hydrogen tasks is `elements/001-hydrogen/hydrogen/`. Ignore local editor/agent worktree directories — they are gitignored and are not part of the project.

## Hydrogen (001) — the workhorse

If you are asked to change the project, it is almost certainly Hydrogen. The build/test conventions below — the `mkt`/`mkp`/`mks`/`mku`/`mka` aliases, the Unity + blackbox test suite, and the no-`static`-in-`src/` rule — are **Hydrogen-specific**; other elements (notably Lithium) use their own toolchains.

- **Source:** [`elements/001-hydrogen/hydrogen/src/`](/elements/001-hydrogen/hydrogen/src) — subsystems (`api/`, `config/`, `database/`, `launch/`, `logging/`, `mcp/`, `network/`, `oidc/`, `print/`, `queue/`, `scripting/`, `state/`, `threads/`, `utils/`, `webserver/`, `websocket/`, `mailrelay/`, …) and `src/hydrogen.h` (included first in every `.c`).
- **CMake:** `elements/001-hydrogen/hydrogen/cmake/` (`CMakeLists.txt` for all builds including Unity).
- **Build artifacts:** `elements/001-hydrogen/hydrogen/build/` (gitignored).

### Tests (two complementary approaches)

- **Unity unit tests** (`elements/001-hydrogen/hydrogen/tests/unity/`) — C unit tests, one file per function, 75% line coverage target. Test files are named uniquely — search before creating a new one.
- **Blackbox / integration** (`elements/001-hydrogen/hydrogen/tests/test_NN_*.sh`) — run the real binary + embedded payload; 60% target (lower by design — it covers happy paths and code unit tests can't reach).
- **Combined target: 85%** (either Unity OR blackbox).
- **Orchestrator:** `tests/test_00_all.sh` runs the whole suite. Individual tests are numbered; the lint/coverage ones: `test_89` coverage, `test_90` markdownlint, `test_91` cppcheck, `test_92` shellcheck, `test_93` jsonlint, `test_94` eslint, `test_95` stylelint, `test_96` htmlhint, `test_97` xmlstarlet, `test_98` luacheck, `test_99` code size/cloc.
- **Database tests** (30–38, 40–47, 71) need live DB engines; tests 32–38 are per-engine migrations.

### Coding standards (session-critical)

Full C, Bash, and Markdown rules live in [`docs/H/INSTRUCTIONS.md`](/docs/H/INSTRUCTIONS.md); skim it before editing Hydrogen source. The essentials that abort a session if missed:

- **No `static` functions in `src/`.** Unity links object files directly and cannot call `static` helpers; `mkt` fails the build on any **new** `static` function. File-scope *state* only. Grandfathered set: `tests/.static-baseline.txt`.
- **Headers first.** `src/hydrogen.h` is included first in every `.c`; every function needs a visible prototype in a header.
- **Includes:** `#include <src/folder/...>`, not `"../../..."`.
- **`jq` for JSON**, never `grep`/text tools.
- **Lint clean:** cppcheck all-directives (Test 91, `mkp`) and shellcheck all-directives (Test 92, `mks`) — fix, don't suppress.
- **`CHANGELOG`/`TEST_VERSION`** at the top of every script after a change.
- **Markdown:** absolute links rooted at `/docs/H` or `/elements/...`; no `:line` refs; run Test 04.
- **Verify:** `mkt` after any C change, `mkp` after that, `mks` after any script change. `mkt` is also the dead-code gate (`build/deadcode/dead_functions.txt`).

For Lithium UI work, follow the element-specific [`elements/003-lithium/AGENTS.md`](/elements/003-lithium/AGENTS.md) and [`docs/Li/LITHIUM-INS.md`](/docs/Li/LITHIUM-INS.md) instead.

## Helium (002) — migrations

- **Lua migrations** for multiple designs (Acuranzo = product DB; plus GAIUS, GLM, Helium). Primary target is PostgreSQL 15+ via YugabyteDB.
- Migrations are **embedded into the Hydrogen payload** and applied by Hydrogen's AutoMigrations — so **after changing a migration you must rebuild the payload** (`mkt`/`mka`) before running migration tests (30–38, 71).
- Authors should read, in order: [`docs/He/GUIDE.md`](/docs/He/GUIDE.md) → [`docs/He/MIGRATION_ANATOMY.md`](/docs/He/MIGRATION_ANATOMY.md) → [`docs/He/MACRO_REFERENCE.md`](/docs/He/MACRO_REFERENCE.md).
- **SchemaTool** ([docs/H/tools/SCHEMATOOL.md](/docs/H/tools/SCHEMATOOL.md), `extras/schematool/`) audits migration drift (Lua migrations vs live `queries`); **SchemaHelper** ([docs/H/tools/SCHEMAHELPER.md](/docs/H/tools/SCHEMAHELPER.md), `extras/schematool/schemahelper.lua`) is its interactive Lua TUI front-end. `mks`/luacheck (Test 98) covers the Lua.
- Never hand-edit a production DB as the source of truth; migrations are source of truth, and `schemahelper` apply steps are run by a human, not the agent.

## Lithium (003) — JS SPA

Lithium has its own, detailed [`elements/003-lithium/AGENTS.md`](/elements/003-lithium/AGENTS.md) which is **authoritative for all UI work** — it covers the manager/ID map, boot sequence, known defects, the Vite/Vitest/ESLint toolchain, Conduit QueryRefs, and the Hydrogen OIDC auth flow. Use `npm test` (Vitest) and `npm run lint` from `elements/003-lithium/`.

## Other elements

For any element 004+, **read its `README.md` in place of assuming activity.** Most have a status emoji and a pointer to further docs. Vanadium (023, custom Iosevka fonts) is the most "complete" of the non-core ones and is self-contained under `elements/023-vanadium/`.

## Documentation map (start here, not with Google)

- **Master cross-element index:** [`docs/README.md`](/docs/README.md)
- **All Hydrogen docs:** [`docs/H/README.md`](/docs/H/README.md) (table of contents)
- **Markdown index (Hydrogen-centric):** [`docs/H/SITEMAP.md`](/docs/H/SITEMAP.md)
- **Every file in the repo:** [`docs/H/STRUCTURE.md`](/docs/H/STRUCTURE.md)
- **Helium docs:** [`docs/He/README.md`](/docs/He/README.md) · **Lithium docs:** [`docs/Li/README.md`](/docs/Li/README.md)
- **AI/human dev guide (Hydrogen):** [`docs/H/INSTRUCTIONS.md`](/docs/H/INSTRUCTIONS.md) and [`docs/H/PROMPTS.md`](/docs/H/PROMPTS.md)
- **Architecture:** [`docs/H/core/README.md`](/docs/H/core/README.md), [`docs/H/core/ARCHITECTURE.md`](/docs/H/core/ARCHITECTURE.md)
- **API reference:** [`docs/H/core/API_OVERVIEW.md`](/docs/H/core/API_OVERVIEW.md), [`docs/H/api/`](/docs/H/api)
- **Subsystem deep-dives:** [`docs/H/core/subsystems/`](/docs/H/core/subsystems), e.g. [`mcp/mcp.md`](/docs/H/core/subsystems/mcp/mcp.md), [`scripting/lua_api.md`](/docs/H/core/subsystems/scripting/lua_api.md)
- **Deployment/run:** [`docs/H/DEPLOYMENT.md`](/docs/H/DEPLOYMENT.md), [`docs/H/SETUP.md`](/docs/H/SETUP.md), [`docs/H/SECRETS.md`](/docs/H/SECRETS.md)

## Common "first questions" answered

- **"What should I work on?"** → [`docs/H/TODO.md`](/docs/H/TODO.md). P0 is Chat Finale ([`docs/H/plans/CHAT_FINALE.md`](/docs/H/plans/CHAT_FINALE.md)) and Auth Finale ([`docs/H/plans/AUTH_FINALE.md`](/docs/H/plans/AUTH_FINALE.md)).
- **"Where is the C code?"** → `elements/001-hydrogen/hydrogen/src/`. `src/hydrogen.h` is the umbrella header.
- **"How do I build?"** → `zsh -ic 'mkt'` (trial) or `mka` (all targets). On a clean box, set `HYDROGEN_ROOT` and `HELIUM_ROOT` and run `elements/001-hydrogen/hydrogen/extras/make-trial.sh`.
- **"How do I test one thing?"** → `zsh -ic 'mku <base_test_name>'` builds + runs a single Unity test (tab-completion is wired up). Blackbox tests are `tests/test_NN_*.sh`. The full orchestrator is `tests/test_00_all.sh`.
- **"How do I run C lint?"** → `zsh -ic 'mkp'` (cppcheck, Test 91).
- **"How do I lint my shell script?"** → `zsh -ic 'mks'` (shellcheck, Test 92).
- **"I changed a migration — why do tests fail?"** → rebuild the payload first: `zsh -ic 'mkt'` (or `mka`), then re-run migration tests. The binary embeds the migrations.
- **"I need to check if a function is dead code."** → run `mkt`; the linker-based gate writes `elements/001-hydrogen/hydrogen/build/deadcode/dead_functions.txt`. Do **not** baseline chat-named dead functions — drive them to zero (see CHAT_FINALE Phase 5).
- **"How do I add a DB migration?"** → read `docs/He/GUIDE.md` + `MIGRATION_ANATOMY.md` + `MACRO_REFERENCE.md`. Generate the packet via SchemaTool; **do not apply it yourself** — hand it to a human.
- **"Am I allowed to create a new test script?"** → only if explicitly asked. Prefer extending existing tests and following their numbering/conventions.

## Pull requests

Only commit/push/create PRs when explicitly asked. When you do:

- Work against `main` (the default branch); open PRs from a named branch.
- Stage only the files you intended; the Hydrogen `.gitignore` already ignores `build/`.
- Run the relevant verification (`mkt`/`mkp`/`mks`/`mku`) and report green output.
- Match commit-message tone to recent history (the log mixes planning notes and "Apply automatic changes" housekeeping).
- Do not commit secrets, JWTs, or `node_modules` (Lithium and the hbm browser have their own).
