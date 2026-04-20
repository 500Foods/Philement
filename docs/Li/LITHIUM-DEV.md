# Lithium Development Environment

This document describes the development environment, build tools, and workflow for working with Lithium.

---

## IMPORTANT: Use Session Logging, Not console.log

When debugging issues in Lithium, always use the session logging system instead of `console.log`. This ensures:

- All debug output is preserved for later review
- Debug messages are visible in the browser's Lithium session log panel
- Messages can be filtered by subsystem and status
- Debug info persists across hot reloads during development

### How to Log

```javascript
import { log, Subsystems, Status } from '../core/log.js';

// Basic usage
log(Subsystems.MANAGER, Status.INFO, 'Export button clicked');
log(Subsystems.MANAGER, Status.DEBUG, 'Popup rect:', { width: 140, height: 283 });

// For complex objects, log as a group
log(Subsystems.MANAGER, Status.DEBUG, 'Export options:', { 
  exportOptions, 
  extraOptions 
});
```

### Status Levels

| Status | Purpose |
|--------|---------|
| `Status.DEBUG` | Development debugging info |
| `Status.INFO` | General information |
| `Status.WARN` | Warnings (non-critical issues) |
| `Status.ERROR` | Errors that need attention |
| `Status.SUCCESS` | Successful operations |

### Subsystems

Use the relevant subsystem: `Subsystems.MANAGER`, `Subsystems.TABLE`, `Subsystems.UI`, etc.

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
| `npm test` | Run all tests (672 tests) |
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
| [`manager-ui.js`](elements/003-lithium/src/core/manager-ui.js) | Shared UI utilities (keyboard shortcuts, footer buttons, font popup) |
| [`manager-edit-helper.js`](elements/003-lithium/src/core/manager-edit-helper.js) | Consolidated edit mode, dirty tracking, and save/cancel button management for all managers |
| [`codemirror-setup.js`](elements/003-lithium/src/core/codemirror-setup.js) | Shared CodeMirror config, `setEditorContentNoHistory()`, `updateUndoRedoButtons()` |

## Shared UI Utilities (`manager-ui.js`)

The `manager-ui.js` module provides common UI functionality used across all managers:

### `createFontPopup(options)`

Creates a reusable font settings popup (size, family, weight). Used by Lookups Manager, Style Manager, and any manager with an editor panel.

```javascript
import { createFontPopup } from '../../core/manager-ui.js';

const { popup, toggle, show, hide, getState, setState } = createFontPopup({
  anchor: this.elements.fontBtn,
  fontSize: 14,
  fontFamily: 'var(--font-mono)',
  fontWeight: 'normal',
  onChange: ({ fontSize, fontFamily, fontWeight }) => {
    // Apply to editor elements
  },
});
```

CSS classes: `.manager-font-popup`, `.manager-font-popup-row`, `.manager-font-popup-input`, `.manager-font-popup-select`, `.manager-font-popup-style-btn` (defined in `manager-ui.css`).

### `setupManagerFooterIcons(group, options)`

Creates Print, Email, Export, and Reports select buttons in the manager footer.

### `setupFooterButtons(slotId, managerId, options)`

Creates the standard footer action icons (Crimson, Notifications, Concierge, Annotations, Tours, Training).

### Popup Positioning and Behavior

All manager UI popups follow standardized positioning, styling, and animation rules:

#### **Header Popups** (Keyboard Shortcuts, Zoom)
- **Positioning**: Drop down from header buttons
- **Anchor**: Top-right corner of popup positioned 1px below bottom-right corner of button
- **Corners**: Top corners squared off (`border-radius: 0 0 var(--border-radius-lg) var(--border-radius-lg)`)
- **Animation**: Scales from top-right origin (expands down and left)
- **Examples**: Keyboard shortcuts popup, Zoom popup

#### **Footer Popups** (Tours, Training, Export Formats)
- **Positioning**: Rise up from footer buttons
- **Anchor**: Bottom-right corner of popup positioned 1px above top-right corner of button
- **Corners**: Bottom corners squared off (`border-radius: var(--border-radius-lg) var(--border-radius-lg) 0 0`)
- **Animation**: Scales from bottom-left origin (expands up and left)
- **Examples**: Tours popup, Training popup, Export format popups
- **Export Popups**: Manager-specific export menus (Queries, Lookups, Style Manager, Version History) use the same positioning system but are implemented separately in each manager

#### **Common Behavior**
- **Exclusive Opening**: Opening any popup closes all other popups via `closeAllPopups()` event
- **Animation**: Scale from 50% to 100% with fade-in, reverse on close
- **Timing**: Uses `--transition-duration` (350ms) for consistency
- **Z-Index**: 10001 for visibility above most UI elements

#### **CSS Classes**
- `.manager-ui-popup` - Base popup styling (background, border, shadow, transitions)
- `.manager-header-popup` - Header popup styling (top squared corners, top-right transform origin)
- `.manager-footer-popup` - Footer popup styling (bottom squared corners, bottom-left transform origin)
- `.manager-ui-popup.visible` - Visible state (opacity: 1, scale: 1)

#### **Implementation Functions**
- `positionPopup(popup, button, alignment)` - Positions popup relative to button
  - `alignment: 'header-dropdown'` - Header popup positioning
  - `alignment: 'footer-riseup'` - Footer popup positioning
- `closeAllPopups()` - Closes all manager UI popups and dispatches global close event
- Event: `'close-all-popups'` - Custom event all popups listen for to close when others open

#### **Troubleshooting**
- **Popups not closing**: Ensure all popup functions call `closeAllPopups()` and listen for `'close-all-popups'` event
- **Positioning issues**: Use `positionPopup()` after popup is added to DOM, and call `requestAnimationFrame()` twice for proper dimension calculation
- **Animation problems**: Check `transform-origin` matches expected expansion direction
- **Corner styling**: Verify CSS classes are applied correctly and border-radius overrides are working

---

## Coding Patterns

- Whether in JavaScript or CSS, use --transition-duration or a function of it for most animations. This makes it easy to validate that the animations are working properly by adjusting this variable in CSS. There are functions already for retrieving this value in JavaScript from the CSS environment. Do NOT change the CSS value for this as we change it ourselves during testing.
- Generally, our default UI avoides the use of opacity for most things just as part of its overall theme
- Please read additional doc files if you're working on something, like a Manager, that you've not already read a doc file for. The LITHIUM-TOC.md file has a listing of all doc files, and you should read all applicable files when working on the project. There are many and generally you will be guided to those that you need, but read others if your work touches on those topic areas in any way.
- QueryRefs and Lookups are VERY DIFFERENT THINGS. QueryRefs are a way to invoke SQL commands at the backend, passing parameters and getting typically JSON result sets back. There are many QueryRefs, identified by a number (eg: QueryRef 26). Lookups are enums, lists, and related cotnent. There are many Lookupds, identified by a LOOKUP_ID, and each has many values, using KEY_IDX as a primary key.  You can use a QueryRef (eg: QueryRef 26) to retrieve a Lookup.

---

## Code Hygiene

See [LITHIUM-INS.md](LITHIUM-INS.md) for the full canonical coding standards. The following are the most critical reminders when working on Lithium:

### CSS-First Styling
Prefer CSS classes over inline JavaScript styles. Never use `element.style.property = value` except for dynamic positioning (e.g., drag handles, popup anchors). Define classes in CSS files; toggle with `classList`.

### File Size Discipline
- **Target:** 750 lines per source file
- **Hard limit:** 1,000 lines (per [LITHIUM-INS.md](LITHIUM-INS.md) §2)

When a file approaches the limit, split into logical modules along responsibility lines.

### No Fallback Code
Do not create fallback implementations that duplicate primary feature functionality. Make the primary implementation robust instead (per [LITHIUM-INS.md](LITHIUM-INS.md) §1).

### Clean Build Required
Every phase of active work requires:
- `npm run lint` — zero errors, zero warnings attributable to changes
- `npm run build` — clean production build
- `npm test` — all tests pass (or tests that break are fixed)

---

## Working with Column Types (AI Guide)

When working with LithiumTable column definitions, use the column type documentation:

### Quick Reference

For a fast overview of all available types, see the **Quick Reference** table in:
→ [LITHIUM-TAB-TYPES.md](LITHIUM-TAB-TYPES.md#quick-reference)

### Detailed Type Documentation

Each column type has its own dedicated document in `docs/Li/LITHIUM-TAB-TYPES-*.md`:

| Task | Document |
|------|----------|
| Find a type's default properties | `LITHIUM-TAB-TYPES-DEFAULT.md` |
| Configure a string column | `LITHIUM-TAB-TYPES-STRING.md` |
| Configure a lookup column | `LITHIUM-TAB-TYPES-LOOKUP.md` |
| Use icon lookups | `LITHIUM-TAB-TYPES-LOOKUPICON.md`, `LOOKUPICONTEXT.md`, `LOOKUPICONLIST.md` |
| Numeric types | `INTEGER.md`, `DECIMAL.md`, `CURRENCY.md`, `PERCENT.md` |
| Date/time types | `DATE.md`, `DATETIME.md`, `TIME.md` |

### JSON Schema (Programmatic Access)

For validation or code generation, use the JSON Schema:
→ [LITHIUM-TAB-TYPES.schema.json](LITHIUM-TAB-TYPES.schema.json)

This schema defines all 25 column types and can be used to:
- Validate column definitions
- Generate TypeScript types
- Enable autocomplete in JSON editors

### Key Patterns

1. **Type selection**: Choose the type that matches your data semantics (e.g., `integer` for IDs, `currency` for money)
2. **Property override**: Column definitions override type defaults; only specify what differs
3. **Lookup types**: Require `lookupRef` on the column definition (not in the type)
4. **Enum types**: Require `values` in `editorParams` on the column definition

### For AI Agents

When writing code that defines or modifies LithiumTable columns:
1. First check the Quick Reference table to identify the appropriate type
2. Read the detailed type document for exact property values
3. Validate column definitions against the JSON Schema
4. Remember: column definitions go in table configs; types are the base defaults

---

## Lessons Learned

This section captures patterns and gotchas discovered during development that can help avoid similar issues in future work.

### CSS Transitions and Animations

**Problem:** Transitions not working when adding elements to the DOM.

**Solution:** When you need a transition on a newly added element:
1. Add element to DOM with initial state (e.g., `opacity: 0`)
2. Force a browser reflow: `void element.offsetWidth` 
3. Change to final state (e.g., add `.visible` class) to trigger transition

Similarly for removal:
1. Change to initial state (e.g., remove `.visible` class)
2. Wait for transition duration (`setTimeout` with same duration)
3. Remove element from DOM

### Event Propagation in Nested Handlers

**Problem:** Click handlers on child elements firing multiple times due to parent delegation.

**Solution:** When a child element is inside a container that also has a click handler, use `e.stopPropagation()` in the child handler:

```javascript
childButton.addEventListener('click', (e) => {
  e.stopPropagation(); // Prevents parent's delegated handler from catching this
  doAction();
});
```

### localStorage State Persistence

**Problem:** NaN or Infinity values being saved to localStorage, causing errors on reload.

**Solution:** Always validate numeric values before saving and loading:

```javascript
// Saving - validate with isFinite
const safeWidth = (typeof state.width === 'number' && isFinite(state.width) && state.width > 0) 
  ? Math.round(state.width) : 0;

// Loading - explicit type check
if (savedState && typeof savedState.width === 'number' && savedState.width > 0) {
  initialWidth = savedState.width;
}
```

Avoid truthy checks like `if (savedState.width)` because `0` is falsy even when valid.

### JavaScript Variable Scope with Closures

**Problem:** Using `const` functions inside closures that get reassigned later.

**Solution:** Be careful when trying to wrap functions like:
```javascript
const originalFn = fn;
fn = () => { originalFn(); doSomething(); }; // Works
```

But this doesn't:
```javascript
const handleDragEnd = () => { /* ... */ };
handleDragEnd = () => { /* adds extra logic */ }; // Error: can't reassign const
```

Instead, modify the function directly or use a different pattern:
```javascript
let handleDragEnd = () => { /* ... */ };
handleDragEnd = () => { /* replaces */ };
```

### Defining Constants Before Use

**Problem:** ReferenceError when using constants before they're defined in the file.

**Solution:** Ensure all constants and variables are declared before any code that references them. In long functions, define helper constants at the beginning of the function scope.

### Video Player State

**Problem:** HTMLMediaElement.volume setter throwing errors with NaN values.

**Solution:** Always validate volume before setting:
```javascript
const volume = parseFloat(localStorage.getItem('volume'));
video.volume = (volume !== null && !isNaN(volume)) ? volume : 1;
```

### Table Selection & Navigation Best Practices

When implementing table functionality, follow these patterns established for smooth user experience:

#### Single Row Selection Enforcement

Always ensure tables enforce single row selection:

```javascript
// In row selection handlers
const selectedRows = this.table.getSelectedRows();
const otherRows = selectedRows.filter(row => row !== selectedRow);
if (otherRows.length > 0) {
  this.table.deselectRow(otherRows);
}
```

#### Transition-Aware UI Updates

Prevent button flashing during selection operations:

```javascript
// Set transition flag before operations
this._inSelectionTransition = true;

try {
  // Perform selection changes
  await this.loadData();
  this.autoSelectRow(targetId);
} finally {
  // Clear flag after completion
  this._inSelectionTransition = false;
}
```

#### View-Based Navigation

Navigation buttons should reflect the current filtered view:

```javascript
// Use 'active' rows instead of all rows
const activeRows = this.table.getRows('active');
const selectedIndex = activeRows.findIndex(row => row === selectedRow);
```

#### Robust Primary Key Handling

Support tables with or without explicit primary key definitions:

```javascript
// Fallback logic for ID detection
const idFields = ['id', 'query_id', 'lookup_id', 'key_idx'];
const primaryId = idFields.find(field => rowData[field] != null) || Object.keys(rowData).find(key => key.toLowerCase().includes('id'));
```

#### Per-Parent Child Selection Persistence

For parent-child table relationships, implement per-parent selection memory:

```javascript
// Save per-parent selections
const selections = JSON.parse(localStorage.getItem('child_selections') || '{}');
selections[parentId] = childCompositeId;
localStorage.setItem('child_selections', JSON.stringify(selections));

// Restore on parent selection
const savedChildId = selections[parentId];
if (savedChildId) {
  this.childTable.saveSelectedRowId(savedChildId);
}
```

### Dirty State Tracking and Save Guards

**Critical Pattern:** When implementing dirty detection during row changes, use `getEditingRow()` not `getSelectedDataRow()`:

```javascript
// WRONG: When switching rows, Tabulator has already selected the new row
const currentRow = this.getSelectedDataRow(); // Returns the NEW row!

// CORRECT: Always use the editing row for snapshot/comparison
const editingRow = this.getEditingRow(); // Returns the row being edited
```

**Save Operation Guards:** Always protect against concurrent saves and spurious save attempts:

```javascript
async autoSaveBeforeRowChange() {
  // Guard 1: Prevent concurrent saves
  if (this._saveInProgress) return true;

  // Guard 2: Verify actually dirty
  const actuallyDirty = this.isActuallyDirty?.() || this.isDirty;
  if (!actuallyDirty) return true;

  this._saveInProgress = true;
  try {
    await this.executeSave(row);
    // ... success handling
  } finally {
    this._saveInProgress = false;
  }
}
```

**Type-Safe Comparisons:** Use loose equality for primitive comparisons to handle API type mismatches:

```javascript
// Handles "431" (string from API) vs 431 (number in JS)
const isChanged = snapshotVal != currentVal; // NOT !==
```

**Null/Empty Equivalence:** Treat null/undefined/empty string as equivalent to avoid false positives:

```javascript
const isEmpty = val == null || val === ''; // Treats null, undefined, '' as same
```

---

## Next Steps

- Learn about JavaScript libraries: [LITHIUM-LIB.md](LITHIUM-LIB.md)
- Understand the manager system: [LITHIUM-MGR.md](LITHIUM-MGR.md)
- Lookup tables: [LITHIUM-LUT.md](LITHIUM-LUT.md) (includes debugging tips)
- Deployment: [LITHIUM-WEB.md](LITHIUM-WEB.md)
- Troubleshooting: [LITHIUM-FAQ.md](LITHIUM-FAQ.md)
