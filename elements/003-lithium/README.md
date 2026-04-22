# Lithium

A lightweight, performant modular SPA built with vanilla JavaScript ES modules. Features JWT authentication, manager-based architecture, and comprehensive testing.

**Live Site:** https://lithium.philement.com  
**Coverage:** https://lithium.philement.com/coverage/

---

## Documentation

| Document | Purpose |
|----------|---------|
| **[docs/Li/LITHIUM-TOC.md](/docs/Li/LITHIUM-TOC.md)** | **Table of contents** — Start here |
| **[docs/Li/LITHIUM-INS.md](/docs/Li/LITHIUM-INS.md)** | **⚠️ Mandatory** — Coding standards |
| [docs/Li/LITHIUM-DEV.md](/docs/Li/LITHIUM-DEV.md) | Development environment |
| [docs/Li/LITHIUM-TST.md](/docs/Li/LITHIUM-TST.md) | Testing framework |
| [docs/Li/LITHIUM-FAQ.md](/docs/Li/LITHIUM-FAQ.md) | Troubleshooting |
| [docs/Li/README.md](/docs/Li/README.md) | Full documentation index |

---

## Quick Start

```bash
cd elements/003-lithium

npm install        # Install dependencies
npm run dev        # Start dev server (requires Hydrogen backend)
npm test           # Run tests
npm run build      # Build for production
```

---

## Tech Stack

- **Language:** Vanilla JavaScript (ES modules)
- **Build:** Vite
- **Testing:** Vitest + happy-dom
- **UI:** Custom CSS with variables (dark-mode-first)
- **Tables:** Tabulator
- **Editors:** CodeMirror 6
- **Icons:** Font Awesome Kit
- **Auth:** JWT from Hydrogen backend

---

## Project Structure

```structure
elements/003-lithium/
├── src/
│   ├── app.js       # Bootstrap + manager loader
│   ├── core/       # Core modules (event-bus, jwt, config, etc.)
│   ├── managers/   # Feature modules
│   └── styles/    # CSS architecture
├── tests/unit/      # Vitest tests
├── config/         # Runtime config
└── docs/Li/        # Documentation
```

---

**← Back to [Project Root](/README.md)**