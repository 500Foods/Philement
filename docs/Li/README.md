# Lithium Documentation

**Complete documentation for the Lithium PWA**  
🔗 **Project Home:** [elements/003-lithium/README.md](/elements/003-lithium/README.md)  
🌐 **Live Site:** <https://lithium.philement.com>  
📊 **Coverage:** <https://lithium.philement.com/coverage/>

---

## 📚 Documentation Index

### Core Documentation

| Document | Purpose |
|----------|---------|
| [🏗️ BLUEPRINT.md](BLUEPRINT.md) | **Architecture blueprint** - Technical specification, implementation phases, deployment details |
| [📖 INSTRUCTIONS.md](INSTRUCTIONS.md) | **Developer guide** - How to work with the codebase, manager system, event bus, testing |
| [🔧 BUILD.md](BUILD.md) | Build configuration, deployment scripts, CI/CD setup |
| [🧪 TESTING.md](TESTING.md) | Testing framework, coverage reports, test writing guide |
| [📋 LICENSE.md](LICENSE.md) | MIT License |

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

## 📁 Documentation Guide

### For New Developers

Start here to understand the codebase:

1. **[INSTRUCTIONS.md](INSTRUCTIONS.md)** - Comprehensive developer guide
   - Project structure
   - Manager system architecture
   - Event bus usage
   - CSS architecture and variables
   - Authentication (JWT)
   - Permissions (Punchcard)
   - Testing guide
   - Troubleshooting

### For Architects

Deep technical details:

1. **[BLUEPRINT.md](BLUEPRINT.md)** - Technical specification
   - Project goals and tech stack
   - Bootstrap flow
   - JWT claims and handling
   - Event bus standard events
   - CSS architecture
   - Implementation phases
   - Deployment process
   - Known limitations

### For DevOps

Build and deployment:

1. **[BUILD.md](BUILD.md)** - Build system
   - Vite configuration
   - Production builds
   - Environment variables
   - Deployment scripts

2. **[TESTING.md](TESTING.md)** - Testing infrastructure
   - Vitest configuration
   - Test organization
   - Coverage reports
   - CI/CD integration

---

## 🏗 Architecture Overview

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
- ⬜ Profile Manager - User preferences
- ⬜ Dashboard - (placeholder)
- ⬜ Lookups - (placeholder)
- ⬜ Queries - (placeholder)

---

## 📊 Current Status

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

#### 127 tests, 0 failures

| Module | Statements | Branch | Functions | Lines |
|--------|-----------|--------|-----------|-------|
| event-bus.js | 100% | 100% | 100% | 100% |
| config.js | 100% | 90% | 100% | 100% |
| permissions.js | 96% | 95% | 100% | 96% |
| utils.js | 88% | 86% | 92% | 88% |
| jwt.js | 75% | 73% | 75% | 75% |
| json-request.js | 40% | 31% | 50% | 40% |
| lookups.js | 37% | 18% | 50% | 37% |

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
├── README.md              # ⬅️ Project overview (TOC)
├── package.json           # Dependencies and scripts
├── vite.config.js         # Vite configuration
├── vitest.config.js       # Vitest configuration
├── index.html             # Entry point
├── config/
│   └── lithium.json       # Runtime configuration
├── src/
│   ├── app.js             # Main application bootstrap
│   ├── core/              # Core modules
│   │   ├── event-bus.js   # Event system
│   │   ├── jwt.js         # JWT handling
│   │   ├── config.js      # Configuration loader
│   │   ├── permissions.js # Permission system
│   │   ├── json-request.js# HTTP client
│   │   └── utils.js       # Utilities
│   ├── managers/          # Feature modules
│   │   ├── login/         # Login manager
│   │   ├── main/          # Main layout
│   │   └── style-manager/ # Theme management
│   ├── shared/            # Shared utilities
│   └── styles/            # CSS files
├── tests/unit/            # Unit tests
└── public/                # Static assets
```

---

**← Back to [Project README](/elements/003-lithium/README.md)**
