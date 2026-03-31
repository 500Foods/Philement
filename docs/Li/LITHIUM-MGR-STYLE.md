# Lithium Style Manager

This document describes the Style Manager, a visual CSS styling interface for the Lithium application that provides interactive element selection and real-time style editing.

---

## Overview

The Style Manager (Manager ID: TBD) provides a three-panel interface for managing CSS styles across different UI sections of the Lithium application. It combines a lookup-based style elements table with a hardcoded sections table and a visual, interactive editing area.

**Key Features:**

- **Three-panel layout** — Lookup 41 elements (left) + Sections (middle) + Editor (right)
- **Visual section editing** — Interactive HTML mockups with clickable targets
- **Real-time preview** — See style changes immediately
- **Multiple editor types** — Section (visual) and JSON modes
- **Integrated toolbar** — Undo/Redo, font controls, pretty printing
- **LithiumSplitter** — Resizable panels using the standard splitter component
- **Shared font popup** — Uses `createFontPopup()` from `manager-ui.js` (not manager-specific)

**Location:** `src/managers/style-manager/`

**Key Files:**

- `style-manager.js` — Main manager class
- `style-manager.css` — Styling
- `style-manager.html` — HTML template

---

## UI Layout

The interface uses a three-panel layout matching the Lookups Manager pattern:

```
┌──────────┬──────────────┬──────────────────────────────────────┐
│          │              │  [◀] [◀] [Section] [JSON] [   ] [↩] [↪] [Aa] [✿] │
│ Lookup 41│  Sections    ├──────────────────────────────────────┤
│ (Styles) │  (Hardcoded) │                                      │
│          │              │  ┌──────────────────────────────────┐│
│ ┌──────┐ │  ┌────────┐  │  │                                  ││
│ │Name  │ │  │Name  D │  │  │   Section Visualization          ││
│ │Col 1 │ │  │Base  0 │  │  │   (Interactive HTML Mockup)      ││
│ │Col 2 │ │  │Semn  0 │  │  │                                  ││
│ │...   │ │  │Cust  0 │  │  │   ○ → Button                     ││
│ └──────┘ │  │Login 0 │  │  │   ○ → Input Field                ││
│          │  │...    │  │  │   ○ → Panel Background           ││
│ [Nav]    │  └────────┘  │  │                                  ││
│          │              │  └──────────────────────────────────┘│
│          │  [Nav]       │                                      │
│          │              │  ┌──────────────────────────────────┐│
│          │              │  │   Style Editor                   ││
│          │              │  │   (CSS/JSON/Visual)              ││
│          │              │  └──────────────────────────────────┘│
└──────────┴──────────────┴──────────────────────────────────────┘
     ▲            ▲
     splitter     splitter
```

### Left Panel: Lookup 41 Table

Displays style elements from Lookup 41 for editing.

**Features:**

- LithiumTable component with full Navigator
- Data loaded via **QueryRef 26** with `INTEGER: { LOOKUPID: 41 }`
- Row selection loads element details in editor panel
- Follows the same pattern as Version History Manager (which loads Lookups 44 and 45)

**Data Loading:**

```javascript
// Uses QueryRef 26 - Get Lookup (same as Version History Manager uses for Lookups 44/45)
const rows = await authQuery(this.app.api, 26, {
  INTEGER: { LOOKUPID: 41 },
});
```

### Middle Panel: Sections Table (Hardcoded)

Static list of UI sections for visual editing. Contains two columns: **Section** (name) and **Delta** (integer showing changes from base theme).

**Features:**

- Fixed, hardcoded list (no CRUD operations)
- Populated immediately after `init()` — no API call needed
- Row selection switches right panel to Section view
- Uses `tablePath: 'style-manager/sections'` with fallback columns only
- Data loaded via direct `setData()` call, not via `loadData()`

**Sections Data:**

Each row has three fields:
- `sectionId` — numeric identifier
- `sectionName` — display name
- `delta` — integer count of style changes from base theme (0 = no changes)

```javascript
const SECTIONS_DATA = [
  { sectionId: 1, sectionName: 'Base Variables', delta: 0 },
  { sectionId: 2, sectionName: 'Semantic Variables', delta: 0 },
  // ... 22 sections total
];
```

### Right Panel: Editor Area

The right panel dynamically changes based on active mode and selection. Uses the **Query Manager** toolbar pattern.

#### Toolbar

The toolbar header uses `subpanel-header-group` with the following layout:

**Left side:**

| Button | Action |
|--------|--------|
| ◀ | Collapse/Expand Left Panel (Lookup 41) |
| ◀ | Collapse/Expand Middle Panel (Sections) |
| **[Section] [JSON]** | Mode toggle buttons |
| `placeholder` | Fills remaining space |

**Right side:**

| Button | Action | Keyboard |
|--------|--------|----------|
| ↩ Undo | Revert last change | `Ctrl+Z` |
| ↪ Redo | Reapply undone change | `Ctrl+Shift+Z` |
| Aa | Font size popup | — |
| ✿ | Pretty print CSS/JSON | `Ctrl+P` |

#### Section Mode (Visual)

When Section mode is active and a section is selected:

1. **Section Visualization** — HTML mockup of the selected UI section
2. **Interactive Targets** — Small circles with arrows pointing at styleable elements
3. **Click Target** — Opens editor for that specific element's styles
4. **Real-time Preview** — Changes apply immediately to mockup

**Currently implemented mockups:**

- **Login** (Section 4) — Full login card with header, inputs, button group
- **Menu** (Section 7) — Menu items mockup

#### JSON Mode

Raw JSON editor for the selected style configuration (not yet implemented).

---

## Architecture

### Class Structure

```javascript
import { ManagerEditHelper } from '../../core/manager-edit-helper.js';

export default class StyleManager {
  constructor(app, container) {
    this.app = app;
    this.container = container;
    this.elements = {};
    this.lookupTable = null;      // LithiumTable for Lookup 41
    this.sectionsTable = null;    // LithiumTable for hardcoded sections
    this.currentMode = 'section'; // 'section' or 'json'
    this.leftSplitter = null;     // LithiumSplitter between left/middle
    this.rightSplitter = null;    // LithiumSplitter between middle/right
    this.fontPopup = null;
    this.jsonEditor = null;
    this.cssEditor = null;        // CodeMirror CSS editor
    this.editHelper = new ManagerEditHelper({ name: 'Style' });
  }

  async init() {
    await this.render();
    this.setupEventListeners();
    this.applyPermissions();
    await this.initLookupTable();   // QueryRef 26 + LOOKUPID 41
    await this.initSectionsTable(); // Hardcoded data via setData()
    this.setupSplitters();
    this.setupModeToggle();
    this.setupToolbar();
    this.initFontPopup();
    this.initPreviewStyle();
    this.setupFooter();             // Wires editHelper to footer buttons
    this.restorePanelState();
    this.show();
  }

  // In initLookupTable():
  //   this.editHelper.registerTable(this.lookupTable);
  //   this.editHelper.registerEditor('css', { getContent, setEditable, boundTable });
  // In initSectionsTable():
  //   this.editHelper.registerTable(this.sectionsTable);
  // In setupFooter():
  //   this.editHelper.wireFooterButtons(saveBtn, cancelBtn, dummyBtn);

  // Table handlers
  async handleLookupRowSelected(rowData) { /* ... */ }
  handleLookupRowDeselected() { /* ... */ }
  async handleSectionRowSelected(rowData) { /* ... */ }
  handleSectionRowDeselected() { /* ... */ }

  // Mode switching
  setMode(mode) { /* 'section' | 'json' */ }

  // Section visualization
  loadSectionMockup(sectionId) { /* ... */ }
  setupInteractiveTargets(targets) { /* ... */ }
  handleTargetClick(target, element, circle) { /* ... */ }
  loadStyleProperties(targetId) { /* ... */ } // Placeholder property editor

  // Toolbar actions
  undo() { /* ... */ }
  redo() { /* ... */ }
  prettify() { /* ... */ }

  // Lifecycle
  onActivate() { /* Redraw tables */ }
  onDeactivate() { /* ... */ }
  cleanup() { this.editHelper?.destroy(); /* Destroy tables, splitters, popups */ }
}
```

### Initialization Order

1. `render()` — Load HTML template, cache DOM elements
2. `setupEventListeners()` — Bind collapse buttons, editor actions, keyboard shortcuts
3. `setupSplitters()` — Create two LithiumSplitter instances (left and right)
4. `applyPermissions()` — Check user claims for edit/delete permissions
5. `initLookupTable()` — Create LithiumTable for Lookup 41
6. `loadLookupData()` — Fetch data via QueryRef 26 with LOOKUPID=41, populate table
7. `initSectionsTable()` — Create LithiumTable for sections
8. `loadSectionsData()` — Populate with hardcoded SECTIONS_DATA via `setData()`
9. `setupModeToggle()` — Bind Section/JSON toggle buttons
10. `setupToolbar()` — Bind Undo/Redo/Prettify buttons
11. `initFontPopup()` — Create font settings popup (uses shared `createFontPopup()` from `manager-ui.js`)
12. `setupFooter()` — Create footer with Print/Email/Export buttons
13. `show()` — Add visible class for fade-in

---

## Three-Panel Layout Pattern

### Why Three Panels?

The Style Manager uses three panels (left + middle + right) because it needs two separate tables visible simultaneously:

1. **Lookup 41 table** (left) — Shows available style elements/themes
2. **Sections table** (middle) — Shows which section of the UI to edit
3. **Editor panel** (right) — Shows the visual mockup or JSON editor

This matches the **Lookups Manager** pattern (Manager ID: 5), which also has left + middle + right panels.

### Panel Width Management

- **Left panel** default width: `280px` (min: `150px`)
- **Middle panel** default width: `350px` (min: `200px`)
- **Right panel** fills remaining space (`flex: 1`)
- Panels are collapsible via the two ◀ buttons in the toolbar

### Splitter Components

Two `LithiumSplitter` instances manage the panel dividers:

```javascript
// Left splitter (between left and middle panels)
this.leftSplitter = new LithiumSplitter({
  element: this.elements.splitterLeft,
  leftPanel: this.elements.leftPanel,
  minWidth: 157,
  maxWidth: 1000,
  onResize: (width) => { this.leftPanelWidth = width; },
  onResizeEnd: () => { this.lookupTable?.table?.redraw?.(); },
});

// Right splitter (between middle and right panels)
this.rightSplitter = new LithiumSplitter({
  element: this.elements.splitterRight,
  leftPanel: this.elements.middlePanel,
  minWidth: 157,
  maxWidth: 1000,
  onResize: (width) => { this.middlePanelWidth = width; },
  onResizeEnd: () => { /* redraw tables */ },
});
```

---

## HTML Template Structure

The HTML uses `data-panel` attributes required by `LithiumSplitter`:

```html
<div class="style-manager-container">
  <!-- Left Panel -->
  <div data-panel="left" id="style-left-panel">
    <div class="style-table-container lithium-table-container" id="style-lookup-table-container"></div>
    <div class="style-navigator-container lithium-navigator-container" id="style-lookup-navigator"></div>
  </div>

  <div class="lithium-splitter" id="style-splitter-left"></div>

  <!-- Middle Panel -->
  <div data-panel="left" id="style-middle-panel">
    <div class="style-table-container lithium-table-container" id="style-sections-table-container"></div>
    <div class="style-navigator-container lithium-navigator-container" id="style-sections-navigator"></div>
  </div>

  <div class="lithium-splitter" id="style-splitter-right"></div>

  <!-- Right Panel -->
  <div data-panel="right" id="style-right-panel">
    <!-- Toolbar header -->
    <div class="style-tabs-header">...</div>
    <!-- Editor content -->
    <div class="style-editor-content">...</div>
  </div>
</div>
```

**Important:** Both middle and left panels use `data-panel="left"` because `LithiumSplitter` looks for the element immediately before it with this attribute. The splitter finds the first `[data-panel="left"]` sibling.

---

## Hardcoded Sections Data

The sections table is populated with hardcoded data — no API call is needed. The table is initialized with a `tablePath` for column configuration lookup, but data is loaded directly:

```javascript
loadSectionsData() {
  if (!this.sectionsTable?.table) return;

  this.sectionsTable.currentData = SECTIONS_DATA;

  const visibleColumns = [
    { title: 'Section', field: 'sectionName', resizable: true, headerSort: true, widthGrow: 1 },
    { title: 'Delta', field: 'delta', width: 60, hozAlign: 'right', resizable: false, headerSort: true },
  ];

  this.sectionsTable.table.blockRedraw?.();
  try {
    this.sectionsTable.table.setData(SECTIONS_DATA);
    this.sectionsTable.discoverColumns(SECTIONS_DATA);
    visibleColumns.forEach(col => {
      const existingCol = this.sectionsTable.table.getColumn(col.field);
      if (!existingCol) {
        this.sectionsTable.table.addColumn(col, false);
      } else {
        existingCol.updateDefinition(col);
      }
    });
  } finally {
    this.sectionsTable.table.restoreRedraw?.();
  }
}
```

The `Delta` column is intended to show the number of style changes from the base theme (0 = unmodified).

---

## Section Mockups

Each section that can be visually edited has a mockup defined in `SECTION_MOCKUPS`. Mockups are HTML templates with `data-target` attributes marking interactive elements.

### Login Mockup (Section 4)

The login mockup mirrors the actual login page layout from `login.html`:

- **Header** with gradient background, logo area ("Lithium"), and version box
- **Username input** with clear button
- **Password input** with toggle visibility button
- **Error message** area (hidden by default)
- **Button group** with language, theme, login, logs, and help buttons

```javascript
SECTION_MOCKUPS[4] = {
  name: 'Login',
  html: `<div class="login-mockup">
    <div class="login-card" data-target="login-panel" ...>
      <div class="login-header" data-target="login-header" ...>
        <!-- Logo + Version -->
      </div>
      <form class="login-form" ...>
        <!-- Username, Password, Error, Button Group -->
      </form>
    </div>
  </div>`,
  targets: [
    { id: 'login-panel', selector: '.login-card', label: 'Login Card Background' },
    { id: 'login-header', selector: '.login-header', label: 'Header' },
    { id: 'login-logo-area', selector: '.login-header-primary', label: 'Logo Area' },
    { id: 'login-version', selector: '.login-header-version', label: 'Version Box' },
    { id: 'login-username', selector: '.login-username', label: 'Username Input' },
    { id: 'login-password', selector: '.login-password', label: 'Password Input' },
    { id: 'login-submit', selector: '.login-btn-primary', label: 'Login Button' },
  ],
};
```

### Target Indicators

Each styleable element gets a numbered circle positioned next to it:

```css
.style-target-circle {
  position: absolute;
  width: 24px;
  height: 24px;
  border-radius: 50%;
  background: var(--accent-primary);
  border: 2px solid var(--accent-hover);
  cursor: pointer;
  /* ... */
}
.style-target-circle::after {
  content: '\2192';  /* right arrow */
  position: absolute;
  right: -18px;
  /* ... */
}
```

Clicking a target:
1. Highlights the circle (active state)
2. Shows the editor panel below the mockup
3. Displays property editor with placeholder CSS properties (to be implemented)

---

## Data Loading Patterns

### Lookup 41 Table (API-driven)

Uses QueryRef 26 to fetch lookup values, following the same pattern as Version History Manager:

```javascript
// Version History Manager uses:
await authQuery(this.app.api, 26, { INTEGER: { LOOKUPID: 44 } });
await authQuery(this.app.api, 26, { INTEGER: { LOOKUPID: 45 } });

// Style Manager uses:
await authQuery(this.app.api, 26, { INTEGER: { LOOKUPID: 41 } });
```

### Sections Table (Hardcoded)

Data is loaded using the `loadStaticData()` method, which encapsulates the blockRedraw/setData/discoverColumns pattern:

```javascript
this.sectionsTable.loadStaticData(SECTIONS_DATA);
```

This method automatically:
- Sets the table data
- Discovers and adds hidden columns from the data
- Auto-selects the first row
- Updates navigator button state

---

## Query References

### QueryRef 26 — Get Lookup

Returns all values for a given lookup ID.

**Parameters:**

```javascript
{ INTEGER: { LOOKUPID: 41 } }
```

**Response:** Array of lookup value objects

### QueryRef 060 — Tabulator Schemas

The `tablePath` options (e.g., `'style-manager/sections'`, `'style-manager/lookup-41'`) reference Tabulator schema definitions stored in Lookup 060. If no schema is found, fallback columns are used.

---

## CSS Architecture

### Layout Classes

| Class | Purpose |
|-------|---------|
| `.style-manager-container` | Flex container for entire manager |
| `#style-left-panel` (`[data-panel="left"]`) | Left panel with Lookup 41 table |
| `#style-middle-panel` (`[data-panel="left"]`) | Middle panel with Sections table |
| `#style-right-panel` (`[data-panel="right"]`) | Right editor panel |
| `.style-table-container` | Table wrapper (flex: 1, min-height: 0) |
| `.style-navigator-container` | Navigator wrapper (flex-shrink: 0) |
| `.lithium-splitter` | Resizable panel divider |
| `.style-tabs-header` | Toolbar header area |
| `.style-mode-toggle` | Section/JSON mode toggle |
| `.style-editor-content` | Editor content area |

### Table Container Styling

Table containers use standard LithiumTable classes (`lithium-table-container`, `lithium-navigator-container`) plus manager-specific overrides:

```css
.style-table-container .tabulator { border: none; height: 100% !important; }
.style-table-container .tabulator-tableholder { background: var(--bg-secondary); }
.style-table-container .tabulator-header { background: var(--bg-tertiary); }
.style-table-container .tabulator-row { background: var(--bg-secondary); cursor: pointer; }
.style-table-container .tabulator-row:hover { background: var(--bg-tertiary); }
.style-table-container .tabulator-row.tabulator-selected { background: var(--accent-focus); }
```

### Sort Icons CSS (Automatic)

LithiumTable automatically injects sort icon styles for custom `cssPrefix` values. The Style Manager uses `cssPrefix: 'style-lookup'` and `cssPrefix: 'style-sections'`, and LithiumTable will automatically inject sort icon styles for both prefixes.

**Navigator styles are NOT injected** — all tables share the same Navigator styling via `.lithium-*` base classes in `lithium-table.css`. This enables theme-level overrides through CSS variables.

**You do not need to add sort icon CSS or Navigator CSS to `style-manager.css`.**

See [LITHIUM-TAB.md](LITHIUM-TAB.md#sort-icons-css-automatic) for details on how this works.

### Collapse Icon Rotation

When panels are collapsed, the corresponding ◀ icon rotates 180°:

```css
#style-left-panel.collapsed ~ ... ~ #style-right-panel .style-collapse-left-btn ... {
  transform: rotate(180deg);
}
```

---

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl/Cmd + Z` | Undo |
| `Ctrl/Cmd + Shift + Z` | Redo |
| `Ctrl/Cmd + P` | Pretty print |
| `Ctrl/Cmd + 1` | Switch to Section mode |
| `Ctrl/Cmd + 2` | Switch to JSON mode |

---

## Footer Controls

The footer includes Print/Email/Export buttons using `setupManagerFooterIcons()`:

- **Print** — Print table data
- **Email** — Open mailto link with table data
- **Export** — PDF, CSV, TXT, XLS download options
- **Datasource selector** — Choose which table to export (Lookup 41 view/data, Sections view/data)
- **Save** — Enabled (green) only when a dirty record is detected; commits changes
- **Cancel** — Enabled (red) only when a dirty record is detected; reverts changes

Save/Cancel buttons are managed by `ManagerEditHelper`. See [LITHIUM-TAB.md](LITHIUM-TAB.md#footer-savecancel-buttons-and-manageredithelper) for the full pattern.

---

## Related Documentation

- [LITHIUM-MGR-LOOKUPS.md](LITHIUM-MGR-LOOKUPS.md) — Lookups Manager (dual-table layout pattern)
- [LITHIUM-MGR-QUERY.md](LITHIUM-MGR-QUERY.md) — Query Manager (toolbar pattern)
- [LITHIUM-TAB.md](LITHIUM-TAB.md) — LithiumTable component documentation
- [LITHIUM-MGR-LOGIN.md](LITHIUM-MGR-LOGIN.md) — Login Manager (source for login mockup)

---

## Implementation History

**Key Design Decisions:**

1. **Three-panel layout** — Follows the Lookups Manager pattern with `data-panel` attributes for LithiumSplitter compatibility
2. **QueryRef 26 for Lookup 41** — Same QueryRef pattern as Version History Manager (which uses Lookups 44/45)
3. **Hardcoded sections data** — No API call needed; data loaded via `loadStaticData()` (wraps blockRedraw/setData/discoverColumns)
4. **Two visible columns on sections** — `sectionName` (Section) and `delta` (Delta, integer, 60px wide, right-aligned)
5. **LithiumSplitter** — Uses the standard splitter component instead of custom drag handlers
6. **Login mockup mirrors actual login** — Header with gradient, logo, version box, input fields, button group
7. **Section mockups use `data-target` attributes** — For interactive target circle positioning
8. **`data-panel="left"` on both left and middle panels** — Required by LithiumSplitter which finds the first preceding sibling with this attribute
9. **Section/JSON toolbar follows Query Manager pattern** — Two collapse buttons on left, mode toggle, placeholder, then Undo/Redo/Font/Prettify on right
10. **Manager footer** — Uses `setupManagerFooterIcons()` for Print/Email/Export controls
11. **Shared font popup** — Uses `createFontPopup()` from `manager-ui.js` instead of inline implementation
12. **Navigator styles shared** — All Navigator buttons use `.lithium-*` base classes; themeable via CSS variables on `.lithium-nav-btn`
