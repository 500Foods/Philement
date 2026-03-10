# Lithium FAQ and Lessons Learned

This document captures lessons learned, issues encountered, and how to resolve them.

---

## Build & Import Issues

### Vite Warning: "Dynamically Imported but Also Statically Imported"

**Problem:** A module is both imported at the top of a file AND dynamically imported with `await import()` elsewhere.

**Impact:** Vite cannot create a separate chunk because the static import already bundles it.

**Fix:** Use static imports for core modules. Only use dynamic `import()` for:

- Lazy-loaded managers
- Heavy third-party libraries (Tabulator, CodeMirror, DOMPurify)

```javascript
// Bad: Mixed imports
import { storeJWT } from './core/jwt.js';
const mod = await import('./core/jwt.js');  // Warning!

// Good: Static for core
import { storeJWT } from './core/jwt.js';
// Dynamic only for managers
const { LoginManager } = await import('./managers/login/login.js');
```

---

### Dynamic Import with Runtime Variables

**Problem:** `import(/* @vite-ignore */ someVar)` bypasses Vite's analysis. The module doesn't exist in production.

**Fix:** Use static switch dispatch:

```javascript
// Bad
const manager = await import(/* @vite-ignore */ managerPath);

// Good
async function loadManager(id) {
  switch (id) {
    case 1: return import('./managers/style-manager/style-manager.js');
    case 3: return import('./managers/dashboard/dashboard.js');
    // ...
  }
}
```

---

## Deployment Issues

### HTML Template 404 Errors

**Problem:** Managers fail to load with 404 for `.html` files in production.

**Cause:** Managers fetch HTML templates at runtime. Vite only bundles JS/CSS — not runtime-fetched HTML.

**Fix:** Run `npm run templates:copy` or use `npm run deploy` which runs it automatically.

**What happens:** Copies HTML templates from `src/managers/*/` to `public/src/managers/*/`

---

### Font Awesome Not Loading

**Problem:** Icons appear as squares or don't load.

**Cause:** CDN links with SRI hashes can fail if CDN content changes.

**Fix:** Use Font Awesome Kit (`https://kit.fontawesome.com/`) instead of CDN with SRI.

---

### Runtime Config Overwritten

**Problem:** Changes to `config/lithium.json` lost on redeploy.

**Cause:** Build copies source config over existing config.

**Fix:** The deploy script now preserves `config/lithium.json` if it already exists in the deploy directory. Only seed on first deploy.

---

### Old Hashed Assets Accumulating

**Problem:** Hundreds of old JS/CSS files in deploy directory.

**Fix:** Deploy prune step now keeps only the newest N versions per asset family (default: 3).

```bash
# Set custom retention
export LITHIUM_DEPLOY_KEEP=5
npm run deploy
```

---

## PWA Issues

### Service Worker Not Registering

**Check:**

1. `service-worker.js` exists in deploy root
2. Served over HTTPS
3. Scope is correct in browser DevTools

---

### Update Not Detected

**Problem:** New version deployed but app doesn't reload.

**Fix:** Four-layer detection system:

1. Service worker uses network-first for `version.json`
2. Bootstrap gate fetches version before app loads
3. Promise never resolves on reload (blocks UI)
4. `controllerchange` fallback for SW replacement

---

## Testing Issues

### Test Coverage Gaps

**Low coverage files:**

- `json-request.js` (40%) — ESM mocking complexity
- `lookups.js` (37%) — Fetch + auth chains

**Why:** ESM mocking with Vitest + happy-dom is challenging for modules with fetch/await chains.

**Mitigation:** Focus testing on core logic, accept lower coverage for HTTP wrapper modules.

---

### Integration Tests Need Server

**Problem:** Integration tests fail without Hydrogen server.

**Fix:** Tests automatically skip with clear message if credentials unavailable.

**Required env vars:**

```bash
export HYDROGEN_SERVER_URL=https://lithium.philement.com
export HYDROGEN_DEMO_USER_NAME=testuser
export HYDROGEN_DEMO_USER_PASS=usertest
export HYDROGEN_DEMO_API_KEY=EveryGoodBoyDeservesFudge
```

---

## CSS & Styling Issues

### Button Group Design

Use the connected button pattern for related actions:

```html
<div class="login-btn-group">
  <button class="login-btn-icon">...</button>
  <button class="login-btn-primary">Login</button>
  <button class="login-btn-icon">...</button>
</div>
```

Key: 2px gaps, shared height, rounded corners on ends only.

---

### Slot Button Injection

Managers inject buttons into the slot's existing header/footer — not their own toolbar.

```javascript
const mainMgr = this.app._getMainManager();
const slotId = mainMgr._slotId(managerId);

// Header buttons
mainMgr.addHeaderButtons(slotId, [
  { icon: 'fa-refresh', onClick: () => this.refresh() }
]);

// Footer buttons
mainMgr.addFooterButtons(slotId, 'right', [
  { icon: 'fa-export', onClick: () => this.export() }
]);
```

---

## Authentication Issues

### JWT Not Stored

**Check:**

1. Login successful (no 401/429)
2. Token in response
3. localStorage accessible

**Key:** `localStorage.setItem('lithium_jwt', token)`

---

### Token Not Attached to Requests

**Check:** `json-request.js` adds `Authorization: Bearer <token>` header automatically if JWT exists.

---

### Auto-Renewal Not Working

**Check:** Token has 80% lifetime remaining. If expired, renewal fails and `auth:expired` fires.

---

## Event Bus Issues

### Events Not Firing

**Check:**

1. Import correct: `import { eventBus, Events } from '../../core/event-bus.js'`
2. Listener added before emit
3. Use correct event name

---

### Memory Leaks from Listeners

**Fix:** Always remove listeners in `teardown()`:

```javascript
teardown() {
  eventBus.off(Events.AUTH_LOGIN, this.handleLogin);
}
```

Or use `once()` for one-time listeners.

---

## Known Limitations

1. **Config path** — Fetches `/config/lithium.json` relative to root; may need adjustment for path-prefixed deployments.

2. **Permissions fallback** — Allow-all when no Punchcard; should be configurable for production.

3. **json-request.js** — No way to suppress `auth:expired` event for specific requests.

---

## Lessons Learned

### Build & Import

1. Use static imports for core modules, dynamic for managers/heavy libs
2. Don't use `/* @vite-ignore */` with runtime variables — use switch dispatch
3. Run `templates:copy` for production HTML templates

### Deployment

1. Font Awesome Kit > CDN with SRI
2. Verify deploy directory structure before serving
3. Keep deploy prune enabled to prevent asset accumulation

### Testing

1. happy-dom > jsdom for ESM support
2. Use `vi.hoisted()` for ESM mocks
3. Integration tests should skip gracefully if server unavailable

### PWA

1. Network-first for `version.json` ensures freshness
2. Block UI during reload to prevent flash
3. Service worker must be registered early (not on window.load)

---

## Related Documentation

- Development: [LITHIUM-DEV.md](LITHIUM-DEV.md)
- Libraries: [LITHIUM-LIB.md](LITHIUM-LIB.md)
- Managers: [LITHIUM-MGR.md](LITHIUM-MGR.md)
- Deployment: [LITHIUM-WEB.md](LITHIUM-WEB.md)
