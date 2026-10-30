# AGENTS.md — Philement repo guide for automated agents

This is the first file an automated coding agent should read when dropped onto a fresh clone of `500Foods/Philement`. It points you at the elements that actually do work, the docs that matter, the build/test workflow, and the conventions the project enforces. If a pointer below conflicts with a more specific per-element `AGENTS.md` or guide, the specific document wins (Hydrogen's map is [`elements/001-hydrogen/AGENTS.md`](/elements/001-hydrogen/AGENTS.md) and its contract is [`docs/H/INSTRUCTIONS.md`](/docs/H/INSTRUCTIONS.md); Helium's is [`elements/002-helium/AGENTS.md`](/elements/002-helium/AGENTS.md); Lithium's is [`elements/003-lithium/AGENTS.md`](/elements/003-lithium/AGENTS.md)).

## When the user names an element

Open the session at the Philement root unless the note below says otherwise. A session started there loads this file. Grok also loads every `AGENTS.md` on the path from the repo root to the working directory, and the deeper file wins on a conflict. Naming an element does not move the working directory.

When the user says they are working on an element, read that row before any other project file. Do not wait for them to name the documents.

| They say | Read first |
| --- | --- |
| hydrogen, H, 001 | [`elements/001-hydrogen/AGENTS.md`](/elements/001-hydrogen/AGENTS.md), then the docs it names |
| helium, He, 002, a migration | [`elements/002-helium/AGENTS.md`](/elements/002-helium/AGENTS.md) |
| lithium, Li, 003 | [`elements/003-lithium/AGENTS.md`](/elements/003-lithium/AGENTS.md), then the file it names for the task |
| any other element | that element's `README.md`. Stop if the status line says the element is still an idea |

A task that crosses elements stays at the repo root and reads each named guide. Also read [`docs/PLANNING.md`](/docs/PLANNING.md) when the task is a phase of a plan.

Start inside an element directory only when that session stays there and its `AGENTS.md` should be in the prompt from the first message:

- Lithium-only work: `elements/003-lithium`. The guide is the session map.
- A migration with no other tree: `elements/002-helium`.
- Hydrogen, including its contract docs: `elements/001-hydrogen/hydrogen` (`cdh`, then launch Grok). That loads this file, [`elements/001-hydrogen/AGENTS.md`](/elements/001-hydrogen/AGENTS.md), and the symlinks in `elements/001-hydrogen/hydrogen/.grok/rules/` (`INSTRUCTIONS.md`, `TESTING.md`, `TESTING_UNITY.md`). Launching from the Philement root does not load those three.

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
│   ├── PLANNING.md          # how a phased plan is worked, one phase per conversation
│   ├── H/                   # Hydrogen (the big one)
│   ├── He/                  # Helium (Lua migrations)
│   ├── Li/                  # Lithium (JS SPA)
│   └── V/                   # Vanadium (fonts)
└── elements/                # source trees, named 001-…-name
    ├── 001-hydrogen/        # C server; agent map is AGENTS.md, contract is docs/H/INSTRUCTIONS.md
    │   └── hydrogen/        # the actual project (src/, tests/, extras/, payloads/)
    ├── 002-helium/          # Lua migrations + SchemaTool/SchemaHelper (has its OWN AGENTS.md)
    ├── 003-lithium/         # JS SPA (has its OWN AGENTS.md — start there)
    ├── 004-beryllium …      # gcode / 3D-printer-specific elements
    └── 023-vanadium … 026-iron
```

Docs use **absolute links** rooted at the repo (`/docs/H/...`, `/elements/...`). Do not invent relative links like `../../../` — Hydrogen's markdownlint (Test 90) flags them.

## The elements

Elements are grouped by readiness. **Check each element's `README.md`** before touching it — most have a status line.

| Element | Path | Maturity | Docs root | Notes |
|---|---|---|---|---|
| **001 Hydrogen** | `elements/001-hydrogen/hydrogen/` | active / primary | [`docs/H/README.md`](/docs/H/README.md) | C server; map: [`elements/001-hydrogen/AGENTS.md`](/elements/001-hydrogen/AGENTS.md); contract: [`docs/H/INSTRUCTIONS.md`](/docs/H/INSTRUCTIONS.md) |
| **002 Helium** | `elements/002-helium/` | active (data layer) | [`docs/He/README.md`](/docs/He/README.md) | Lua migrations; **[`elements/002-helium/AGENTS.md`](/elements/002-helium/AGENTS.md) is authoritative for migration numbers, packs, and lookup ids** |
| **003 Lithium** | `elements/003-lithium/` | active (UI) | [`docs/Li/README.md`](/docs/Li/README.md) | Vanilla JS SPA; **[`elements/003-lithium/AGENTS.md`](/elements/003-lithium/AGENTS.md) is element-specific and authoritative for UI work** |
| 004 Beryllium | `elements/004-beryllium/` | 🏆 usable | [`elements/004-beryllium/README.md`](/elements/004-beryllium/README.md) | gcode handling |
| 005 Boron … 026 Iron | `elements/005-*` … `elements/026-iron` | 💡🔨 ideas/stubs | per-element `README.md` | Read the README before assuming anything is "live" |

Key: 💡 Idea/Planning, 🔨 Working on it, 🏆 Usable but incomplete.

> **This guide is Hydrogen-centric.** Hydrogen (001) is where the active plans and most conventions live. Other elements have their own conventions, toolchains, and (where present) their own `AGENTS.md` files; their docs may reference aliases/conventions that differ from the Hydrogen ones below, so check an element's `README.md` and docs first. Helium and Lithium each ship an element-specific `AGENTS.md`. Helium still follows Hydrogen's payload/build/test flow.

## Where work is organized: plans and the backlog

There is one canonical action backlog and phased plans.

- **Active backlog:** [`docs/H/TODO.md`](/docs/H/TODO.md) — prioritized incomplete Hydrogen work only (effort/done/remaining/code paths). Start here for "what should I do next."
- **Plan index:** [`docs/H/plans/README.md`](/docs/H/plans/README.md). Active plans include **AUTH_FINALE** (P0), **UNITY_ASAN**; completed plans live in `plans/complete/` as `*_COMPLETE.md`. Chat Finale: [`CHAT_FINALE_COMPLETE.md`](/docs/H/plans/complete/CHAT_FINALE_COMPLETE.md). SchemaHelper v2: [`SCHEMAHELPER_V2_COMPLETE.md`](/docs/H/plans/complete/SCHEMAHELPER_V2_COMPLETE.md).
- **Completed plans are history:** files in [`docs/H/plans/complete/`](/docs/H/plans/complete/) and any `*_SUPERSEDED.md` are archives. Implement against the active plan file named as the only current one, not the archives.
- **All repo docs:** [`docs/H/SITEMAP.md`](/docs/H/SITEMAP.md) (markdown index), [`docs/H/STRUCTURE.md`](/docs/H/STRUCTURE.md) (file index). Use these to avoid reinventing a file that already exists.

### How plans are run (critical for agents)

Phased work follows [`docs/PLANNING.md`](/docs/PLANNING.md). Read it at the start of any session that opens a plan. The short form:

1. **One phase per conversation.** Confirm the prior phase's Status and Exit gate. Discuss the current phase and ask qualifying questions before editing. The locks and the current phase body are the contract. Working-log entries keep the numbers of the day they were written.
2. **Wait for approval** before editing source on a phase that has not been approved. A small fix with no plan file follows [`INSTRUCTIONS.md`](/docs/H/INSTRUCTIONS.md).
3. **Close the phase before marking it complete.** The named verification has passed (`mkt` / `mku <base>` / `mkp` / `mks` / the named test). Then write Status, Accomplished, Lessons learned, and Handoff, and append the Working Log. "Intent to verify" is not verification. Then stop. The next phase starts in a new conversation.
4. **A sitting that ends early** sets Status to in progress, names the last finished work item, and does not mark the phase complete.
5. **Never apply a database migration.** Prepare the packet and hand it to the user. Do not run `schematool` or `schemahelper` apply.

[`CHAT_FINALE_COMPLETE.md`](/docs/H/plans/complete/CHAT_FINALE_COMPLETE.md) closed under this arrangement. New and amended plans follow [`docs/PLANNING.md`](/docs/PLANNING.md).

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

Session map: [`elements/001-hydrogen/AGENTS.md`](/elements/001-hydrogen/AGENTS.md). If you are asked to change the project, it is almost certainly Hydrogen. The build/test conventions below — the `mkt`/`mkp`/`mks`/`mku`/`mka` aliases, the Unity + blackbox test suite, and the no-`static`-in-`src/` rule — are **Hydrogen-specific**; other elements (notably Lithium) use their own toolchains.

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

Start at [`elements/002-helium/AGENTS.md`](/elements/002-helium/AGENTS.md). It points at **Designs, packs, and numbers** in [`docs/He/GUIDE.md`](/docs/He/GUIDE.md). Read that section before choosing a file number, a QueryRef, or a lookup id.

- **Lua migrations** for multiple designs. Acuranzo owns the product database (migration numbers 1000–1999, lookup ids 0–199). Argent is an optional 2xxx pack on that same database. Its file numbers, caller-facing QueryRefs, and lookup ids all use 2000–2999 and may share integers. Courses is the 4000–4999 pack on that same database. Its file numbers, caller-facing QueryRefs, and lookup ids all use 4000–4999 and may share integers (`courses_4000.lua` through `courses_4075.lua`; caller-facing QueryRefs already used are 4000–4007; Lookup 055 keeps family id 055). The full suite string is `PAYLOAD:acuranzo+courses+argent`. Courses alone is rejected. The archive rebuilt by `mka` on 2026-10-16 contains the Courses files. Tests 32–40, 62, and 73 use that string. `PAYLOAD:acuranzo` and `PAYLOAD:acuranzo+argent` stay valid and omit the pack. A database that already applied the old 1xxx course files must be rebuilt before this pack is applied. GLM and today's Gaius tree stay on their own databases. `elements/002-helium/helium/database.lua` is the one macro set, packed once as `helium/database.lua`. It is not a design. Primary target is PostgreSQL 15+ via YugabyteDB.
- The migration number, `query_id`, and a caller-facing QueryRef are three different values. The bookkeeping rows store the migration number in `query_ref`. `query_id` is `MAX+1`. A caller-facing QueryRef is `cfg.QUERY_REF`.
- Migrations are **embedded into the Hydrogen payload** and applied by Hydrogen's AutoMigrations — so **after changing a migration you must regenerate the payload** (`payload-generate.sh` or `mka`) before running migration tests (30–38, 71). `mkt` does not refresh the archive.
- Authors then read, in order: [`docs/He/GUIDE.md`](/docs/He/GUIDE.md) → [`docs/He/MIGRATION_ANATOMY.md`](/docs/He/MIGRATION_ANATOMY.md) → [`docs/He/MACRO_REFERENCE.md`](/docs/He/MACRO_REFERENCE.md).
- **SchemaTool** ([docs/H/tools/SCHEMATOOL.md](/docs/H/tools/SCHEMATOOL.md), `extras/schematool/`) audits migration drift (Lua migrations vs live `queries`); **SchemaHelper** ([docs/H/tools/SCHEMAHELPER.md](/docs/H/tools/SCHEMAHELPER.md), `extras/schematool/schemahelper.lua`) is its interactive Lua TUI front-end. `mks`/luacheck (Test 98) covers the Lua.
- Never hand-edit a production DB as the source of truth; migrations are source of truth, and `schemahelper` apply steps are run by a human, not the agent.

## Lithium (003) — JS SPA

Lithium has its own, detailed [`elements/003-lithium/AGENTS.md`](/elements/003-lithium/AGENTS.md) which is **authoritative for all UI work** — it covers the manager/ID map, boot sequence, known defects, the Vite/Vitest/ESLint toolchain, Conduit QueryRefs, and the Hydrogen OIDC auth flow. Use `npm test` (Vitest) and `npm run lint` from `elements/003-lithium/`.

## Other elements

For any element 004+, **read its `README.md` in place of assuming activity.** Most have a status emoji and a pointer to further docs. Vanadium (023, custom Iosevka fonts) is the most "complete" of the non-core ones and is self-contained under `elements/023-vanadium/`.

## Documentation map (start here, not with Google)

- **Master cross-element index:** [`docs/README.md`](/docs/README.md)
- **Phased plans:** [`docs/PLANNING.md`](/docs/PLANNING.md) — one phase per conversation, qualifying questions, and the close-out written before a phase is marked complete
- **All Hydrogen docs:** [`docs/H/README.md`](/docs/H/README.md) (table of contents)
- **Markdown index (Hydrogen-centric):** [`docs/H/SITEMAP.md`](/docs/H/SITEMAP.md)
- **Every file in the repo:** [`docs/H/STRUCTURE.md`](/docs/H/STRUCTURE.md)
- **Helium docs:** [`docs/He/README.md`](/docs/He/README.md) · agent map [`elements/002-helium/AGENTS.md`](/elements/002-helium/AGENTS.md) · **Lithium docs:** [`docs/Li/README.md`](/docs/Li/README.md)
- **AI/human dev guide (Hydrogen):** [`docs/H/INSTRUCTIONS.md`](/docs/H/INSTRUCTIONS.md) and [`docs/H/PROMPTS.md`](/docs/H/PROMPTS.md)
- **Architecture:** [`docs/H/core/README.md`](/docs/H/core/README.md), [`docs/H/core/ARCHITECTURE.md`](/docs/H/core/ARCHITECTURE.md)
- **API reference:** [`docs/H/core/API_OVERVIEW.md`](/docs/H/core/API_OVERVIEW.md), [`docs/H/api/`](/docs/H/api)
- **Subsystem deep-dives:** [`docs/H/core/subsystems/`](/docs/H/core/subsystems), e.g. [`mcp/mcp.md`](/docs/H/core/subsystems/mcp/mcp.md), [`scripting/lua_api.md`](/docs/H/core/subsystems/scripting/lua_api.md)
- **Deployment/run:** [`docs/H/DEPLOYMENT.md`](/docs/H/DEPLOYMENT.md), [`docs/H/SETUP.md`](/docs/H/SETUP.md), [`docs/H/SECRETS.md`](/docs/H/SECRETS.md)

## Common "first questions" answered

- **"What should I work on?"** → [`docs/H/TODO.md`](/docs/H/TODO.md). P0 is Auth Finale ([`docs/H/plans/AUTH_FINALE.md`](/docs/H/plans/AUTH_FINALE.md)). Chat Finale is complete ([`CHAT_FINALE_COMPLETE.md`](/docs/H/plans/complete/CHAT_FINALE_COMPLETE.md)).
- **"How does a phased plan run?"** → [`docs/PLANNING.md`](/docs/PLANNING.md). One phase per conversation. Ask qualifying questions, then write Status, Accomplished, Lessons learned, Handoff, and the Working Log before marking the phase complete.
- **"Where is the C code?"** → `elements/001-hydrogen/hydrogen/src/`. `src/hydrogen.h` is the umbrella header.
- **"How do I build?"** → `zsh -ic 'mkt'` (trial) or `mka` (all targets). On a clean box, set `HYDROGEN_ROOT` and `HELIUM_ROOT` and run `elements/001-hydrogen/hydrogen/extras/make-trial.sh`.
- **"How do I test one thing?"** → `zsh -ic 'mku <base_test_name>'` builds + runs a single Unity test (tab-completion is wired up). Blackbox tests are `tests/test_NN_*.sh`. The full orchestrator is `tests/test_00_all.sh`.
- **"How do I run C lint?"** → `zsh -ic 'mkp'` (cppcheck, Test 91).
- **"How do I lint my shell script?"** → `zsh -ic 'mks'` (shellcheck, Test 92).
- **"I changed a migration — why do tests fail?"** → regenerate the payload first (`payload-generate.sh` or `zsh -ic 'mka'`), then re-run migration tests. `mkt` does not refresh the archive. The binary embeds the migrations.
- **"I need to check if a function is dead code."** → run `mkt`; the linker-based gate writes `elements/001-hydrogen/hydrogen/build/deadcode/dead_functions.txt`. Do **not** baseline chat-named dead functions — drive them to zero (see CHAT_FINALE Phase 5).
- **"How do I add a DB migration?"** → [`elements/002-helium/AGENTS.md`](/elements/002-helium/AGENTS.md), then **Designs, packs, and numbers** in [`docs/He/GUIDE.md`](/docs/He/GUIDE.md), then `MIGRATION_ANATOMY.md` and `MACRO_REFERENCE.md`. Generate the packet via SchemaTool; **do not apply it yourself** — hand it to a human.
- **"Am I allowed to create a new test script?"** → only if explicitly asked. Prefer extending existing tests and following their numbering/conventions.

## Pull requests

Only commit/push/create PRs when explicitly asked. When you do:

- Work against `main` (the default branch); open PRs from a named branch.
- Stage only the files you intended; the Hydrogen `.gitignore` already ignores `build/`.
- Run the relevant verification (`mkt`/`mkp`/`mks`/`mku`) and report green output.
- Match commit-message tone to recent history (the log mixes planning notes and "Apply automatic changes" housekeeping).
- Do not commit secrets, JWTs, or `node_modules` (Lithium and the hbm browser have their own).
