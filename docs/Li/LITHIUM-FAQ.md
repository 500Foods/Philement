# Lithium FAQ

Quick answers to common Lithium questions. For detailed technical information, see the specific documentation files linked throughout.

---

## Build & Import Issues

### Vite Warning: "Dynamically Imported but Also Statically Imported"

**Problem:** A module is both imported statically and dynamically.

**Fix:** Use static imports for core modules. Dynamic `import()` only for:
- Lazy-loaded managers
- Heavy third-party libraries (Tabulator, CodeMirror, DOMPurify)

```javascript
// Good: Static for core
import { storeJWT } from './core/jwt.js';
// Dynamic only for managers
const { LoginManager } = await import('./managers/login/login.js');
```

### Dynamic Import with Runtime Variables

**Problem:** `import(/* @vite-ignore */ someVar)` bypasses Vite analysis.

**Fix:** Use static switch dispatch:

```javascript
async function loadManager(id) {
  switch (id) {
    case 1: return import('./managers/style-manager/style-manager.js');
    case 3: return import('./managers/dashboard/dashboard.js');
  }
}
```

---

## Deployment Issues

### HTML Template 404 Errors

**Problem:** Managers fail with 404 for `.html` files in production.

**Fix:** Run `npm run templates:copy` before deploying, or use `npm run deploy` which runs it automatically.

### Font Awesome Not Loading

**Problem:** Icons appear as squares.

**Fix:** Use Font Awesome Kit (`https://kit.fontawesome.com/`) instead of CDN with SRI.

### Runtime Config Overwritten

**Problem:** Changes to `config/lithium.json` lost on redeploy.

**Fix:** The deploy script now preserves existing config. Only seed on first deploy.

### Old Hashed Assets Accumulating

**Problem:** Hundreds of old JS/CSS files in deploy directory.

**Fix:** Deploy prune step keeps only newest N versions:

```bash
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

### Update Not Detected

**Problem:** New version deployed but app doesn't reload.

**Fix:** Four-layer detection system:
1. Service worker uses network-first for `version.json`
2. Bootstrap gate fetches version before app loads
3. Promise never resolves on reload (blocks UI)
4. `controllerchange` fallback for SW replacement

---

## Testing Issues

### Low Coverage Files

| File | Coverage | Reason |
|------|----------|--------|
| `json-request.js` | 40% | ESM mocking complexity |
| `lookups.js` | 37% | Fetch + auth chains |

**Mitigation:** Focus testing on core logic. Accept lower coverage for HTTP wrappers.

### Integration Tests Need Server

**Problem:** Integration tests fail without Hydrogen server.

**Fix:** Tests automatically skip with clear message if credentials unavailable.

```bash
export HYDROGEN_SERVER_URL=https://lithium.philement.com
export HYDROGEN_DEMO_USER_NAME=testuser
export HYDROGEN_DEMO_USER_PASS=usertest
export HYDROGEN_DEMO_API_KEY=EveryGoodBoyDeservesFudge
```

---

## Authentication Issues

### JWT Not Stored

**Check:**
1. Login successful (no 401/429)
2. Token in response
3. localStorage accessible
4. `localStorage.setItem('lithium_jwt', token)` called

### Token Not Attached to Requests

**Check:** `json-request.js` adds `Authorization: Bearer <token>` header automatically if JWT exists.

### Auto-Renewal Not Working

**Check:** Token has 80% lifetime remaining. If expired, renewal fails and `auth:expired` fires.

---

## Event Bus Issues

### Events Not Firing

**Check:**
1. Import correct: `import { eventBus, Events } from '../../core/event-bus.js'`
2. Listener added before emit
3. Use correct event name

### Memory Leaks from Listeners

**Fix:** Always remove listeners in `teardown()`:

```javascript
teardown() {
  eventBus.off(Events.AUTH_LOGIN, this.handleLogin);
}
```

Or use `once()` for one-time listeners.

---

## CSS & Styling Issues

### Button Group Design

Use the connected button pattern:

```html
<div class="login-btn-group">
  <button class="login-btn-icon">...</button>
  <button class="login-btn-primary">Login</button>
  <button class="login-btn-icon">...</button>
</div>
```

Key: 2px gaps, shared height, rounded corners on ends only.

### Slot Button Injection

Managers inject buttons into the slot's existing header/footer:

```javascript
const mainMgr = this.app._getMainManager();
const slotId = mainMgr._slotId(managerId);

mainMgr.addHeaderButtons(slotId, [
  { icon: 'fa-refresh', onClick: () => this.refresh() }
]);

mainMgr.addFooterButtons(slotId, 'right', [
  { icon: 'fa-export', onClick: () => this.export() }
]);
```

---

## Known Limitations

1. **Config path** — Fetches `/config/lithium.json` relative to root; may need adjustment for path-prefixed deployments.
2. **Permissions fallback** — Allow-all when no Punchcard; should be configurable for production.
3. **json-request.js** — No way to suppress `auth:expired` event for specific requests.

---

## Quick Reference Links

| Topic | Document |
|-------|---------|
| Coding standards | [LITHIUM-INS.md](LITHIUM-INS.md) |
| Development | [LITHIUM-DEV.md](LITHIUM-DEV.md) |
| Testing | [LITHIUM-TST.md](LITHIUM-TST.md) |
| Managers | [LITHIUM-MGR.md](LITHIUM-MGR.md) |
| Tables | [LITHIUM-TAB.md](LITHIUM-TAB.md) |
| Deployment | [LITHIUM-WEB.md](LITHIUM-WEB.md) |