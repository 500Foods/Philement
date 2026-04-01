# Lithium JavaScript Libraries

This document describes each JavaScript library used in the Lithium project.

---

## Overview

Lithium uses a minimal set of third-party libraries, preferring to build functionality in-house where practical.

| Category | Libraries |
|----------|-----------|
| Build | Vite, Vitest |
| Tables | Tabulator |
| Code Editor | CodeMirror 6 |
| JSON Editor | CodeMirror 6 (JSON mode) |
| Tooltips | Floating UI |
| Sanitization | DOMPurify |
| Markdown | marked |
| SQL Formatting | sql-formatter |
| Icons | Font Awesome |
| Fonts | Vanadium Sans/Mono (self-hosted) |

---

## NPM-Managed Libraries

These are installed via npm and bundled by Vite.

### Tabulator

**Purpose:** Interactive data tables with sorting, filtering, pagination

**Package:** `tabulator-tables`

**Import:** Dynamic (lazy-loaded)

```javascript
const tabulatorModule = await import('tabulator-tables');
const Tabulator = tabulatorModule.default;
```

**Usage in Lithium:** Style Manager theme list view

**Key features used:**

- Column sorting
- Data formatting
- Row selection

---

### CodeMirror 6

**Purpose:** Code editor with syntax highlighting

**Packages:**

- `@codemirror/view`
- `@codemirror/state`
- `@codemirror/lang-css`
- `@codemirror/theme-one-dark`

**Import:** Dynamic (lazy-loaded)

```javascript
const { EditorView } = await import('@codemirror/view');
const { css } = await import('@codemirror/lang-css');
```

**Usage in Lithium:**

- Style Manager CSS editor
- Session Log viewer
- Login panel System Logs

**Key features used:**

- Read-only mode (for log viewers)
- CSS syntax highlighting
- One Dark theme
- Line numbers

---

### JSON Editing (CodeMirror JSON mode)

**Purpose:** JSON editor for viewing and editing structured data

JSON editing throughout Lithium uses **CodeMirror 6** with `@codemirror/lang-json`. This replaced `vanilla-jsoneditor` (which itself had replaced the older `jsoneditor` library). The consolidation means one editor library (CodeMirror) handles SQL, Markdown, JSON, and CSS across all managers.

**Usage in Lithium:**

- Query Manager "Collection" tab — JSON editor with fold/unfold
- Lookups Manager "JSON" tab — JSON editor for lookup values
- Style Manager "JSON" mode — JSON theme data editor

**Key features via `codemirror-setup.js`:**

- Syntax highlighting, bracket matching
- Fold/unfold support for nested structures
- Read-only compartment (toggled on edit mode enter/exit)
- `onUpdate` callback for dirty detection via `editHelper.checkDirtyState()`
- Undo/redo via CodeMirror history

**Migration history:** `jsoneditor` (v10.x) → `vanilla-jsoneditor` (v2.x) → CodeMirror JSON mode (current). The `init/jsoneditor-init.js` file remains as dead code and can be removed.

---

### Floating UI

**Purpose:** Intelligent tooltip positioning, popover placement, and floating element computation

**Package:** `@floating-ui/dom`

**Import:** Static (core module used by tooltip system)

```javascript
import { computePosition, flip, shift, offset, arrow, autoUpdate } from '@floating-ui/dom';
```

**Usage in Lithium:**

- Tooltip system (`src/core/tooltip.js`) — all tooltip positioning
- Provides auto-flip when tooltips approach viewport edges
- Arrow positioning relative to trigger element

**Key features used:**

- `computePosition` — one-shot positioning calculation
- `autoUpdate` — reactive repositioning on scroll/resize (only while visible)
- `flip` — automatically flips placement when near viewport edges
- `shift` — keeps tooltips within viewport horizontally
- `arrow` — calculates arrow offset relative to tooltip and trigger

**Documentation:** See [LITHIUM-TIP.md](LITHIUM-TIP.md) for the full tooltip system documentation.

---

### DOMPurify

**Purpose:** Sanitize HTML and CSS to prevent XSS

**Package:** `dompurify`

**Import:** Dynamic (lazy-loaded)

```javascript
const dompurifyModule = await import('dompurify');
const DOMPurify = dompurifyModule.default;
```

**Usage in Lithium:** Style Manager theme application

**Key features used:**

- HTML sanitization
- Custom allowed tags/attributes
- Style tag sanitization

```javascript
const clean = DOMPurify.sanitize(dirtyHtml, {
  ALLOWED_TAGS: ['b', 'i', 'em', 'strong'],
  ALLOWED_ATTR: ['class'],
});
```

---

### Vitest

**Purpose:** Unit testing framework

**Package:** `vitest`

**Configuration:** `vitest.config.js`

**Usage:**

```bash
npm test
```

---

### happy-dom

**Purpose:** DOM implementation for testing

**Package:** `happy-dom`

**Why:** Better ESM support than jsdom

---

## CDN Libraries

These are loaded via CDN, not npm.

### Font Awesome

**Purpose:** Icon library

**Loading:** Kit-based (more reliable than CDN with SRI)

**In HTML:**

```html
<script src="https://kit.fontawesome.com/YOUR_KIT_CODE.js" crossorigin="anonymous"></script>
```

**Lithium Custom System:** Uses `<fa>` tag instead of `<i>`:

```html
<!-- Instead of -->
<i class="fas fa-user"></i>

<!-- Use -->
<fa fa-user></fa>
```

**Configuration in `config/lithium.json`:**

```json
{
  "icons": {
    "set": "duotone",
    "weight": "thin",
    "prefix": "fad",
    "fallback": "solid"
  }
}
```

**JavaScript API:**

```javascript
import { setIcon, createIcon, processIcons } from './core/icons.js';

// Set icon on existing element
setIcon(element, 'user', { set: 'solid', classes: ['fa-fw'] });

// Create new icon
const icon = createIcon('cog', { set: 'duotone', weight: 'thin' });

// Process dynamically added <fa> elements
processIcons(container);
```

---

## Self-Hosted Fonts

### Vanadium Sans & Vanadium Mono

**Purpose:** Custom brand fonts

**Location:** `public/assets/fonts/`

**Format:** WOFF2 (with fallback)

**Loading:** CSS `@font-face` in `base.css`

```css
@font-face {
  font-family: 'Vanadium Sans';
  src: local('Vanadium Sans'),
       url('/assets/fonts/vanadium-sans.woff2') format('woff2');
  font-display: swap;
}
```

---

## Libraries Considered but Not Used

| Library | Reason for Exclusion |
|---------|---------------------|
| React/Vue | Prefer vanilla JS for simplicity |
| Bootstrap | Custom CSS needed anyway |
| Highlight.js | CodeMirror handles this |
| jsoneditor | Replaced by vanilla-jsoneditor, then by CodeMirror JSON mode |
| vanilla-jsoneditor | Replaced by CodeMirror JSON mode (consolidation — one editor library for all languages) |
| Luxon | Use native Intl.* APIs |
| SheetJS | Not needed yet |
| Split.js | Not needed yet |

---

## Dynamic Import Guidelines

Heavy dependencies should be **dynamically imported** to enable code splitting:

```javascript
// Good: Dynamic import for lazy loading
const tabulatorModule = await import('tabulator-tables');

// Bad: Static import for heavy library
import Tabulator from 'tabulator-tables';
```

Core modules (`jwt.js`, `config.js`, etc.) should always be **statically imported**:

```javascript
// Good: Static import for core modules
import { storeJWT, retrieveJWT } from './core/jwt.js';

// Only dynamic import for managers and heavy libraries
const { LoginManager } = await import('./managers/login/login.js');
```

---

## Import Rules Summary

| Module Type | Import Type | Examples |
|-------------|-------------|----------|
| Core modules | Static | `event-bus.js`, `jwt.js`, `config.js` |
| Managers | Dynamic | `login.js`, `main.js`, `style-manager.js` |
| Heavy libraries | Dynamic | Tabulator, CodeMirror, DOMPurify, marked, sql-formatter |

---

## Related Documentation

- Manager system: [LITHIUM-MGR.md](LITHIUM-MGR.md)
- Troubleshooting: [LITHIUM-FAQ.md](LITHIUM-FAQ.md)
- Deployment: [LITHIUM-WEB.md](LITHIUM-WEB.md)
