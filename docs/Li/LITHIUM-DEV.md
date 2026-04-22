# Lithium Development Environment

This document describes the development environment, build tools, and workflow for Lithium.

---

## Prerequisites

| Requirement | Version | Notes |
|-------------|---------|-------|
| Node.js | >= 18.0.0 | LTS recommended |
| npm | >= 9.0.0 | Comes with Node.js |
| Browser | Modern (Chrome, Firefox, Edge, Safari) | For testing |

---

## Installation

```bash
cd elements/003-lithium
npm install
```

This installs all dependencies defined in `package.json`.

---

## Development Server

Start the Vite development server with hot reload:

```bash
npm run dev
```

The app will be available at `http://localhost:3000`.

### With Auto-Login

For faster development, auto-login using URL parameters:

```bash
http://localhost:3000/?USER=testuser&PASS=usertest
```

This fills and submits the login form automatically.

---

## npm Scripts

### Development

| Command | Description |
|---------|-------------|
| `npm run dev` | Start development server with hot reload |
| `npm run build` | Build development version (no minification) |
| `npm run preview` | Preview production build locally |

### Testing

| Command | Description |
|---------|-------------|
| `npm test` | Run all tests |
| `npm run test:watch` | Run tests in watch mode |
| `npm run test:coverage` | Run tests with coverage report |

### Production

| Command | Description |
|---------|-------------|
| `npm run build:prod` | Production build with minification |
| `npm run deploy` | Build, test, and deploy |

### Utilities

| Command | Description |
|---------|-------------|
| `npm run lint` | Run ESLint |
| `npm run format` | Format code with Prettier |
| `npm run clean` | Clean build artifacts |
| `npm run version:bump` | Increment build number |
| `npm run templates:copy` | Copy HTML templates to public/ |

---

## Build System

### Vite Configuration

Lithium uses [Vite](https://vitejs.dev/) as its build tool. Key configuration files:

| File | Purpose |
|------|--------|
| `vite.config.js` | Main Vite configuration |
| `vitest.config.js` | Vitest testing configuration |
| `eslint.config.js` | ESLint flat config |

### Build Output

Running `npm run build` or `npm run build:prod` produces:

```structure
dist/
├── index.html
├── manifest.json
├── service-worker.js
├── config/lithium.json
├── assets/
│   ├── index-[hash].js
│   ├── index-[hash].css
│   └── ...
└── src/managers/
    └── ... (HTML templates)
```

### Deployment Output

| File | Purpose |
|------|--------|
| `index.html` | Entry point |
| `manifest.json` | PWA manifest |
| `service-worker.js` | Service worker for offline |
| `config/lithium.json` | Runtime configuration |
| `assets/` | Bundled JS and CSS |
| `src/managers/` | HTML templates |

---

## Environment Variables

Lithium uses these environment variables for deployment:

| Variable | Purpose | Example |
|----------|---------|--------|
| `LITHIUM_ROOT` | Project source directory | `/mnt/extra/Projects/Philement/elements/003-lithium` |
| `LITHIUM_DEPLOY` | Web server document root | `/fvl/tnt/t-philement/lithium` |
| `LITHIUM_DEPLOY_KEEP` | Rollback window for deploy pruning | `3` |

---

## Runtime Configuration

The app reads configuration from `config/lithium.json` at startup:

```json
{
  "server": {
    "url": "https://hydrogen.example.com",
    "api_prefix": "/api"
  },
  "auth": {
    "api_key": "public-app-id",
    "default_database": "Demo_PG"
  },
  "app": {
    "name": "Lithium",
    "version": "1.0.0",
    "default_theme": "dark"
  }
}
```

This file is **not** bundled by Vite — it's fetched at runtime. On first deploy, it's seeded from source. On subsequent deploys, it's preserved.

---

## Project Structure

```structure
elements/003-lithium/
├── index.html              # Entry point
├── package.json           # Dependencies and scripts
├── vite.config.js        # Vite configuration
├── vitest.config.js    # Vitest configuration
├── eslint.config.js    # ESLint configuration
├── config/
│   └── lithium.json   # Runtime configuration
├── src/
│   ├── app.js       # Bootstrap + manager loader
│   ├── core/      # Core modules
│   ├── managers/  # Feature managers
│   ├── shared/   # Shared utilities
│   └── styles/  # CSS files
├── tests/
│   └── unit/     # Vitest tests
└── public/        # Static assets
```

### Core Modules

| File | Description |
|------|----------|
| `src/app.js` | Main bootstrap, manager loader |
| `src/core/event-bus.js` | Event system |
| `src/core/jwt.js` | JWT handling |
| `src/core/config.js` | Configuration loader |
| `src/core/permissions.js` | Punchcard permissions |
| `src/core/json-request.js` | HTTP client |
| `src/core/log.js` | Session logging |
| `src/core/lookups.js` | Lookup table caching |

---

## Version System

Lithium uses an auto-incrementing build number independent of git:

| Field | Description |
|-------|-----------|
| `build` | Monotonic integer starting at 1000 |
| `timestamp` | ISO 8601 timestamp |
| `version` | Semantic-style: `0.1.<build>` |

Run `npm run version:bump` to increment the build number.

---

## Code Style

Lithium uses:
- **ESLint** for linting (flat config)
- **Prettier** for formatting (optional)

Run:

```bash
npm run lint
npm run format
```

---

## Deployment Scripts

### `npm run deploy`

The deploy script performs these steps:

1. **Clean** — Remove old build artifacts
2. **Lint** — Run ESLint (must pass)
3. **Test** — Run all tests (must pass)
4. **Build** — Production build with minification
5. **Templates** — Copy HTML templates to public/
6. **Config** — Seed runtime config (if not exists)
7. **Prune** — Remove old hashed assets (keep newest N versions)
8. **Copy** — Copy to deploy directory

### Environment for Deployment

```bash
export LITHIUM_ROOT=/mnt/extra/Projects/Philement/elements/003-lithium
export LITHIUM_DEPLOY=/fvl/tnt/t-philement/lithium
export LITHIUM_DEPLOY_KEEP=3
npm run deploy
```

---

## Next Steps

- **Coding standards:** [LITHIUM-INS.md](LITHIUM-INS.md)
- **Testing:** [LITHIUM-TST.md](LITHIUM-TST.md)
- **Managers:** [LITHIUM-MGR.md](LITHIUM-MGR.md)
- **Libraries:** [LITHIUM-LIB.md](LITHIUM-LIB.md)
- **Deployment:** [LITHIUM-WEB.md](LITHIUM-WEB.md)
- **Troubleshooting:** [LITHIUM-FAQ.md](LITHIUM-FAQ.md)