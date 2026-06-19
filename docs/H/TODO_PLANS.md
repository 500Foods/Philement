# TODO: Incomplete Plans

This document catalogs all `_PLAN.md` documents with incomplete or in-progress implementation items.

---

## Priority 1 — Active Implementation (In Progress)

### 1. OIDC Plan — Phase 26: Lithium `auth.last_method` and subtle highlighting
**Status:** ⏳ IN PROGRESS
**Document:** [`docs/OIDC-PLAN.md`](docs/OIDC-PLAN.md#L4923)

Remaining incomplete items:
- [ ] **Lint/build/test green** — Manual smoke test against running Lithium+Hydrogen dev environment
- [ ] **Setting persists across reloads** — Manual verification in dev environment

**Note:** Phases 1-25 are complete. Phase 27 (e2e against real Keycloak) follows, but Phase 26 must gate first.

---

### 2. OIDC Plan — Phase 27: End-to-end against real Keycloak (dev environment)
**Status:** ⏳ IN PROGRESS
**Document:** [`docs/OIDC-PLAN.md`](docs/OIDC-PLAN.md#L4949)

Remaining incomplete items:
- [ ] Manual test plan all ticked (7 items: password login, OIDC login, new user provisioning, JWT renewal, logout, managers work identically, JWT renewal mid-session)
- [ ] One independent reviewer runs through manual plan
- [ ] [`docs/Li/LITHIUM-OIDC.md`](docs/Li/LITHIUM-OIDC.md) merged
- [ ] [`docs/H/api/auth/oidc_rp.md`](docs/H/api/auth/oidc_rp.md) merged
- [ ] `test_04_check_links.sh` green after new docs added

**Note:** Phases 1-26 complete. All implementation code shipped; remaining work is operator configuration and manual test checklist.

---

### 3. Chat Plan — Phase 13: Advanced Features
**Status:** ⏳ IN PROGRESS
**Document:** [`docs/H/plans/CHAT_PLAN_PHASE_13.md`](docs/H/plans/CHAT_PLAN_PHASE_13.md)

Work items - most unchecked, Streaming Architecture completed (2026-03-25):
- [ ] Implement Function Calling support
- [ ] Implement Response Caching
- [ ] Implement Load Balancing across API keys
- [ ] Implement Fallback Engines
- [ ] Implement Usage Analytics Dashboard
- [ ] Implement Prompt Templates
- [ ] Implement Conversation Management APIs
- [ ] Implement Real-time Cost Tracking
- [ ] Implement A/B Testing Framework
- [ ] Unit and Integration Tests for all advanced features

---

## Priority 2 — Database Updates (Partially Complete)

### 4. Database Parameter Support Enhancement Plan
**Status:** 🚧 PARTIALLY COMPLETE (Phases 1-4 complete)
**Document:** [`docs/H/plans/DATABASE_UPDATE_PLAN.md`](docs/H/plans/DATABASE_UPDATE_PLAN.md)

Incomplete Phase 5 items (Integration Testing):
- [ ] **2.5** Run Test 89: `./tests/test_89_coverage.sh` - Verify code coverage improvements
- [ ] **2.6** Run Test 91: `./tests/test_91_cppcheck.sh` - Ensure no new linting issues
- [ ] **2.7** Run Test 92: `./tests/test_92_shellcheck.sh` - Verify shell script quality
- [ ] **3.1** Run Test 11: `./tests/test_11_leaks_like_a_sieve.sh` - Check for memory leaks with valgrind

Incomplete Phase 6 items (Documentation and Cleanup):
- [ ] **1.1** Add/update function comments in `database_params.c`
- [ ] **1.2** Add/update function comments in `db2/query.c` for parameter binding
- [ ] **1.3** Add/update function comments in `mysql/query.c` for parameter handling
- [ ] **1.4** Add/update function comments in `sqlite/query.c` for parameter handling
- [ ] **1.5** Add/update function comments in `postgresql/query.c` for parameter handling
- [ ] **2.1** Create or update `/docs/H/database/PARAMETER_TYPES.md`
- [ ] **2.2** Create or update `/docs/H/database/PARAMETER_BINDING.md`
- [ ] **2.3** Update `docs/H/tests/test_40_auth.md` if it exists
- [ ] **3.3** Confirm code coverage targets met
- [ ] **3.4** Verify no memory leaks detected
- [ ] **3.5** Confirm cppcheck passes with no new warnings
- [ ] **3.6** Mark project as complete

---

### 5. Test 40 Authentication Debugging
**Status:** 🚧 INCOMPLETE (awaiting Test 40 execution)
**Document:** [`docs/H/plans/AUTH_PLAN_PROGRESS.md`](docs/H/plans/AUTH_PLAN_PROGRESS.md)

Current Status shows 100% overall progress, but:
- [ ] **Test 40 execution** — Tests are implemented and ready, but need to be run against live database
- [ ] Debug plan in [`TEST_40_DEBUG_PLAN.md`](docs/H/plans/TEST_40_DEBUG_PLAN.md) has extensive debugging checklist to work through

---

## Priority 3 — Lithium Table Refactor (Phases 12-19 Incomplete)

**Status:** 🚧 PARTIALLY COMPLETE (Phases 1-11 complete)
**Document:** [`docs/Li/LITHIUM-TAB-PLAN.md`](docs/Li/LITHIUM-TAB-PLAN.md)

### Phase 12 — Date/time coltypes: Luxon integration
Incomplete items:
- [ ] Add `luxon` to `package.json`
- [ ] Add entry to `LITHIUM-LIB.md`
- [ ] Design property surface (`dateFormat`, `timezone`)
- [ ] Implement custom formatter with Luxon
- [ ] Implement matching sorter for Luxon DateTime
- [ ] Update migration 1153 with default `dateFormat` values
- [ ] Tests (deferred until working)

### Phase 13 — Date/time coltypes: Flatpickr editor integration
Incomplete items:
- [ ] Add `flatpickr` to `package.json`
- [ ] Add entry to `LITHIUM-LIB.md`
- [ ] Design editor property (`pickerOptions`)
- [ ] Implement custom Tabulator editor per coltype
- [ ] Implement Flatpickr-backed header filter
- [ ] Theme Flatpickr to match dark theme
- [ ] Tests for editor mount, commit, cancel

### Phase 14 — Navigator Refresh button isolation
Incomplete items:
- [ ] Reproduce cross-instance mutation in dual-table managers
- [ ] Audit global event listeners in `src/tables/`
- [ ] Separate cache-clear from instance-reset
- [ ] Audit other nav buttons for cross-instance reach
- [ ] Test two-instance harness

### Phase 15 — Exhaustive tableDef generation
Incomplete items:
- [ ] Catalog what needs to capture
- [ ] Capture table-level runtime state (`layout`, `tableWidthMode`, `initialSort`, `_filterValues`, etc.)
- [ ] Capture column-level runtime state (width, visibility, order, group/sort priorities)
- [ ] Build `generateMigrationSeed()` helper
- [ ] Add "Copy as Migration Seed" action
- [ ] Round-trip test

### Phase 16 — Column Manager cleanup
Incomplete items:
- [ ] Audit every use of `alwaysEditable` in base class/mixins
- [ ] Confirm Column Manager uses three-stage resolution
- [ ] Confirm Column Manager uses single extractor for save-changes
- [ ] Confirm Column Manager's Navigator works the same
- [ ] Confirm Column Manager Manager loads/edits correctly
- [ ] Retire any cruft found during audit

### Phase 17 — Table-wide settings in Column Manager
Incomplete items:
- [ ] Identify table-wide properties (`layout`, width preset, readonly, persistSort, etc.)
- [ ] Design compact header section for Column Manager popup
- [ ] Implement UI and wire each field
- [ ] Ensure settings captured by Phase 15's extractor
- [ ] Tests for field-to-tableDef round-trip

### Phase 18 — CSS file audit and refactor
Incomplete items:
- [ ] Audit `lithium-table.css` for extraction candidates
- [ ] Audit `lithium-column-manager.css` similarly
- [ ] Extract along responsibility lines (Navigator, Popup, Filter, Edit-mode, Groups)
- [ ] Remove orphaned CSS rules
- [ ] Confirm no visual regressions

### Phase 19 — Test coverage and documentation closure
Incomplete items:
- [ ] Run full test suite and capture coverage
- [ ] Add integration-level tests for end-to-end flows
- [ ] Cross-check `LITHIUM-TOC.md`
- [ ] Update `LITHIUM-FAQ.md` with refactor summary
- [ ] Move plan to `Plans-Complete/`

---

## Status Summary

| Plan | Total Phases/Stages | Complete | In Progress | Incomplete |
|------|---------------------|----------|-------------|------------|
| OIDC Plan | 30 | 26 | 2 (26, 27) | 2 (28-30 not planned) |
| Chat Plan | 13 | 12 | 1 (13) | - |
| Database Update | 6 | 4 | - | 2 (5, 6) |
| Lithium Tab Refactor | 19 | 11 | - | 8 (12-19) |
| Auth Endpoints | 6 phases | 6 | - | Test 40 execution pending |
| Database Subsystem | 6 phases | 2 | - | 4 (3-6) |
| Terminal Subsystem | 6 phases | 6 | - | - |
| Mirage Proxy Network | 1 | 0 | - | 1 (implementation deferred) |

**Legend:**
- ✅ Complete phase
- ⏳ In progress (work started, gating/testing remains)
- 🚧 Partially complete (some items done, some remain)
- Empty = Not yet started

---

## Notes

### Terminal Plan
Per [`TERMINAL_PLAN.md`](docs/H/plans/TERMINAL_PLAN.md), all phases are marked complete (Phases 1-6). The terminal subsystem is fully implemented with xterm.js integration, PTY management, and WebSocket communication.

### Mirage Plan
Per [`MIRAGE_PLAN.md`](docs/H/plans/MIRAGE_PLAN.md), this is an architecture design document for a distributed proxy network. It describes the dual-server architecture (Mirage server + Remote server) but has no implementation phases defined. Implementation is deferred.