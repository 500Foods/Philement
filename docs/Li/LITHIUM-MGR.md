# Lithium Managers

This document describes each manager in Lithium — both completed work and planned features.

---

## What is a Manager?

A **manager** is a self-contained feature module with UI. Each manager:

- Has its own directory under `src/managers/`
- Exports a default class with `init()`, `render()`, and `teardown()` lifecycle
- Is independently loadable via dynamic `import()`
- Receives the app instance and container on construction

---

## Manager Types

Lithium has three types of managers:

| Type | Description | Access |
|------|-------------|--------|
| **Menu Manager** | Appears in sidebar navigation | Punchcard-gated |
| **Utility Manager** | Fixed sidebar footer button | Always available |
| **Standalone Manager** | URL-driven, no chrome | Direct URL access |

---

## Manager Documentation

### Overview

- [LITHIUM-MGR-LOGIN.md](LITHIUM-MGR-LOGIN.md) - Login Manager detailed documentation
- [LITHIUM-MGR-MAIN.md](LITHIUM-MGR-MAIN.md) - Main Manager detailed documentation

---

## Implemented Managers

### ✅ Login Manager

**Location:** `src/managers/login/`

**Type:** Menu Manager (ID: special — shown before auth)

**Purpose:** JWT authentication against Hydrogen backend

**Features:**

- POSTs to Hydrogen `/api/auth/login`
- Handles 401 (bad creds), 429 (rate limit), 5xx, network errors
- FOUC prevention, auto-focus username
- Loading spinner on submit
- Fade-out transition on success

**Status:** ✅ Deployed at <https://lithium.philement.com>

---

### ✅ Main Manager

**Location:** `src/managers/main/`

**Type:** Menu Manager (ID: special — wrapper for all authenticated views)

**Purpose:** Main layout — sidebar + header + workspace

**Features:**

- Sidebar buttons from `getPermittedManagers()`
- Lazy-load managers on click
- Header: manager name, offline indicator, version, theme, profile, logout
- Sidebar: collapsible (desktop), overlay (mobile)
- Logout panel with keyboard shortcuts

**Status:** ✅ Implemented

---

### ✅ Style Manager (Legacy)

**Location:** `src/managers/style-manager/`

**Type:** Menu Manager (ID: 1 — legacy/deprecated)

**Purpose:** Theme management — list, apply, edit, create themes

**Features:**

- **List View:** Tabulator table (name, date, owner, actions)
- **CSS View:** CodeMirror editor with CSS syntax highlighting
- Theme application via dynamic `<style>` injection
- DOMPurify sanitization
- Feature-gated UI (view, edit, delete)

**Feature IDs (Punchcard):**

| ID | Description |
|----|-------------|
| 1 | View all styles (read-only) |
| 2 | Edit all styles |
| 3 | Use visual editor |
| 4 | Use CSS code editor |
| 5 | Delete themes |

**Status:** ✅ Implemented (Legacy — may be removed in future)

---

### ✅ Session Log Manager (Utility)

**Location:** `src/managers/session-log/`

**Type:** Utility Manager (key: `session-log`)

**Purpose:** Display current session's client-side action log

**Features:**

- CodeMirror read-only viewer (One Dark theme)
- Refresh button
- Coverage link to `/coverage/`
- Clear archived sessions

**Access:** Sidebar footer button (always available)

**Status:** ✅ Implemented

---

### ✅ User Profile Manager (Utility)

**Location:** `src/managers/profile-manager/`

**Type:** Utility Manager (key: `user-profile`)

**Purpose:** User preferences

**Features:**

- Language selector
- Date format (Intl.DateTimeFormat)
- Time format (12h/24h)
- Number format
- Default database
- Active theme quick-apply

**Status:** ✅ Implemented (save flow pending API)

---

### ✅ Queries Manager

**Location:** `src/managers/queries/`

**Type:** Menu Manager (ID: 4)

**Purpose:** Query builder and execution interface

**Features:**

- **Left Panel:** Tabulator table with list of queries
- **Right Panel:** Tabbed interface with SQL editor, Summary (Markdown), Collection (JSON), and Preview tabs
- **LithiumTable Component:** Uses the reusable `LithiumTable` class with integrated Navigator (Control, Move, Manage, Search blocks)
- **Edit Mode:** Inline cell editing with dirty state tracking
- **Footer:** Print, Email, Export controls with View/Data mode toggle
- **Template System:** Save/load column configurations via localStorage
- **Column Chooser:** Popup to show/hide/reorder columns

**Navigator Blocks:**

- **Control:** Refresh, Filter toggle, Menu (column filters), Width presets, Layout mode, Templates
- **Move:** First, Previous Page, Previous, Next, Next Page, Last
- **Manage:** Add, Duplicate, Edit, Save, Cancel, Delete
- **Search:** Magnifying glass + text input + clear button

**QueryRefs Used:**

| Ref | Purpose |
|-----|---------|
| 25 | Query List (all queries) |
| 27 | Get System Query (details) |
| 28 | Update System Query |
| 32 | Query Search |

**Status:** ✅ Fully Implemented with LithiumTable component

**Documentation:** [LITHIUM-MGR-QUERY.md](LITHIUM-MGR-QUERY.md)

---

### ✅ Lookups Manager

**Location:** `src/managers/lookups/`

**Type:** Menu Manager (ID: 5)

**Purpose:** Manage lookup tables (key-value pairs used throughout the system)

**Features:**

- **Dual-Table Interface:** Parent/child relationship between lookups and their values
- **Left Panel (Parent):** List of all lookup tables (Lookup Names)
- **Right Panel (Child):** Values for the selected lookup
- **Two LithiumTable Instances:** Both tables use the reusable component with full Navigator support
- **Parent/Child Relationship:** Selecting a lookup in the parent loads its values in the child
- **Resizable Splitter:** Drag to resize left panel (200-600px range)
- **Collapsible Left Panel:** Collapse/expand button in child header
- **Footer:** Four select options — View/Data toggles for both parent and child tables

**Navigator Blocks (per table):**

- **Control:** Refresh, Filter toggle, Menu, Width, Layout, Templates
- **Move:** First, Previous Page, Previous, Next, Next Page, Last
- **Manage:** Add, Duplicate, Edit, Save, Cancel, Delete
- **Search:** Text search with clear button

**QueryRefs Used:**

| Ref | Purpose |
|-----|---------|
| 30 | Lookup Names (list of all lookups) |
| 34 | Get Lookup List (values for a specific lookup) |

**Status:** ✅ Fully Implemented with dual LithiumTable components

**Documentation:** [LITHIUM-MGR-LOOKUPS.md](LITHIUM-MGR-LOOKUPS.md)

---

## LithiumTable Component

The **LithiumTable** is a reusable, modular component that combines Tabulator with a custom Navigator control bar. It was extracted from the Query Manager implementation to enable consistent table interfaces across all managers.

### Architecture

**Files:**

- `src/core/lithium-table-base.js` — Core functionality (initialization, events, navigation)
- `src/core/lithium-table-ops.js` — CRUD operations mixin (add, edit, save, delete)
- `src/core/lithium-table-ui.js` — Navigator UI mixin (popups, column chooser, filters)
- `src/core/lithium-table-main.js` — Combined LithiumTable class export
- `src/core/lithium-table.css` — Component styles

### Usage

```javascript
import { LithiumTable } from '../../core/lithium-table-main.js';

const table = new LithiumTable({
  container: document.getElementById('table-container'),
  navigatorContainer: document.getElementById('nav-container'),
  tablePath: 'lookups/lookups-list',  // JSON config path
  queryRef: 30,                        // QueryRef for data loading
  cssPrefix: 'lookups-parent',         // CSS class prefix
  storageKey: 'lookups_parent_table',  // localStorage key
  app: this.app,                       // App instance for API
  readonly: false,                     // Enable editing
  onRowSelected: (rowData) => { /* ... */ },
  onDataLoaded: (rows) => { /* ... */ },
});

await table.init();
await table.loadData();
```

**Status:** ✅ Implemented and used by Queries and Lookups managers

**Documentation:** [LITHIUM-TAB.md](LITHIUM-TAB.md)

---

## Placeholder Managers

These are registered in `app.js` but not yet implemented:

### ⬜ Session Logs

**Type:** Menu Manager (ID: 2)

**Status:** Placeholder

---

### ⬜ Version History

**Type:** Menu Manager (ID: 3)

**Status:** Placeholder

---

### ⬜ Dashboard

**Type:** Menu Manager (ID: 8)

**Status:** Placeholder

---

### ⬜ Document Library

**Type:** Menu Manager (ID: 9)

**Status:** Placeholder

---

### ⬜ Media Library

**Type:** Menu Manager (ID: 10)

**Status:** Placeholder

---

### ⬜ Diagram Library

**Type:** Menu Manager (ID: 11)

**Status:** Placeholder

---

### ⬜ Reports

**Type:** Menu Manager (ID: 12)

**Status:** Placeholder

---

### ⬜ Chats

**Type:** Menu Manager (ID: 6)

**Status:** Placeholder

---

### ⬜ AI Auditor

**Type:** Menu Manager (ID: 7)

**Status:** Placeholder

---

## Manager ID Registry

| ID | Manager | Status |
|----|---------|--------|
| 1 | Style Manager (Legacy) | ✅ Implemented |
| 2 | Session Logs | ⬜ Placeholder |
| 3 | Version History | ⬜ Placeholder |
| 4 | Queries | ✅ **Implemented** |
| 5 | Lookups | ✅ **Implemented** |
| 6 | Chats | ⬜ Placeholder |
| 7 | AI Auditor | ⬜ Placeholder |
| 8 | Dashboard | ⬜ Placeholder |
| 9 | Document Library | ⬜ Placeholder |
| 10 | Media Library | ⬜ Placeholder |
| 11 | Diagram Library | ⬜ Placeholder |
| 12 | Reports | ⬜ Placeholder |

---

## Planned Managers

### Popup Panels (Not Full Managers)

Popup panels are lightweight overlays, not registered managers:

| Popup | Purpose |
|-------|---------|
| Logout Panel | Already implemented in Main Manager |
| Manager Switcher | Switch between multiple instances |
| Annotations | Scratch-pad notes overlay |

---

## Creating a New Manager

### Menu Manager

1. Create directory: `src/managers/new-manager/`
2. Create `new-manager.js`:

   ```javascript
   export default class NewManager {
     constructor(app, container) {
       this.app = app;
       this.container = container;
     }
     async init() {
       await this.render();
       this.setupEventListeners();
     }
     async render() {
       const response = await fetch('/src/managers/new-manager/new-manager.html');
       this.container.innerHTML = await response.text();
     }
     teardown() {
       // Clean up
     }
   }
   ```

3. Create `new-manager.html` template
4. Create `new-manager.css` styles
5. Register in `app.js` `managerRegistry` with numeric ID
6. Add to `main.js` `managerIcons`

### Utility Manager

Same as Menu Manager, but:

1. Register in `app.js` `utilityManagerRegistry` with string key
2. Add import case in `_importUtilityManager()`
3. Add button to `main.html` sidebar footer
4. Wire click in `main.js` to `app.showUtilityManager()`

---

## Manager Lifecycle

```javascript
// Loading a manager
const { default: ManagerClass } = await import(managerPath);
const manager = new ManagerClass(app, container);
await manager.init();  // Calls render() internally

// Switching managers (state preserved)
manager.container.classList.remove('visible');

// Returning to manager
manager.container.classList.add('visible');

// Unloading (rare - usually state is preserved)
manager.teardown();
```

---

## Event System

Managers communicate via the Event Bus:

```javascript
import { eventBus, Events } from '../../core/event-bus.js';

// Emit events
eventBus.emit(Events.AUTH_LOGIN, { userId: 123 });

// Listen for events
eventBus.on(Events.THEME_CHANGED, (e) => {
  console.log('Theme changed:', e.detail);
});
```

**Standard Events:**

| Event | When |
|-------|------|
| `auth:login` | Login success |
| `auth:logout` | User logs out |
| `theme:changed` | Theme applied |
| `locale:changed` | Language changed |
| `manager:loaded` | Manager finished init |
| `manager:switched` | Active manager changed |

---

## Related Documentation

- Development setup: [LITHIUM-DEV.md](LITHIUM-DEV.md)
- Libraries: [LITHIUM-LIB.md](LITHIUM-LIB.md)
- Deployment: [LITHIUM-WEB.md](LITHIUM-WEB.md)
- FAQ: [LITHIUM-FAQ.md](LITHIUM-FAQ.md)
