# User Profile Manager

This document describes the User Profile Manager — a utility manager for managing user preferences, profile settings, and account information.

---

## Overview

| Aspect | Details |
|-------|--------|
| **Type** | Utility Manager |
| **Key** | `user-profile` |
| **Location** | `src/managers/profile-manager/` |
| **Manager ID** | 2 (Utility) |
| **Closed Manager** | Query Manager (default) |

---

## Features

The User Profile Manager provides:

- **Account Information** — Username, email, roles, database from JWT claims
- **Language Selector** — Interface language (en-US, en-GB, es, fr, de, etc.)
- **Date Format** — MM/DD/YYYY, DD/MM/YYYY, YYYY-MM-DD, etc.
- **Time Format** — 12-hour (AM/PM) or 24-hour
- **Number Format** — US, UK, European, French decimal/thousands separators
- **Default Database** — Default database for new connections
- **Active Theme** — Visual theme quick-apply from Style Manager themes
- **Real-time Preview** — Live preview of date/time/number formats

---

## Architecture

### File Structure

```
src/managers/profile-manager/
├── profile-manager.js       # Main manager class
├── profile-manager.html   # HTML template
└── profile-manager.css    # Styles
```

### Configuration

- **Table Definition:** `config/tabulator/profile-manager/user-options.json`
- **Lookup Source:** Lookup 60 (App UI Lists) via QueryRef 26
- **Storage:** `lithium_preferences` in localStorage

---

## Layout

The manager uses a dual-panel layout:

| Panel | Content | Width |
|-------|---------|-------|
| Left | LithiumTable for user options | 200-600px (default 314px) |
| Right | Settings tabs (Settings, Account) | Flexible |

### Panels Configuration

- **Splitter:** LithiumSplitter component
- **Collapse:** Left panel can collapse via toolbar button
- **State Persistence:** PanelStateManager for width/collapsed state

---

## LithiumTable Integration

### Table Configuration

```javascript
this.optionsTable = new LithiumTable({
  container: this.elements.tableContainer,
  navigatorContainer: this.elements.navigatorContainer,
  tablePath: 'profile-manager/user-options',
  lookupKeyIdx: 8,  // Direct Lookup 59 key_idx for unambiguous schema loading
  cssPrefix: 'profile-options',
  storageKey: 'profile_options_table',
  app: this.app,
  readonly: true,
  panel: this.elements.leftPanel,
  panelStateManager: this.leftPanelState,
  onRowSelected: (rowData) => this.handleOptionSelected(rowData),
});
```

### Column Definition

Defined in `config/tabulator/profile-manager/user-options.json`:

| Field | Display | Type | Visible |
|-------|---------|------|---------|
| `key_idx` | ID# | index | No |
| `section` | Section | string | Yes |
| `icon` | Icon | string | Yes |
| `label` | Option | string | Yes |
| `status_a1` | Status | lookup (a1) | No |

### Data Source

The table loads data from Lookup 60 (App UI Lists):

```javascript
const rows = await authQuery(this.app.api, 26, { INTEGER: { LOOKUPID: 60 } });
```

Data format from Lookup (array of `[section, icon, label]`):

```javascript
this.userOptions = sections.map((section, index) => ({
  key_idx: index + 1,
  section: section[0] || '',   // e.g., "General", "Security", "Formatting"
  icon: section[1] || '',       // e.g., "<fa fa-user></fa>"
  label: section[2] || '',     // e.g., "Account"
  status_a1: 1,
}));
```

Falls back to default options if lookup data unavailable:

| Section | Icon | Label |
|---------|------|-------|
| General | `<fa fa-user></fa>` | Account and Names |
| Security | `<fa fa-key></fa>` | Authentication |
| Formatting | `<fa fa-globe></fa>` | Language |
| Formatting | `<fa fa-calendar></fa>` | Date/Time Formats |
| Formatting | `<fa fa-00></fa>` | Number Formats |
| Application | `<fa fa-flag-pennant></fa>` | Startup |
| Application | `<fa fa-bell></fa>` | Notifications |

---

## Styling Reference

### CSS Classes

| Class | Element |
|-------|---------|
| `#profile-manager-page` | Main page container |
| `.profile-manager-container` | Panels wrapper |
| `#profile-left-panel` | Left panel with table |
| `#profile-right-panel` | Right panel with settings |
| `#profile-table-container` | Table container |
| `#profile-navigator` | Navigator container |
| `#profile-tab-toolbar` | Tab toolbar |
| `.tab-panel` | Tab content panels |

### Key Styles

The manager uses nav-block styling for toolbars, matching LithiumTable Navigator:

- 24px height
- 1px gap between buttons
- Outline styling on container

---

## Lookup 59 Integration

The table definition is loaded from Lookup 59 (Tabulator Schemas) via the `loadTableDef()` function.

### Path Mapping

The `profile-manager/user-options` table path must be mapped in `lithium-table.js`:

```javascript
const tableToKeyIdx = {
  // ... existing mappings
  'profile-manager/user-options': 8,  // Profile Manager user options
};
```

If this mapping is missing, the table will have no columns.

---

## Event Handling

### Events Fired

| Event | When |
|-------|------|
| `LOCALE_CHANGED` | Language preference changed |
| `theme:changed` | Theme preference changed |

### Local Storage Keys

| Key | Content |
|-----|---------|
| `lithium_preferences` | User preferences JSON |
| `lithium_themes` | Cached theme list |
| `activeThemeId` | Current theme ID |

---

## Template Loading

The manager loads its HTML template dynamically:

```javascript
const response = await fetch('/src/managers/profile-manager/profile-manager.html');
this.container.innerHTML = await response.text();
```

---

## Related Documentation

- [LITHIUM-MGR.md](LITHIUM-MGR.md) — Manager system overview
- [LITHIUM-TAB.md](LITHIUM-TAB.md) — LithiumTable component
- [LITHIUM-LUT.md](LITHIUM-LUT.md) — Lookup tables

---

Last updated: April 2026