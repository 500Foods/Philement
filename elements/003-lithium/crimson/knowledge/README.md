# Lithium Documentation

**Complete documentation for the Lithium PWA**  
🔗 **Project Home:** [elements/003-lithium/README.md](/elements/003-lithium/README.md)  
🌐 **Live Site:** <https://lithium.philement.com>  
📊 **Coverage:** <https://lithium.philement.com/coverage/>

---

## Documentation Index

### New Split Documentation (Recommended)

| Document | Purpose |
|----------|---------|
| [LITHIUM-TOC.md](LITHIUM-TOC.md) | Table of contents and overview |
| [LITHIUM-DEV.md](LITHIUM-DEV.md) | Development environment and build tools |
| [LITHIUM-TST.md](LITHIUM-TST.md) | Testing framework and coverage |
| [LITHIUM-LIB.md](LITHIUM-LIB.md) | JavaScript libraries |
| [LITHIUM-MGR.md](LITHIUM-MGR.md) | Manager system (implemented and planned) |
| [LITHIUM-FAQ.md](LITHIUM-FAQ.md) | Lessons learned and troubleshooting |
| [LITHIUM-WEB.md](LITHIUM-WEB.md) | Deployment process |

---

## 🚀 Quick Start

```bash
cd elements/003-lithium

# Install dependencies
npm install

# Start development server
npm run dev

# Run tests
npm test

# Run tests with coverage
npm run test:coverage

# Deploy to production
npm run deploy
```

---

## Documentation Guide

### For Developers Working on

- **Getting started:** [LITHIUM-DEV.md](LITHIUM-DEV.md)
- **Adding features/managers:** [LITHIUM-MGR.md](LITHIUM-MGR.md)
- **Using libraries:** [LITHIUM-LIB.md](LITHIUM-LIB.md)
- **Testing:** [LITHIUM-TST.md](LITHIUM-TST.md)
- **Deployment:** [LITHIUM-WEB.md](LITHIUM-WEB.md)
- **Troubleshooting:** [LITHIUM-FAQ.md](LITHIUM-FAQ.md)

---

## Architecture Overview

### Tech Stack

- **Language:** Vanilla JavaScript (ES modules, async/await, fetch)
- **Build Tool:** Vite (dev server, hot reload, production build)
- **Testing:** Vitest + happy-dom
- **UI:** Custom CSS with CSS variables (dark-mode-first)
- **Tables:** Tabulator
- **Code Editor:** CodeMirror 6
- **Sanitization:** DOMPurify
- **Auth:** JWT from Hydrogen backend
- **Icons:** Font Awesome Kit
- **Fonts:** Vanadium Sans + Vanadium Mono

### Manager System

Lithium uses a **manager-based architecture** where each feature is a self-contained module:

```javascript
// Managers are lazy-loaded and have lifecycle methods
class ExampleManager {
  async init() { /* Initialize */ }
  async render() { /* Load HTML template */ }
  teardown() { /* Cleanup */ }
}
```

**Implemented Managers:**

- ✅ Login Manager - JWT authentication
- ✅ Main Manager - Sidebar + header + workspace
- ✅ Style Manager - Theme management with Tabulator/CodeMirror
- ✅ Session Log - Utility manager for session logs
- ✅ User Profile - Utility manager for preferences
- ⬜ Dashboard - (placeholder)
- ⬜ Lookups - (placeholder)
- ⬜ Queries - (placeholder)

---

## Current Status

### Implementation Phases

| Phase | Status | Description |
|-------|--------|-------------|
| 0 | ✅ | Blueprint |
| 1 | ✅ | Foundation (core modules, CSS, Vite/Vitest) |
| 2 | ✅ | Login (CCC-style, JWT, error handling) |
| 3 | ✅ | Main Layout (sidebar, workspace, responsive) |
| 4 | ✅ | Style Manager (Tabulator, CodeMirror, DOMPurify) |
| 5 | ⬜ | Profile Manager (preferences, JWT refresh) |
| 6 | ⬜ | Testing and Polish (integration tests, a11y) |

### Test Coverage

#### 171 tests, 0 failures

| Module | Coverage |
|--------|----------|
| event-bus.js | 100% |
| config.js | 100% |
| permissions.js | 96% |
| utils.js | 88% |
| icons.js | 85% |
| jwt.js | 75% |

---

## 🔗 External Links

- **Live Application:** <https://lithium.philement.com>
- **Test Coverage Dashboard:** <https://lithium.philement.com/coverage/>
- **Project Source:** [elements/003-lithium/](/elements/003-lithium/)
- **Project README:** [elements/003-lithium/README.md](/elements/003-lithium/README.md)

---

## 📝 Key Files

```structure
elements/003-lithium/
├── README.md              # ⬅️ Project overview
├── package.json           # Dependencies and scripts
├── vite.config.js         # Vite configuration
├── vitest.config.js       # Vitest configuration
├── index.html             # Entry point
├── config/
│   └── lithium.json       # Runtime configuration
├── src/
│   ├── app.js             # Main application bootstrap
│   ├── core/              # Core modules
│   │   ├── event-bus.js  # Event system
│   │   ├── jwt.js        # JWT handling
│   │   ├── config.js     # Configuration loader
│   │   ├── permissions.js# Permission system
│   │   ├── json-request.js# HTTP client
│   │   └── utils.js     # Utilities
│   ├── managers/          # Feature modules
│   │   ├── login/        # Login manager
│   │   ├── main/         # Main layout
│   │   ├── style-manager/# Theme management
│   │   └── ...
│   ├── shared/           # Shared utilities
│   └── styles/           # CSS files
├── tests/unit/           # Vitest tests
└── public/               # Static assets
```

---

## Other Files

| Document | Purpose |
|----------|---------|
| [LICENSE.md](LICENSE.md) | MIT License |

---

**← Back to [Project README](/elements/003-lithium/README.md)**
