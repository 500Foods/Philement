# Lithium Development Environment

This document describes the development environment, build tools, and workflow for working with Lithium.

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

For faster development, you can auto-login using URL parameters:

```bash
# Example: auto-login with test credentials
http://localhost:3000/?USER=testuser&PASS=usertest
```

This fills and submits the login form automatically.

---

## Available npm Scripts

### Development

| Command | Description |
|---------|-------------|
| `npm run dev` | Start development server with hot reload |
| `npm run build` | Build development version (no minification) |
| `npm run preview` | Preview production build locally |

### Testing

| Command | Description |
|---------|-------------|
| `npm test` | Run all tests (171 tests) |
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

---

## Build System

### Vite Configuration

Lithium uses [Vite](https://vitejs.dev/) as its build tool. Key configuration files:

| File | Purpose |
|------|---------|
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

---

## Environment Variables

Lithium uses these environment variables for deployment:

| Variable | Purpose | Example |
|----------|---------|---------|
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

This file is **not** bundled by Vite — it's fetched at runtime. On first deploy, it's seeded from the source. On subsequent deploys, it's preserved.

---

## Project Structure

```structure
elements/003-lithium/
├── index.html              # Entry point
├── package.json            # Dependencies and scripts
├── vite.config.js          # Vite configuration
├── vitest.config.js        # Vitest configuration
├── eslint.config.js        # ESLint configuration
├── config/
│   └── lithium.json        # Runtime configuration
├── src/
│   ├── app.js              # Bootstrap + manager loader
│   ├── core/               # Core modules
│   ├── managers/           # Feature managers
│   ├── shared/             # Shared utilities
│   └── styles/             # CSS files
├── tests/
│   └── unit/               # Vitest tests
└── public/                 # Static assets
```

---

## Testing Setup

### Framework

Lithium uses:

- **Vitest** — Test runner
- **happy-dom** — DOM implementation (better ESM support than jsdom)

### Running Tests

```bash
# Run all tests
npm test

# Run with coverage
npm run test:coverage

# Watch mode
npm run test:watch
```

### Coverage Dashboard

Generate and view coverage:

```bash
npm run test:coverage
npm run coverage:copy
```

Then open `coverage/index.html` in a browser.

**Live dashboard:** <https://lithium.philement.com/coverage/>

---

## Version System

Lithium uses an auto-incrementing build number independent of git:

| Field | Description |
|-------|-------------|
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

## Key Files Reference

| File | Description |
|------|-------------|
| [`app.js`](elements/003-lithium/src/app.js) | Main bootstrap, manager loader |
| [`event-bus.js`](elements/003-lithium/src/core/event-bus.js) | Event system |
| [`jwt.js`](elements/003-lithium/src/core/jwt.js) | JWT handling |
| [`config.js`](elements/003-lithium/src/core/config.js) | Configuration loader |
| [`permissions.js`](elements/003-lithium/src/core/permissions.js) | Punchcard permissions |

---

## Next Steps

- Learn about JavaScript libraries: [LITHIUM-LIB.md](LITHIUM-LIB.md)
- Understand the manager system: [LITHIUM-MGR.md](LITHIUM-MGR.md)
- Deployment: [LITHIUM-WEB.md](LITHIUM-WEB.md)
- Troubleshooting: [LITHIUM-FAQ.md](LITHIUM-FAQ.md)
