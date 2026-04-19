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

### Animating Icons Inside Host-Regenerated DOM (Tabulator Group Arrows)

**Problem:** The expand/collapse arrow in a Tabulator group header doesn't animate — it snaps instantly between its two rotations.

**Cause:** Tabulator's `Group.generateElement()` wipes the group header's child nodes and rebuilds them from the `groupHeader()` HTML string on every toggle. The resulting `<fa>` element is brand new, with no previous computed style for a CSS transition to animate from. The FA three-stage pipeline (`<fa>` → `<i>` → `<svg>`) replaces the element further, making inline-style caching fragile.

**Fix:** Use **prior-state pinning** — the pattern documented as Approach 3 in [LITHIUM-ICN.md](LITHIUM-ICN.md#approach-3-prior-state-pinning-for-host-regenerated-icons). Canonical implementation in `_syncGroupIconsNow()` in [`lithium-table-base.js`](../../elements/003-lithium/src/tables/lithium-table-base.js).

Key points that previous attempts got wrong:

1. **CSS-only won't work** — the new icon has no prior state to transition from.
2. **Inline `style.transform` is fragile** — FA replaces the element between the pin and the release, dropping the inline style.
3. **An in-flight guard is mandatory** — Tabulator fires multiple events (`groupVisibilityChanged`, `renderComplete`, `tableRedrawn`) for one toggle. Without guarding, the second sync's "defensive cleanup" strips the anim-from-* class off the row *before the browser paints the pinned frame*, so the transition never fires. Track animating rows in a `Set<HTMLElement>` and skip them during cleanup until the release rAF runs.
4. **Forced reflow between pin and release is mandatory** — read `container.offsetHeight` after adding the anim class. Without it the browser coalesces the class-add and class-remove into one paint and no transition fires.

Symptoms that lead here: "the icon's final rotation is correct, but there's no animation." Every failed attempt over several iterations produced exactly that symptom because one of the four points above was missing.

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

### Library Migrations

1. **jsoneditor → vanilla-jsoneditor → CodeMirror JSON mode:** The JSON editor went through two migrations:
   - **March 2026:** `jsoneditor` (v10.x) → `vanilla-jsoneditor` (v2.x), the maintained successor by the same author.
   - **April 2026:** `vanilla-jsoneditor` → CodeMirror 6 with `@codemirror/lang-json`. This consolidated all editor types (SQL, Markdown, JSON, CSS) onto one library. The `init/jsoneditor-init.js` file remains as dead code.
   - JSON editing now uses the same `buildEditorExtensions()` + `createReadOnlyCompartment()` pattern as all other CodeMirror instances via `codemirror-setup.js`.

### PWA

1. Network-first for `version.json` ensures freshness
2. Block UI during reload to prevent flash
3. Service worker must be registered early (not on window.load)

### Column Property Names (Canonical)

The canonical property for column header text is **`title`**. The legacy property `display` has been retired.

**Migration:**
- Change `display: "Name"` → `title: "Name"` in all tableDef JSON files
- The validator (`validateTableDef()`) will warn if unknown properties like `display` are found
- Code no longer falls back to `display` when `title` is missing

**Files affected:**
- All table definitions in Lookup 059 (Keys 1+)
- Column Manager column definitions
- Template extraction/capture code

**Note:** The `title` property aligns with Tabulator's native naming convention.

---

## Table Definition Merge Issues

### Param Objects Not Merging (Fixed in Phase 6)

**Problem:** Changing `formatterParams.precision` in Lookup 059 Key 0 didn't apply to tables with saved templates. The `formatterParams` object from templates completely replaced the coltype defaults.

**Root Cause:** Shallow merge using spread operator (`{ ...base, ...overlay }`) replaced entire `formatterParams` objects instead of merging their properties.

**Fix:** Deep merge for param objects. The following properties are now deep-merged across all three stages:
- `formatterParams`, `editorParams`, `headerFilterParams`
- `sorterParams`, `accessorParams`, `mutatorParams`
- `bottomCalcFormatterParams`, `downloadFormatterParams`, `downloadCalcParams`, `clipboardParams`

**Example:**
```javascript
// Stage 1 (coltype.integer): { formatterParams: { thousand: ",", precision: 0 } }
// Stage 3 (template):       { formatterParams: { precision: 2 } }
// Result (before fix):      { formatterParams: { precision: 2 } }  // thousand lost!
// Result (after fix):       { formatterParams: { thousand: ",", precision: 2 } }
```

**Implementation:**
- `deepMergeParams()` helper in `column-management.js`
- Used in `mergeColumnsWithTableDef()` (Stage 2) and `_applyDefaultTemplate()` (Stage 3)

---

### Double Column Discovery (Fixed in Phase 6)

**Problem:** `discoverColumns()` was being called twice per data load for auto-discover tables.

**Root Cause:** Both `loadData()` (explicit call) and `dataLoaded` callback (Tabulator event) triggered discovery.

**Fix:** Removed the `dataLoaded` callback in `initTable()`. `loadData()` already calls `discoverColumns()` after `setData()`.

---

### TableDef Cache Not Clearing on Logout (Fixed in Phase 6)

**Problem:** Table definitions from Lookup 059 persisted across user sessions.

**Fix:** Added `AUTH_LOGOUT` event listener in `tabledef-loader.js` to clear `_tableDefCache` on logout. Cache is now truly session-only.

---

### Row Selection Lost After Refresh (Fixed in Phase 6)

**Problem:** After clicking the Refresh button, the currently selected record was not reselected (first row was selected instead).

**Root Cause:** The `loadData()` function determines which row to select using `table.getSelectedRowId() ?? table.restoreSelectedRowId()`. After a full table rebuild during refresh, there's no current selection, so it falls back to `restoreSelectedRowId()`. However, `loadData()` calls `autoSelectRow()` internally which was selecting the first row when it couldn't match the restored ID properly. The explicit restoration step after `loadData()` was missing.

**Fix:** 
- Disable `autoSelectRow` during the refresh data load (replace with no-op)
- Await data load completion
- Explicitly call `autoSelectRow(capturedRowId)` with the ID captured at refresh start
- Fire `onRowSelected` event to update manager UI

**Code flow:**
```javascript
// Before (broken):
await table.loadData();  // Selects first row because getSelectedRowId() returns null
// No explicit restoration

// After (fixed):
const savedAutoSelectRow = table.autoSelectRow;
table.autoSelectRow = () => {};  // Disable during load
await table.loadData();          // Load without selection
table.autoSelectRow = savedAutoSelectRow;
await new Promise(r => requestAnimationFrame(r));
table.autoSelectRow(capturedRowId);  // Explicitly restore correct row
```

---

### Parent/Child Table Refresh Coordination (Fixed in Phase 6)

**Problem:** In Lookups Manager (which has a parent table and child table), after refreshing the parent table, the child table wouldn't reload its data. The parent correctly restored its selected row, but the child remained empty or stale.

**Root Cause:** The `handleParentRowSelected()` function has a guard (`_loadedChildLookupKey === lookupKey`) to prevent duplicate loads when the same lookup is selected. After a parent refresh, this guard was still set from before the refresh, so the child load was skipped.

**Fix:** 
- Added `onRefreshComplete()` callback support to `reloadConfiguration()`
- Managers can implement this callback to clear child table state
- Lookups Manager clears `_loadedChildLookupKey` in `onRefreshComplete`, forcing child reload on next parent selection

**Code in Lookups Manager:**
```javascript
// In parent table options
onRefreshComplete: () => {
  // Clear child load state so child reloads when parent row is reselected
  this._loadedChildLookupKey = null;
},
```

**Flow after fix:**
1. Parent refresh captures state and destroys table
2. Parent recreates table and restores row selection
3. `onRowSelected` fires for restored row
4. `handleParentRowSelected` checks `_loadedChildLookupKey` (now null after `onRefreshComplete`)
5. Child data loads fresh via `loadChildData()`

---

### Refresh Clears All TableDefs (Fixed in Phase 6)

**Problem:** After clicking Refresh in one manager (e.g., Lookups Manager), other managers (e.g., Query Manager) would render with auto-discovered columns instead of their proper table definitions from Lookup 59.

**Root Cause:** The `refreshTabulatorSchemas()` function in `lookups.js` used `fetchBatchQueries()` which doesn't include JWT authentication. The server returned empty results, so the schemas cache was cleared but not repopulated. When other managers loaded, they couldn't find their table definitions and fell back to auto-discovery.

**Fix:** Changed `refreshTabulatorSchemas()` to use `authQuery()` instead of `fetchBatchQueries()` to ensure proper JWT authentication:

```javascript
// Before (broken):
const batchResults = await fetchBatchQueries([60], '060');
// Returns empty array due to missing authentication

// After (fixed):
const { authQuery } = await import('./conduit.js');
const freshSchemas = await authQuery(null, 60);
// Properly authenticated, returns full schema data
```

---

## Phase 7 — String Editing Fixes

### String Editor Not Opening (Fixed in Phase 7)

**Problem:** String columns would not open an editor when clicked in edit mode. Instead, clicking any cell would cancel edit mode entirely.

**Root Cause:** Two issues:
1. **TableDefs used `editor: "string"` instead of `editor: "input"`** — Tabulator doesn't recognize `"string"` as a valid editor type. The correct value is `"input"` for single-line text editing.
2. **Edit mode toggle behavior** — The Edit button would toggle edit mode on/off. If you clicked Edit to enter edit mode, then clicked Edit again, it would exit. But the desired behavior is: Edit button always exits edit mode (saving if dirty).

**Fix:**
1. Changed table definitions to use `editor: "input"` for string columns (and `editor: "textarea"` for text columns).
2. Modified `handleEdit()` in `lithium-table-ops.js` to always exit edit mode when clicked while editing:

```javascript
// Before: Toggle behavior - would re-enter edit mode if not editing
if (this.isEditing && this.editingRowId === selectedId) {
  // save and exit
}

// After: Always exit when in edit mode
if (this.isEditing) {
  // save (if dirty) and exit
}
```

### Edit Mode Exiting on Non-Editable Cell Clicks (Fixed in Phase 7)

**Problem:** Clicking on a non-editable cell (like a primary key or calculated column) while in edit mode would sometimes exit edit mode unexpectedly.

**Root Cause:** The `cellMouseDown` event was triggering row selection even when in edit mode, which could cause race conditions with `handleRowSelected`'s auto-save logic.

**Fix:** Modified `cellMouseDown` handler to skip row selection when already in edit mode:

```javascript
table.table.on('cellMouseDown', (e, cell) => {
  // ...
  // Only select row on mouse down if not already editing
  if (!table.isEditing) {
    table.selectDataRow(cell.getRow());
  }
});
```

This ensures that while editing, row changes are only handled through the explicit `cellClick` → `handleRowSelected` flow, which properly manages auto-save and edit mode exit.

### Same-Row Clicks Exiting Edit Mode (Fixed in Phase 7)

**Problem:** Clicking on different cells within the same row (while in edit mode) would unexpectedly exit edit mode.

**Root Cause:** The `handleRowSelected` function was comparing row IDs and triggering auto-save logic even when the same row was re-selected. This could happen when `cellClick` events triggered row selection.

**Fix:** Added defensive check in `handleRowSelected` to early-return when the same row is clicked:

```javascript
// If editing and clicked the same row, just update UI and return
if (table.isEditing && table.editingRowId === newRowId) {
  table.updateSelectorCell?.(row, true);
  return;
}
```

This ensures that clicking different cells within the same editing row stays in edit mode.

### CodeMirror ESC and Ctrl+Enter Shortcuts (Fixed in Phase 7)

**Problem:** When focus is inside a CodeMirror editor, pressing ESC doesn't cancel edit mode, and there's no keyboard shortcut to save changes.

**Root Cause:** CodeMirror captures keyboard events and doesn't propagate them to the table's keyboard handler.

**Fix:** Added CodeMirror keymap bindings for Escape and Ctrl+Enter:

1. In `codemirror-setup.js`, added support for `onEscape` and `onSave` callbacks in `buildEditorExtensions()`
2. In `ManagerEditHelper`, added `getCodeMirrorKeymapOptions()` method that returns callbacks bound to `handleCancel()` and `handleSave()`
3. Managers can now pass these options when creating CodeMirror instances:

```javascript
const extensions = buildEditorExtensions({
  language: 'sql',
  ...this.editHelper.getCodeMirrorKeymapOptions(),
});
```

**Shortcuts:**
- **Escape**: Cancel edit mode (same as clicking Cancel button)
- **Ctrl+Enter**: Save changes (same as clicking Save button)

### Composite Primary Key Handling (Fixed in Phase 7)

**Problem:** Tables with composite primary keys (like Lookups with `lookup_id` + `key_idx`) had broken edit mode. Entering edit mode would set `editingRowId` to `undefined`, causing all row comparisons to fail. This caused:
- Same-row clicks to exit edit mode
- Cell editors not opening
- Changes not being saved on row change

**Root Cause:** Multiple places in the code assumed `primaryKeyField` was a single string, but it's an array for composite keys:

```javascript
// BROKEN - assumes single field:
const pkField = this.primaryKeyField || 'id';
const rowId = row.getData()?.[pkField]; // Returns undefined when pkField is an array
```

**Fix:** Updated all locations to use `_getCompositeRowId()` which properly handles both single and composite keys:

**Files fixed:**
1. `src/tables/lithium-table-ops.js` - `enterEditMode()` and `queueCellEdit()`
2. `src/tables/columns/column-management.js` - `applyEditModeGate()` (already correct)

```javascript
// FIXED - handles composite keys:
const pkFields = this.primaryKeyField;
const rowId = this._getCompositeRowId(row.getData(), pkFields); // Returns "43::11"
```

---

## Lookup Column Editing

### Tabulator list editor stores label instead of ID

**Problem:** When using Tabulator's built-in `list` editor with an array of labels `["Active", "Inactive"]`, selecting a value stores the label text (string) instead of the underlying ID (integer). This causes server errors like "QUERYSTATUS(string) is not QUERYSTATUS(INTEGER)".

**Root Cause:** Tabulator's `list` editor with `values: ["label1", "label2"]` returns the selected string value. We need `values: {id1: "label1", id2: "label2"}` to return the ID while displaying the label.

**Fix in `createLookupEditor()`:**

```javascript
// BEFORE: Stored label text
const values = lookupData.map(entry => entry.label);

// AFTER: Stores integer ID, displays label
const values = {};
lookupData.forEach(entry => {
  values[entry.id] = entry.label;
});
```

**Additional safeguard:** Added a `mutator` to ensure values are always integers:

```javascript
tabulatorCol.mutator = function(value) {
  if (value === null || value === undefined || value === '') {
    return 0;
  }
  // If Tabulator returns the label string, look up the ID
  if (typeof value === 'string') {
    const entry = lookupData.find(e => e.label === value);
    if (entry) return entry.id;
  }
  return parseInt(value, 10) || 0;
};
```

**Phase:** Fixed in Phase 8 (April 18, 2026)

**Files changed:**
- `src/tables/resolution/lookup-loader.js` — `createLookupEditor()` function
- `src/tables/lithium-table.js` — mutator assignment in `resolveColumn()`

---

## Related Documentation

- Development: [LITHIUM-DEV.md](LITHIUM-DEV.md)
- Libraries: [LITHIUM-LIB.md](LITHIUM-LIB.md)
- Managers: [LITHIUM-MGR.md](LITHIUM-MGR.md)
- Deployment: [LITHIUM-WEB.md](LITHIUM-WEB.md)
