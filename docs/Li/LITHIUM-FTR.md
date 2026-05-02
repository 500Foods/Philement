# Lithium Editor Footer (LithiumEditorFooter)

A toolbar positioned below CodeMirror instances displaying cursor position, error count, error message, and toggles for word wrap/bracket matching.

---

## Overview

`LithiumEditorFooter` provides consistent footer UI for all CodeMirror editors in Lithium managers. It integrates with CodeMirror compartments to toggle word wrap and bracket matching, and displays real-time cursor position and error information.

**Key Features:**
- Cursor position display (Row/Col)
- Error count and message display
- Word wrap toggle button
- Bracket matching toggle button
- Automatic text collapsing (shares `lithium-toolbar` styling with `LithiumToolbar`)
- Listens to CodeMirror selection changes for cursor updates
- Positioned as a **sibling of the toolbar** (same parent as `LithiumToolbar`)

---

## Architecture

**Files:**
- `src/core/editor-footer.js` — Main `LithiumEditorFooter` class
- `src/core/editor-footer.css` — No separate CSS file; uses `lithium-toolbar` classes from `src/core/lithium-toolbar.css`

**Exports:**
- Re-exported from `src/core/manager-ui.js` for convenience:
  ```javascript
  // Import from manager-ui.js (recommended)
  import { LithiumEditorFooter } from '../../core/manager-ui.js';
  
  // Or import directly from source
  import { LithiumEditorFooter } from '../../core/editor-footer.js';
  ```

---

## Usage

### Basic Integration

Follow the pattern used in the Style Manager (`src/managers/style-manager/style-manager.js`):

1. **Create compartments** for word wrap and bracket matching:
   ```javascript
   import { createWordWrapCompartment, createBracketMatchCompartment } from '../../core/codemirror-setup.js';
   
   // In your init method:
   this.cmWordWrapCompartment = createWordWrapCompartment();
   this.cmBracketMatchCompartment = createBracketMatchCompartment();
   ```

2. **Pass compartments to `buildEditorExtensions`** when initializing CodeMirror:
   ```javascript
   import { buildEditorExtensions } from '../../core/codemirror-setup.js';
   
   const extensions = buildEditorExtensions({
     language: 'json',
     readOnlyCompartment: this._readOnlyCompartment,
     wordWrapCompartment: this.cmWordWrapCompartment,
     bracketMatchCompartment: this.cmBracketMatchCompartment,
     wordWrap: false,    // Initial state
     bracketMatch: true,  // Initial state
     // ... other options
   });
   ```

3. **Create footer container and initialize footer** — append as sibling of the toolbar (same parent):
   ```javascript
   import { LithiumEditorFooter } from '../../core/manager-ui.js';
   
   // Create footer container - append to manager container
   // This makes the footer a sibling of the toolbar
   const footerEl = document.createElement('div');
   this.manager.container.appendChild(footerEl);
   
   // Initialize footer
   this.editorFooter = new LithiumEditorFooter({
     container: footerEl,
     editorView: myEditorView,
     wordWrapCompartment: this.cmWordWrapCompartment,
     bracketMatchCompartment: this.cmBracketMatchCompartment,
     initialWordWrap: false,
     initialBracketMatch: true,
   });
   this.editorFooter.init();
   ```

**DOM Structure:**
```
manager-container (this.manager.container)
├── toolbar (LithiumToolbar)        ← existing toolbar
├── tab-content                      ← tab content container
│   └── editor-container           ← CodeMirror container
│       └── cm-editor-dom
└── footer-container                ← footer (sibling of toolbar)
    └── .lithium-toolbar.lithium-editor-footer
```

**Note:** When using tabs, the footer should be shown/hidden based on the active tab. See the Integration Examples below for tab visibility handling.

### API Reference

#### Constructor Options

| Option | Type | Required | Default | Description |
|--------|------|----------|---------|-------------|
| `container` | HTMLElement | ✅ | — | Footer container element |
| `editorView` | EditorView | ✅ | — | CodeMirror EditorView instance |
| `wordWrapCompartment` | Compartment | ❌ | `null` | Word wrap compartment |
| `bracketMatchCompartment` | Compartment | ❌ | `null` | Bracket matching compartment |
| `initialWordWrap` | boolean | ❌ | `false` | Initial word wrap state |
| `initialBracketMatch` | boolean | ❌ | `true` | Initial bracket matching state |

#### Methods

| Method | Description |
|--------|-------------|
| `init()` | Initialize the footer, create DOM elements, attach listeners |
| `updateErrorCount(count)` | Update the error count display |
| `setErrorMessage(message)` | Set or clear the error message |
| `destroy()` | Clean up listeners and references |

### Cleanup

Always destroy the footer when the editor is destroyed:
```javascript
destroy() {
  if (this.editorFooter) {
    this.editorFooter.destroy();
    this.editorFooter = null;
  }
  if (this.editor) {
    this.editor.destroy();
    this.editor = null;
  }
}
```

---

## Integration Examples

### Query Manager (SQL Editor)
```javascript
// In queries-editors.js initSqlEditor()
this._sqlWordWrapCompartment = createWordWrapCompartment();
this._sqlBracketMatchCompartment = createBracketMatchCompartment();

const extensions = buildEditorExtensions({
  language: 'sql',
  readOnlyCompartment: this._sqlReadOnlyCompartment,
  wordWrapCompartment: this._sqlWordWrapCompartment,
  bracketMatchCompartment: this._sqlBracketMatchCompartment,
  wordWrap: false,
  bracketMatch: true,
  // ... other options
});

this.sqlEditor = new EditorView({ state: startState, parent: container });

// Create footer container - append to manager container
// This makes the footer a sibling of the toolbar
const sqlFooterEl = document.createElement('div');
sqlFooterEl.style.display = 'none'; // Hidden by default, shown when tab is active
this.manager.container.appendChild(sqlFooterEl);
this.sqlEditorFooter = new LithiumEditorFooter({
  container: sqlFooterEl,
  editorView: this.sqlEditor,
  wordWrapCompartment: this._sqlWordWrapCompartment,
  bracketMatchCompartment: this._sqlBracketMatchCompartment,
  initialWordWrap: false,
  initialBracketMatch: true,
});
this.sqlEditorFooter.init();
```

**Tab Visibility Handling (in queries.js switchTab method):**
```javascript
async switchTab(tabId) {
  // ... existing code ...

  // Show/hide editor footers based on active tab
  const footers = {
    sql: this.editorManager?.sqlEditorFooter,
    summary: this.editorManager?.summaryEditorFooter,
    collection: this.editorManager?.collectionEditorFooter,
  };
  Object.values(footers).forEach(footer => {
    if (footer?.container) footer.container.style.display = 'none';
  });
  if (footers[tabId]?.container) {
    footers[tabId].container.style.display = '';
  }

  // ... rest of switchTab logic
}
```

**DOM Structure:**
```
queries-right-panel (this.manager.container)
├── queries-toolbar (LithiumToolbar) ← toolbar
├── queries-tabs-content
│   └── queries-tab-sql
│       └── queries-sql-editor (CM container)
└── div (appended)                  ← footer (sibling of toolbar)
    └── .lithium-toolbar.lithium-editor-footer
```

### User Profile Manager (Collection Tab)
```javascript
// In profile-manager-collection.js CollectionTabHandler
this.wordWrapCompartment = createWordWrapCompartment();
this.bracketMatchCompartment = createBracketMatchCompartment();

const extensions = buildEditorExtensions({
  language: 'json',
  readOnlyCompartment: this.readOnlyCompartment,
  wordWrapCompartment: this.wordWrapCompartment,
  bracketMatchCompartment: this.bracketMatchCompartment,
  wordWrap: false,
  bracketMatch: true,
  // ... other options
});

this.editor = new EditorView({ state: startState, parent: this.container });

// Create footer container - append to manager container (passed as `parent` option)
// This makes the footer a sibling of the toolbar
const footerEl = document.createElement('div');
footerEl.style.display = 'none'; // Hidden by default, shown when tab is active
this.parent.appendChild(footerEl);
this.footer = new LithiumEditorFooter({
  container: footerEl,
  editorView: this.editor,
  wordWrapCompartment: this.wordWrapCompartment,
  bracketMatchCompartment: this.bracketMatchCompartment,
  initialWordWrap: false,
  initialBracketMatch: true,
});
this.footer.init();
```

**Tab Visibility Handling:**
The User Profile Manager should show/hide the Collection tab footer when switching to/from the Collection tab. This can be done by:
1. Adding a `showFooter()` and `hideFooter()` method to `CollectionTabHandler`
2. Calling these methods when the Collection tab is activated/deactivated

**DOM Structure:**
```
profile-manager-container (this.parent / this.manager.container)
├── profile-toolbar (LithiumToolbar) ← toolbar
├── tab-content
│   └── collection-editor (CM container)
└── div (appended)                 ← footer (sibling of toolbar)
    └── .lithium-toolbar.lithium-editor-footer
```

---

## Styling

The footer uses the `lithium-toolbar` and `lithium-editor-footer` CSS classes. It shares the same 24px height and outline styling as `LithiumToolbar`.

**CSS Classes:**
| Class | Purpose |
|-------|---------|
| `.lithium-toolbar` | Base toolbar styling (shared with `LithiumToolbar`) |
| `.lithium-editor-footer` | Footer-specific modifier (currently minimal) |
| `.lithium-editor-cursor` | Cursor position display (left section) |
| `.lithium-editor-errors` | Error count display (left section) |
| `.lithium-editor-error-msg` | Error message display (flexible placeholder) |
| `.lithium-toolbar-btn` | Toggle buttons (right section) |

---

## Status

✅ Implemented and integrated into:
- Style Manager (CSS editor)
- Query Manager (SQL, Summary, Collection editors)
- Lookups Manager (JSON editor)
- User Profile Manager (Collection tab)

**Documentation:** This file  
**Implementation:** `src/core/editor-footer.js`

---

## Related Documentation

- [LITHIUM-BAR.md](LITHIUM-BAR.md) — Toolbar system (shared styling)
- [LITHIUM-MGR.md](LITHIUM-MGR.md) — Manager system overview
- [LITHIUM-LIB.md](LITHIUM-LIB.md) — CodeMirror setup and utilities
