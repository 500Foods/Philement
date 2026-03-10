# Lithium - A Philement Project

**Lithium** is a lightweight, performant, modular Single Page Application (SPA) built with vanilla JavaScript ES modules. It features JWT authentication, a manager-based architecture, and comprehensive testing infrastructure.

**Live Site:** <https://lithium.philement.com>  
**Test Coverage:** <https://lithium.philement.com/coverage/>

---

## Quick Links

### 📚 Documentation (in `/docs/Li/`)

| Document | Description |
|----------|-------------|
| [LITHIUM-TOC.md](/docs/Li/LITHIUM-TOC.md) | **Table of contents** - Start here |
| [LITHIUM-DEV.md](/docs/Li/LITHIUM-DEV.md) | **Development guide** - Environment, npm scripts, Vite/Vitest |
| [LITHIUM-TST.md](/docs/Li/LITHIUM-TST.md) | **Testing** - Vitest, coverage, best practices |
| [LITHIUM-LIB.md](/docs/Li/LITHIUM-LIB.md) | **Libraries** - Tabulator, CodeMirror, DOMPurify |
| [LITHIUM-MGR.md](/docs/Li/LITHIUM-MGR.md) | **Managers** - Implemented and planned features |
| [LITHIUM-FAQ.md](/docs/Li/LITHIUM-FAQ.md) | **FAQ** - Lessons learned, troubleshooting |
| [LITHIUM-WEB.md](/docs/Li/LITHIUM-WEB.md) | **Deployment** - Process, configuration |

### 🚀 Quick Start

```bash
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

## Project Structure

```structure
elements/003-lithium/
├── src/
│   ├── app.js              # Main application bootstrap
│   ├── core/               # Core modules (event-bus, jwt, config, etc.)
│   ├── managers/           # Feature modules (login, main, style-manager)
│   ├── shared/             # Shared utilities
│   └── styles/             # CSS architecture
├── public/                  # Static assets
│   ├── assets/             # Fonts, images
│   ├── coverage/           # Test coverage report
│   └── src/managers/       # HTML templates for production
├── tests/unit/             # Vitest unit tests
├── config/                  # Runtime configuration
└── docs/Li/                 # 📚 Full documentation
```

---

## Current Status

✅ **Phase 0-3 Complete:** Foundation, Login, Main Layout  
✅ **Phase 4 Complete:** Style Manager (Tabulator, CodeMirror, DOMPurify)  
✅ **Utility Managers:** Session Log, User Profile  
⬜ **Phase 5:** Dashboard, Lookups, Queries placeholders  

**Test Coverage:** 171 tests, 0 failures  

| Module | Coverage |
|--------|----------|
| event-bus.js | 100% |
| config.js | 100% |
| permissions.js | 96% |
| utils.js | 88% |

---

## Tech Stack

- **Language:** Vanilla JavaScript (ES modules)
- **Build Tool:** Vite
- **Testing:** Vitest + happy-dom
- **UI:** Custom CSS with CSS variables (dark-mode-first)
- **Tables:** Tabulator
- **Code Editor:** CodeMirror 6
- **Icons:** Font Awesome Kit
- **Fonts:** Vanadium Sans + Vanadium Mono

---

## Deployment

Environment variables required:

- `LITHIUM_ROOT` - Project source directory
- `LITHIUM_DEPLOY` - Web server document root

```bash
# Deploy with coverage dashboard
npm run test:coverage
npm run deploy
```

---

## Documentation Overview

### For Developers

1. Start with [**LITHIUM-TOC.md**](/docs/Li/LITHIUM-TOC.md) for the index
2. Reference [**LITHIUM-DEV.md**](/docs/Li/LITHIUM-DEV.md) for development setup
3. Check [**LITHIUM-MGR.md**](/docs/Li/LITHIUM-MGR.md) for manager patterns

### For DevOps

- [**LITHIUM-WEB.md**](/docs/Li/LITHIUM-WEB.md) - Deployment configuration
- [**LITHIUM-TST.md**](/docs/Li/LITHIUM-TST.md) - Testing infrastructure

---

## License

MIT License - see [LICENSE.md](/docs/Li/LICENSE.md)
