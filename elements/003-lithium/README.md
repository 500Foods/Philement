# Lithium - A Philement Project

**Lithium** is a lightweight, performant, modular Single Page Application (SPA) built with vanilla JavaScript ES modules. It features JWT authentication, a manager-based architecture, and comprehensive testing infrastructure.

**Live Site:** <https://lithium.philement.com>  
**Test Coverage:** <https://lithium.philement.com/coverage/>

---

## Quick Links

### 📚 Documentation (in `/docs/Li/`)

| Document | Description |
|----------|-------------|
| [📋 BLUEPRINT.md](/docs/Li/BLUEPRINT.md) | **Architecture blueprint** - Start here for technical details |
| [📖 INSTRUCTIONS.md](/docs/Li/INSTRUCTIONS.md) | **Developer guide** - How to work with the codebase |
| [🔧 BUILD.md](/docs/Li/BUILD.md) | Build process and deployment |
| [🧪 TESTING.md](/docs/Li/TESTING.md) | Testing framework and coverage |
| [📄 README.md](/docs/Li/README.md) | Full documentation index |

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

### 📁 Project Structure

```structure
elements/003-lithium/
├── src/
│   ├── app.js              # Main application bootstrap
│   ├── core/               # Core modules (event-bus, jwt, config, etc.)
│   ├── managers/           # Feature modules (login, main, style-manager)
│   ├── shared/             # Shared utilities
│   └── styles/             # CSS architecture
├── public/                 # Static assets
│   ├── assets/             # Fonts, images
│   ├── coverage/           # Test coverage report
│   └── src/managers/       # HTML templates for production
├── tests/unit/             # Vitest unit tests
├── config/                 # Runtime configuration
└── docs/Li/                # 📚 Full documentation
```

---

## Current Status

✅ **Phase 0-3 Complete:** Foundation, Login, Main Layout  
✅ **Phase 4 Complete:** Style Manager (Tabulator, CodeMirror, DOMPurify)  
⬜ **Phase 5:** Profile Manager  
⬜ **Phase 6:** Integration tests and polish

**Test Coverage:** 127 tests, 0 failures  

| Module | Coverage |
|--------|----------|
| event-bus.js | 100% |
| config.js | 100% |
| permissions.js | 96% |
| utils.js | 88% |
| jwt.js | 75% |

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

1. Start with [**INSTRUCTIONS.md**](/docs/Li/INSTRUCTIONS.md) for:
   - Project structure
   - Manager system architecture
   - Event bus usage
   - CSS architecture
   - Testing guide
   - Troubleshooting

2. Reference [**BLUEPRINT.md**](/docs/Li/BLUEPRINT.md) for:
   - Technical architecture
   - Bootstrap flow
   - JWT handling
   - Permissions (Punchcard)
   - Implementation phases

### For DevOps

1. [**BUILD.md**](/docs/Li/BUILD.md) - Build configuration and scripts
2. [**TESTING.md**](/docs/Li/TESTING.md) - Testing infrastructure and coverage

---

## License

MIT License - see [LICENSE.md](/docs/Li/LICENSE.md)
