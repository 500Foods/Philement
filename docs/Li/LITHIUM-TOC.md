# Lithium Documentation Suite

This directory contains the canonical documentation for the Lithium web application. The docs are split into focused, bite-sized documents for easy reference when working on specific parts of the project.

---

## Key Project Notes

> **Important:** When working on Lithium, do NOT attempt to run the development server (`npm run dev`) to test changes. The project requires a running Hydrogen backend and specific environment configuration. Instead, ensure code correctness through:
>
> - Static analysis and code review
> - ESLint validation when available
> - Build verification (`npm run build`)
>
> Testing is performed in the CI/CD pipeline or manually in a properly configured environment.

---

## Documentation Index

| Document | Purpose |
|----------|---------|
| [LITHIUM-TOC.md](LITHIUM-TOC.md) | **This file** — Table of contents and overview |
| [LITHIUM-DEV.md](LITHIUM-DEV.md) | Development environment setup, build tools, npm scripts |
| [LITHIUM-TST.md](LITHIUM-TST.md) | Testing framework, Vitest, coverage |
| [LITHIUM-LIB.md](LITHIUM-LIB.md) | JavaScript libraries used in the project |
| [LITHIUM-MGR.md](LITHIUM-MGR.md) | Manager system — overview and patterns |
| [LITHIUM-MGR-LOGIN.md](LITHIUM-MGR-LOGIN.md) | Login Manager — auth, panels, keyboard shortcuts |
| [LITHIUM-MGR-MAIN.md](LITHIUM-MGR-MAIN.md) | Main Manager — sidebar, slots, logout |
| [LITHIUM-MGR-QUERY.md](LITHIUM-MGR-QUERY.md) | Query Manager — design and implementation plan |
| [LITHIUM-TAB.md](LITHIUM-TAB.md) | Tabulator Component — table and navigator block |
| [LITHIUM-API.md](LITHIUM-API.md) | Hydrogen API endpoints and Conduit client library (`conduit.js`) |
| [LITHIUM-CSS.md](LITHIUM-CSS.md) | CSS architecture, variables, theming |
| [LITHIUM-ICN.md](LITHIUM-ICN.md) | Icon system, Font Awesome, config |
| [LITHIUM-OTH.md](LITHIUM-OTH.md) | Utilities — transitions, utils, JSON, log, JWT |
| [LITHIUM-CFG.md](LITHIUM-CFG.md) | Configuration, lithium.json, settings |
| [LITHIUM-PWA.md](LITHIUM-PWA.md) | PWA, service worker, fast updates |
| [LITHIUM-FAQ.md](LITHIUM-FAQ.md) | Lessons learned, issues resolved, troubleshooting |
| [LITHIUM-WEB.md](LITHIUM-WEB.md) | Deployment process, environment configuration |

---

## Quick Start

```bash
# Navigate to the Lithium project
cd elements/003-lithium

# Install dependencies
npm install

# Start development server
npm run dev

# Run tests
npm test

# Build for production
npm run build:prod

# Deploy
npm run deploy
```

---

## What is Lithium?

Lithium is a lightweight, performant, modular single-page application (SPA) built with vanilla JavaScript ES modules. It serves as the frontend for the Philement platform, connecting to the Hydrogen backend.

### Key Characteristics

- **No UI frameworks** — Pure vanilla JavaScript
- **ES Modules** — Native browser module system
- **Dark-mode-first** — Custom CSS with CSS variables
- **PWA-ready** — Service worker for offline support
- **JWT Authentication** — Secure auth via Hydrogen backend

---

## Project Location

| Path | Description |
|------|-------------|
| `elements/003-lithium/` | Main project source |
| `docs/Li/` | Documentation (this directory) |

---

## Key Documentation Links

### For Development

- Start here: [LITHIUM-DEV.md](LITHIUM-DEV.md)
- Understand libraries: [LITHIUM-LIB.md](LITHIUM-LIB.md)
- Learn the manager system: [LITHIUM-MGR.md](LITHIUM-MGR.md)

### For Deployment

- Deployment process: [LITHIUM-WEB.md](LITHIUM-WEB.md)
- Troubleshooting: [LITHIUM-FAQ.md](LITHIUM-FAQ.md)

---

## External Links

| Resource | URL |
|----------|-----|
| Live Application | <https://lithium.philement.com> |
| Coverage Dashboard | <https://lithium.philement.com/coverage/> |
| Project Source | `elements/003-lithium/` |

---

## Documentation Version

This documentation suite was created to replace the older combined `INSTRUCTIONS.md` and `BLUEPRINT.md` files, which had grown confused and outdated. The new split format allows for easier maintenance and targeted updates.

Last updated: March 2026
