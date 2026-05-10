# Lithium Documentation Suite

> **Before working on Lithium code, read [LITHIUM-INS.md](LITHIUM-INS.md)** — mandatory coding standards enforced during code review.

This document is the master index for all Lithium documentation. Use it to find the specific file you need based on your task.

---

## Critical Implementation Notes

### Tour Manager Uses Numeric ID Matching Only

The Tour Manager matches tours to managers using **only the numeric portion** of the manager identifier:

- `"003.Profile"` matches `"003.User Profile"` because both have ID **3**
- `"029.Query Manager"` matches `"029.Queries"` because both have ID **29**
- The manager name after the dot is ignored

**See:** [LITHIUM-MGR-TOUR.md](LITHIUM-MGR-TOUR.md)

### Cannot Run Development Server Directly

Lithium requires a running Hydrogen backend. Do **not** run `npm run dev` to test changes. Instead:
- Use static analysis and code review
- Run ESLint: `npm run lint`
- Build verification: `npm run build`
- Testing is performed in CI/CD pipeline

---

## Documentation Index by Category

### Getting Started

| File | Purpose |
|------|--------|
| [LITHIUM-TOC.md](LITHIUM-TOC.md) | **This file** — Master index for all docs |
| [LITHIUM-DEV.md](LITHIUM-DEV.md) | Development environment, prerequisites, npm scripts, build tools |
| [LITHIUM-INS.md](LITHIUM-INS.md) | **⚠️ Mandatory** — Coding standards and rules for Models |
| [LITHIUM-TST.md](LITHIUM-TST.md) | Test framework (Vitest), coverage, how to add tests |
| [LITHIUM-FAQ.md](LITHIUM-FAQ.md) | Common issues, troubleshooting, lessons learned |

### Core Architecture

| File | Purpose |
|------|--------|
| [LITHIUM-MGR.md](LITHIUM-MGR.md) | Manager system overview — lifecycle, patterns |
| [LITHIUM-MGR-NEW.md](LITHIUM-MGR-NEW.md) | **⚠️ New Manager Guide** — Step-by-step checklist |
| [LITHIUM-API.md](LITHIUM-API.md) | Hydrogen API endpoints, Conduit client library |
| [LITHIUM-WSS.md](LITHIUM-WSS.md) | WebSocket connection, keepalive, status indicator |
| [LITHIUM-LUT.md](LITHIUM-LUT.md) | Lookup tables — schema, caching, debugging tips |

### Manager Implementations

| File | Purpose |
|------|--------|
| [LITHIUM-MGR-LOGIN.md](LITHIUM-MGR-LOGIN.md) | Login Manager — auth, panels, keyboard shortcuts |
| [LITHIUM-MGR-MAIN.md](LITHIUM-MGR-MAIN.md) | Main Manager — sidebar, slots, logout |
| [LITHIUM-MGR-QUERY.md](LITHIUM-MGR-QUERY.md) | Query Manager — fully implemented with LithiumTable |
| [LITHIUM-MGR-LOOKUPS.md](LITHIUM-MGR-LOOKUPS.md) | Lookups Manager — dual-table parent/child design |
| [LITHIUM-MGR-STYLE.md](LITHIUM-MGR-STYLE.md) | Style Manager — visual CSS editing with CodeMirror |
| [LITHIUM-MGR-CRIMSON.md](LITHIUM-MGR-CRIMSON.md) | Crimson AI Agent — popup chat interface |
| [LITHIUM-MGR-TOUR.md](LITHIUM-MGR-TOUR.md) | Tour Manager — Shepherd.js guided tours (Lookup #43) |
| [LITHIUM-MGR-VERSION.md](LITHIUM-MGR-VERSION.md) | Version Manager — history browser (Lookups 44/45) |
| [LITHIUM-MGR-SESSION.md](LITHIUM-MGR-SESSION.md) | Session Log Manager — debug output viewer |
| [LITHIUM-MGR-USERPROFILE.md](LITHIUM-MGR-USERPROFILE.md) | User Profile Manager — preferences |

### LithiumTable (Tabulator-Based Tables)

| File | Purpose |
|------|--------|
| [LITHIUM-TAB.md](LITHIUM-TAB.md) | LithiumTable Component — Tabulator + Navigator wrapper |
| [LITHIUM-TAB-TABLES.md](LITHIUM-TAB-TABLES.md) | Table configuration — columns to database, tableDef flow |
| [LITHIUM-TAB-TYPES.md](LITHIUM-TAB-TYPES.md) | Column types quick reference |
| [LITHIUM-TAB-TYPES-*.md](LITHIUM-TAB-TYPES-STAR.md) | Per-type documentation (STRING, INTEGER, DATE, etc.) |
| [LITHIUM-TAB-PLAN.md](LITHIUM-TAB-PLAN.md) | LithiumTable Phase 2 roadmap with gates |

### UI Components

| File | Purpose |
|------|--------|
| [LITHIUM-BAR.md](LITHIUM-BAR.md) | Toolbar system — standardized toolbars |
| [LITHIUM-COL.md](LITHIUM-COL.md) | Column Manager — runtime column customization |
| [LITHIUM-KEY.md](LITHIUM-KEY.md) | Keyboard shortcuts — global and manager-specific |
| [LITHIUM-ICN.md](LITHIUM-ICN.md) | Icon system — Font Awesome, config |
| [LITHIUM-TIP.md](LITHIUM-TIP.md) | Tooltip system — FloatingUI, themes, arrows |
| [LITHIUM-OSB.md](LITHIUM-OSB.md) | OverlayScrollbars — cross-browser scrollbar styling |
| [LITHIUM-CSB.md](LITHIUM-CSB.md) | CodeMirror Scrollbars — custom scrollbar plugin |
| [LITHIUM-FTR.md](LITHIUM-FTR.md) | Editor Footer — CodeMirror footer with cursor/error/word wrap |

### Styling & CSS

| File | Purpose |
|------|--------|
| [LITHIUM-CSS.md](LITHIUM-CSS.md) | CSS architecture — variables, theming |
| [LITHIUM-LIB.md](LITHIUM-LIB.md) | JavaScript libraries used in the project |
| [LITHIUM-OTH.md](LITHIUM-OTH.md) | Utilities — transitions, JSON, log, permissions |

### Authentication & Config

| File | Purpose |
|------|--------|
| [LITHIUM-JWT.md](LITHIUM-JWT.md) | JWT authentication — lifecycle, claims, validation |
| [LITHIUM-OIDC.md](LITHIUM-OIDC.md) | OpenID Connect sign-in — user/operator-facing reference |
| [LITHIUM-KEYCLOAK.md](LITHIUM-KEYCLOAK.md) | OIDC implementer recipe — for building new client SPAs |
| [LITHIUM-CFG.md](LITHIUM-CFG.md) | Configuration — lithium.json, settings |
| [LITHIUM-PWA.md](LITHIUM-PWA.md) | PWA — service worker, fast updates |

### Deployment & DevOps

| File | Purpose |
|------|--------|
| [LITHIUM-WEB.md](LITHIUM-WEB.md) | Deployment process — environment config |

---

## Documentation Map for AI Models

When working on Lithium, reference these files based on your task:

```
┌─────────────────────────────────────────────────────────────────┐
│                        LITHIUM-TOC.md                          │
│                    (This file — find your doc)                 │
└─────────────────────────────────────────────────────────────────┘
                              │
        ┌──────────────────────┼──────────────────────┐
        ▼                      ▼                      ▼
┌───────────────┐    ┌───────────────┐    ┌───────────────┐
│ LITHIUM-INS  │    │ LITHIUM-DEV   │    │  LITHIUM-TST   │
│ (MANDATORY)  │    │  (Devops)     │    │  (Testing)     │
│              │    │              │    │               │
│ Coding rules │    │ npm scripts   │    │ Vitest setup  │
│ File limits │    │ Build system │    │ Coverage    │
│ CSS-first  │    │ Env vars   │    │ Add tests  │
└───────────────┘    └───────────────┘    └───────────────┘
```

### Common Tasks → Right Doc

| Task | Start Here |
|------|----------|
| New to Lithium | [LITHIUM-TOC.md](LITHIUM-TOC.md) (you are here) |
| Writing code | [LITHIUM-INS.md](LITHIUM-INS.md) first |
| Setting up dev environment | [LITHIUM-DEV.md](LITHIUM-DEV.md) |
| Adding a new manager | [LITHIUM-MGR-NEW.md](LITHIUM-MGR-NEW.md) |
| Working on tables | [LITHIUM-TAB.md](LITHIUM-TAB.md) + [LITHIUM-TAB-TYPES.md](LITHIUM-TAB-TYPES.md) |
| Writing tests | [LITHIUM-TST.md](LITHIUM-TST.md) |
| Troubleshooting | [LITHIUM-FAQ.md](LITHIUM-FAQ.md) |
| Deploying | [LITHIUM-WEB.md](LITHIUM-WEB.md) |

---

## Project Location

| Path | Description |
|------|----------|
| `elements/003-lithium/` | Main project source |
| `docs/Li/` | Documentation (this directory) |

## External Links

| Resource | URL |
|----------|-----|
| Live Application | https://lithium.philement.com |
| Coverage Dashboard | https://lithium.philement.com/coverage/ |

---

**Last updated:** April 2026