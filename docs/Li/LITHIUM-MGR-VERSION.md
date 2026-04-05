# Lithium Version Manager

This document describes the Version Manager, a read-only interface for viewing version history and release notes.

---

## Overview

The Version Manager (Manager ID: 11) provides a two-panel interface for browsing software version history:

- **Left panel:** Combined version list from server and client lookup tables
- **Right panel:** Markdown preview of the selected version's summary and release notes

**Key Features:**

- Read-only version history browser
- Combined data from Lookup 41 (Server) and 42 (Client)
- Markdown rendering for release notes
- Collapsible left panel
- Persistent selection and panel state

**Location:** `src/managers/version-history/`

**Key Files:**

- `version-history.js` — Main manager class
- `version-history.css` — Styling
- `version-history.html` — HTML template

---

## UI Layout

```
┌─────────────────┬──────────────────────────────────────────┐
│                 │                                          │
│  Version List   │         Version Preview                  │
│  (Tabulator)    │                                          │
│                 │  ┌────────────────────────────────────┐  │
│  ┌───────────┐  │  │ Version: 1.2.3                     │  │
│  │ Version 1 │  │  │ Released: 2026-01-15               │  │
│  │ Version 2 │  │  │ Focus: Bug fixes                   │  │
│  │ Version 3 │  │  └────────────────────────────────────┘  │
│  │ ...       │  │                                          │
│  └───────────┘  │  ┌────────────────────────────────────┐  │
│                 │  │                                    │  │
│  [Navigator]    │  │   Release notes in Markdown...     │  │
│                 │  │                                    │  │
│                 │  └────────────────────────────────────┘  │
│                 │                                          │
└─────────────────┴──────────────────────────────────────────┘
      ▲
      │ (resizable splitter)
```

### Left Panel: Version List

A read-only LithiumTable showing all versions.

**Data Sources:**

- **Lookup 44:** Server versions
- **Lookup 45:** Client versions

**Columns:**

- `key_idx` — Version identifier
- `value_txt` — Version name/status
- `source_label` — Server or Client
- `Focus` — Release focus (from collection JSON)
- `Released` — Release date (from collection JSON)
- `collection` — Collection summary

### Right Panel: Version Preview

Renders the selected version's summary as Markdown.

**Features:**

- HTML detection vs Markdown parsing
- Version metadata header
- Fallback to plain text if Markdown fails

### Collapsible Left Panel

The left panel can be collapsed to maximize preview space.

- **Collapse button:** Arrow in right panel header
- **Animation:** Smooth CSS transition
- **Persistence:** State saved across sessions

---

## Architecture

### Class Structure

```javascript
export default class VersionHistoryManager {
  constructor(app, container) {
    this.versionsTable = null;           // LithiumTable instance
    this.editHelper = new ManagerEditHelper({ name: 'VersionHistory' });
    // ... panel state, splitter, elements ...
  }

  async init() {
    await this.loadDependencies();       // Load marked.js
    await this.render();
    this.setupEventListeners();
    await this.initVersionsTable();
    this.setupFooter();
    this.restorePanelState();
  }

  // Data loading
  async loadVersionList() { /* QueryRef 26 for lookups 44 + 45 */ }
  async loadVersionDetails(keyIdx, clientserver) { /* QueryRef 45 */ }

  // Selection handling
  async handleVersionSelected(rowData) { /* Load details + update preview */ }
  updatePreview() { /* Render Markdown */ }

  // Lifecycle
  cleanup() { this.editHelper?.destroy(); /* ... */ }
}
```

### Data Loading Flow

1. **Load Dependencies:** Import `marked` for Markdown rendering
2. **Load Version List:** QueryRef 26 for Lookup 44 (Server) + Lookup 45 (Client)
3. **Process Rows:** Extract JSON fields from collection, tag with source
4. **Combine Data:** Merge server and client rows into single table
5. **Row Selection:** QueryRef 45 for full summary + collection details
6. **Render Preview:** Parse Markdown and display

### QueryRefs

| Ref | Purpose | Parameters |
|-----|---------|------------|
| 26 | Get Lookup Values | `LOOKUPID: 44` or `45` |
| 45 | Get Version Details | `CLIENTSERVER`, `KEYIDX` |

---

## Footer Controls

The footer includes standard export options:

- **Print** — Print version list or data
- **Email** — Email version data as tab-separated text
- **Export** — PDF/CSV/TXT/XLS formats
- **View Mode** — Filtered view vs full data

---

## CSS Architecture

### Layout Classes

| Class | Purpose |
|-------|---------|
| `.version-manager-container` | Flex container |
| `.version-left-panel` | Version list panel |
| `.version-right-panel` | Preview panel |
| `.version-splitter` | Resizable divider |
| `.version-preview-content` | Markdown render area |
| `.version-preview-header` | Version metadata |
| `.version-preview-placeholder` | No selection message |

### Key Styles

- **Panel widths:** Default 350px left, flex right
- **Splitter:** LithiumSplitter with min/max constraints
- **Preview:** Scrollable with syntax highlighting
- **Transitions:** 350ms ease for collapse/expand

---

## Related Documentation

- [LITHIUM-MGR.md](LITHIUM-MGR.md) — Manager system overview
- [LITHIUM-TAB.md](LITHIUM-TAB.md) — LithiumTable component
- [LITHIUM-LUT.md](LITHIUM-LUT.md) — Lookup Tables system

---

## Implementation Notes

- **Read-only:** No edit mode or save operations
- **Markdown:** Uses `marked` library for rendering
- **Collection JSON:** Version metadata stored as JSON in lookup collection field
- **Source Tagging:** Rows tagged with "Server" or "Client" for filtering</content>
<parameter name="filePath">docs/Li/LITHIUM-MGR-VERSION.md