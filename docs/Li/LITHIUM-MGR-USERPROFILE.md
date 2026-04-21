# User Profile Manager

This document describes the User Profile Manager — a utility manager for managing user preferences, profile settings, and account information. The manager uses a LithiumTable on the left to navigate settings sections, with a tabbed interface on the right.

---

## Overview

| Aspect | Details |
|-------|--------|
| **Type** | Utility Manager |
| **Key** | `user-profile` |
| **Location** | `src/managers/profile-manager/` |
| **Manager ID** | 3 (matches lithium.json "003.Profile") |
| **Closed Manager** | Query Manager (default) |

---

## UI Layout

```diagram
┌─────────────────┬──────────────────────────────────────────┐
│                 │  [Section Label] [Settings] [Collection] │
│  User Options   ├──────────────────────────────────────────┤
│  (LithiumTable) │                                          │
│                 │  Settings Content                        │
│  ┌───────────┐  │  (index-based page switching)           │
│  │ General   │  │                                          │
│  │ Security  │  │  OR Collection tab →                   │
│  │ Formatting│  │  JSON CodeMirror with controls         │
│  │ Modules   │  │  (Undo Redo Fold Font Prettify)        │
│  └───────────┘  │                                          │
│                 │  [Navigator]                             │
│  [Navigator]    │                                          │
└─────────────────┴──────────────────────────────────────────┘
```

### Right Panel: Tabbed Interface

The right panel contains two top-level tabs:

1. **Settings Tab** — Displays the settings page for the currently selected row in the left table
2. **Collection Tab** — JSON editor containing all settings in one block (dev mode)

#### Index-Based Page Switching

The Settings tab displays different content based on the `index` field of the selected row:

**Internal Pages (Negative Indices):**

| Index | Section Group | Label | Icon |
|-------|--------------|-------|------|
| -1 | General | Account | `<fa fa-id-card></fa>` |
| -2 | General | Names | `<fa fa-id-badge></fa>` |
| -3 | General | Addresses | `<fa fa-address-book></fa>` |
| -4 | General | E-Mail | `<fa fa-at></fa>` |
| -5 | General | Phone | `<fa fa-phone></fa>` |
| -6 | Security | Authentication | `<fa fa-key></fa>` |
| -7 | Security | Tokens | `<fa fa-user-key></fa>` |
| -8 | Formatting | Language | `<fa fa-globe></fa>` |
| -9 | Formatting | Date Formats | `<fa fa-calendar></fa>` |
| -10 | Formatting | Number Formats | `<fa fa-00></fa>` |
| -11 | Application | Startup | `<fa fa-atom-simple></fa>` |
| -12 | Application | Notifications | `<fa fa-bell></fa>` |
| -13 | Application | Concierge | `<fa fa-bell-concierge></fa>` |
| -14 | Application | Annotations | `<fa fa-note-sticky></fa>` |
| -15 | Application | Tours | `<fa fa-signs-post></fa>` |
| -16 | Application | Training | `<fa fa-graduation-cap></fa>` |
| -17 | Security | Login History | `<fa fa-receipt></fa>` |

**Manager Pages (Positive Indices = Manager IDs):**

Manager pages use their actual Manager ID from lithium.json (e.g., Lookups Manager = 23). These are dynamically loaded from the Main Menu data and grouped under their respective menu groups.

---

## Architecture

### File Structure

```
src/managers/profile-manager/
├── profile-manager.js              # Main manager class (~720 lines)
├── profile-manager.html            # HTML template
├── profile-manager.css             # Styles
├── profile-manager-collection.js   # Collection tab handler
├── profile-manager-settings.js     # Settings tab orchestrator
├── profile-settings-service.js     # Centralized JSON storage service
└── pages/                          # Settings page handlers
    ├── page-registry.js            # Registry for loading handlers
    ├── settings-page-base.js       # Base classes for pages
    ├── page-placeholder.js         # Placeholder for unimplemented managers
    ├── page-account.js             # Account page (index -1)
    ├── page-names.js               # Names page (index -2)
    ├── page-addresses.js           # Addresses page (index -3)
    ├── page-email.js               # E-Mail page (index -4)
    ├── page-phone.js               # Phone page (index -5)
    ├── page-authentication.js      # Authentication page (index -6)
    ├── page-tokens.js              # Tokens page (index -7)
    ├── page-language.js            # Language page (index -8)
    ├── page-date-formats.js        # Date Formats page (index -9)
    ├── page-number-formats.js      # Number Formats page (index -10)
    ├── page-startup.js             # Startup page (index -11)
    ├── page-notifications.js       # Notifications page (index -12)
    ├── page-concierge.js           # Concierge page (index -13)
    ├── page-annotations.js         # Annotations page (index -14)
    ├── page-tours.js               # Tours page (index -15)
    ├── page-training.js            # Training page (index -16)
    ├── page-login-history.js       # Login History page (index -17)
    └── manager-23.js               # Lookups Manager settings (example)
```

### Module Responsibilities

| File | Purpose | Lines |
|------|---------|-------|
| `profile-manager.js` | Main manager, table initialization, data loading | ~720 |
| `profile-manager-collection.js` | CodeMirror JSON editor for Collection tab | ~200 |
| `profile-manager-settings.js` | Page navigation, crossfade transitions | ~180 |
| `profile-settings-service.js` | Centralized JSON read/write with debounced persist | ~505 |
| `pages/page-registry.js` | Handler loading, caching, placeholder fallback | ~270 |
| `pages/settings-page-base.js` | Base classes: `BaseSettingsPage`, `SimpleSettingsPage` | ~300 |
| `pages/page-*.js` | Individual settings page handlers | 50–220 each |

---

## Settings Page System

### Base Classes

All settings pages extend `BaseSettingsPage` or `SimpleSettingsPage`:

```javascript
// BaseSettingsPage - Minimal implementation
export class AccountPage extends BaseSettingsPage {
  constructor(options = {}) {
    super({ ...options, index: -1 });
  }

  async onInit() {
    // Initialize the page
    this.loadUserInfo();
  }

  isDirty() { return false; }  // Override for change detection
  async save() { /* Save logic */ }
}

// SimpleSettingsPage - Form binding included
export class LanguagePage extends SimpleSettingsPage {
  constructor(options = {}) {
    super({
      ...options,
      index: -8,
      formSelector: '#preferences-form',  // Auto-binds form fields
    });
  }

  async loadData() { /* Load from storage/API */ }
  async save() { /* Save and mark clean */ }
}
```

### ProfileSettingsService — Centralized JSON Storage

All settings pages read from and write to a single JSON object via the `ProfileSettingsService`. This service lives in `profile-settings-service.js` and is instantiated once by the Profile Manager, then passed to every page handler.

#### Why a Centralized Service?

- **One JSON blob** is persisted to localStorage (and eventually the server API)
- **Each section controls its own subtree** — no page touches another page's data
- **Optional sections are dynamic** — a section only exists in JSON if its page has saved data
- **Readability names** are stored as `_name` inside each section but are never used for routing

#### JSON Structure

The profile JSON is a flat object where each top-level key is a section key:

```json
{
  "-9": {
    "_name": "Date Formats",
    "dates": {
      "short": "YYYY-MM-DD",
      "long": "MMMM D, YYYY"
    },
    "times": {
      "short": "HH:mm",
      "long": "h:mm A"
    }
  },
  "23": {
    "_name": "Lookups Manager",
    "defaultView": "list",
    "pageSize": 50,
    "autoRefresh": true
  },
  "_legacy": {
    "language": "en-US",
    "dateFormat": "MM/DD/YYYY"
  }
}
```

- **Section keys** are the stringified `index` value: negative for internal pages, manager ID for managers.
- **`_name`** is stored for human readability only. The service never uses it for lookups.
- **`_legacy`** holds flat preferences during migration from the old storage scheme.

#### Settings Service API

| Method | Purpose |
|--------|---------|
| `get(sectionKey, path, defaultValue)` | Read a dotted-path value |
| `set(sectionKey, path, value)` | Write a dotted-path value |
| `delete(sectionKey, path)` | Remove a dotted-path value |
| `getSection(sectionKey)` | Get an entire section object (shallow clone) |
| `setSection(sectionKey, data, sectionName)` | Replace an entire section |
| `removeSection(sectionKey)` | Delete a whole section |
| `hasSection(sectionKey)` | Check if a section exists |
| `getSectionKeys()` | List all section keys |
| `batchSet(sectionKey, changes)` | Apply multiple writes in one persist |
| `getAll()` / `setAll(data)` | Get/replace the entire JSON blob |
| `onChange(pathPattern, callback)` | Listen for changes to a path (supports `*` wildcard) |
| `onAnyChange(callback)` | Listen for any change anywhere |
| `exportJSON()` | Export formatted JSON string |
| `importJSON(jsonString)` | Import and replace entire profile |
| `flush()` | Force immediate save (call before logout) |
| `destroy()` | Clear timers and listeners |

#### Page-Level Convenience Helpers

`BaseSettingsPage` provides thin wrappers so pages don't need to know their own section key:

```javascript
// Inside any page handler extending BaseSettingsPage:
this.getSetting('dates.short', 'YYYY-MM-DD');   // reads from this.sectionKey
this.setSetting('dates.short', 'YYYY-MM-DD');   // writes to this.sectionKey
this.setSettingsBatch({                         // batch write
  'dates.short': 'YYYY-MM-DD',
  'dates.long': 'MMMM D, YYYY',
});
this.getSectionData();                          // get entire section
this.setSectionData({ dates: { short: '...' } }, 'Date Formats');
```

#### Wiring: How the Service Reaches the Pages

1. **Profile Manager** creates the service in its constructor:
   ```javascript
   this.settingsService = new ProfileSettingsService({
     onAfterSave: (data) => { /* sync Collection tab */ },
   });
   ```

2. **Profile Manager** passes it to `SettingsTabHandler`:
   ```javascript
   this.settingsHandler = new SettingsTabHandler({
     settingsService: this.settingsService,
     // ...
   });
   ```

3. **SettingsTabHandler** passes it to `SettingsPageRegistry`:
   ```javascript
   this._pageRegistry = new SettingsPageRegistry({
     settingsService: this.settingsService,
     // ...
   });
   ```

4. **Page Registry** injects it into every page constructor as `options.settings`:
   ```javascript
   const handler = new HandlerClass({
     index: index,
     settings: this.settingsService,
     onDirtyChange: this.onDirtyChange,
   });
   ```

5. **BaseSettingsPage** stores it as `this.settings` and exposes the convenience helpers.

### Page Registry

The `SettingsPageRegistry` handles loading page handlers:

```javascript
// Internal pages (negative indices) - statically imported
const INTERNAL_PAGE_MAP = {
  '-1': AccountPage,
  '-2': NamesPage,
  // ... etc
};

// Manager pages (positive indices) - registered in switch
_getManagerHandlerClass(index) {
  switch (index) {
    case 23: // Lookups Manager
      return LookupsManagerPage;
    default:
      return null;  // Uses PlaceholderPage
  }
}
```

### Adding a New Settings Page

**For Internal Pages (negative index):**

1. Add HTML to `profile-manager.html`:
```html
<div class="settings-page" data-page-index="-18" id="settings-page--18">
  <section class="profile-section">
    <h3 class="section-title"><fa fa-icon></fa> Page Title</h3>
    <!-- Content -->
  </section>
</div>
```

2. Create `pages/page-{name}.js`:
```javascript
import { SimpleSettingsPage } from './settings-page-base.js';

export class NewPage extends SimpleSettingsPage {
  constructor(options) {
    super({ ...options, index: -18, formSelector: 'form' });
  }
  async loadData() { /* ... */ }
  async save() { /* ... */ }
}

export default NewPage;
```

3. Import and register in `pages/page-registry.js`:
```javascript
import NewPage from './page-{name}.js';

const INTERNAL_PAGE_MAP = {
  // ... existing
  '-18': NewPage,
};
```

**For Manager Pages (positive index = Manager ID):**

1. Add HTML to `profile-manager.html` with `data-page-index="{managerId}"`

2. Create `pages/manager-{id}.js` with handler class

3. Add case to `_getManagerHandlerClass()` in `page-registry.js`

4. If no handler exists, `PlaceholderPage` is shown automatically

---

## Placeholder Pages

Managers without specific settings pages display a placeholder:

```
┌─────────────────────────────┐
│  [Manager Name] Settings    │
├─────────────────────────────┤
│                             │
│     [wrench icon]           │
│                             │
│   Settings Not Available    │
│                             │
│  No settings are currently  │
│  available for [Manager].   │
│                             │
│  This feature will be       │
│  implemented in a future    │
│  update.                    │
│                             │
└─────────────────────────────┘
```

The placeholder is automatically shown for any manager ID that doesn't have a handler registered in `_getManagerHandlerClass()`.

---

## Main Menu Integration

### Group Names

Manager entries in the User Options table use their Main Menu group names:

```javascript
// From menu data: group.name
this._menuData.forEach((group) => {
  group.items?.forEach((item) => {
    this._managerGroups.set(item.managerId, group.name);
  });
});

// Applied when transforming sections
const groupName = this._getManagerGroup(index);
return {
  section: groupName,  // e.g., "Core", "Content", "System"
  // ...
};
```

This ensures the Profile Manager's left table groups managers the same way as the Main Menu.

### Icon and Label Merging

When Lookup 60 data is loaded, manager entries (index ≥ 0) are merged with Main Menu data:

```javascript
if (!isInternal && this._managerIcons[index]) {
  const menuInfo = this._managerIcons[index];
  const groupName = this._getManagerGroup(index);
  return {
    section: groupName,
    icon: menuInfo.iconHtml || section[1] || '<fa fa-cube></fa>',
    label: menuInfo.name || section[2] || 'Unknown',
    index: index,
    status_a1: 1,
  };
}
```

This ensures the Profile Manager always shows current icons and labels from the Main Menu.

---

## Collection Tab

The Collection tab is a JSON-based CodeMirror editor containing all profile settings in a single JSON block.

### Editor Controls

| Control | Purpose |
|---------|---------|
| **Undo** | Undo last edit (Ctrl+Z) |
| **Redo** | Redo edit (Ctrl+Shift+Z) |
| **Fold** | Fold all JSON blocks |
| **Unfold** | Unfold all JSON blocks |
| **Font** | Font size popup |
| **Prettify** | Format sorted JSON |

### Implementation

```javascript
// CollectionTabHandler class manages the editor
this.collectionHandler = new CollectionTabHandler({
  container: this.elements.collectionEditorContainer,
  onDirtyChange: (dirty) => {
    this.editHelper.setDirty(dirty);
    this.updateFooterButtons(dirty);
  },
});

// Initialize with current preferences
this.collectionHandler.init(this.getCollectionData());
```

---

## LithiumTable Integration

### Table Configuration

```javascript
this.optionsTable = new LithiumTable({
  container: this.elements.tableContainer,
  navigatorContainer: this.elements.navigatorContainer,
  tablePath: 'profile-manager/user-options',
  lookupKeyIdx: 4,  // Lookup 59 key_idx 4 = user-profile-sections
  cssPrefix: 'profile-options',
  storageKey: 'profile_options_table',
  app: this.app,
  readonly: true,
  panel: this.elements.leftPanel,
  panelStateManager: this.leftPanelState,
  onRowSelected: (rowData) => this.handleOptionSelected(rowData),
});
```

### Data Sources

**Lookup 60 (App UI Lists)** - Primary source:

```javascript
const rows = await authQuery(this.app.api, 26, { INTEGER: { LOOKUPID: 60 } });
const profileSectionsRow = rows?.find(row => row.key_idx === 0);

// Format: ["Section", "Icon", "Label", Index]
this.userOptions = sections.map((section, index) => ({
  section: section[0],  // Group name
  icon: section[1],     // Icon HTML
  label: section[2],    // Display label
  index: section[3],    // Page index (negative for internal, positive for managers)
  status_a1: 1,
}));
```

**Fallback** - Default options if Lookup 60 unavailable:

```javascript
getDefaultUserOptions() {
  const baseOptions = [
    { key_idx: 1, section: 'General', icon: '<fa fa-id-card></fa>',
      label: 'Account', index: -1, status_a1: 1 },
    // ... more internal options
  ];

  // Add manager entries from menu data with group names
  const managerOptions = Object.entries(this._managerIcons).map(([id, info]) => {
    const managerId = parseInt(id, 10);
    const groupName = this._getManagerGroup(managerId);
    return {
      key_idx: 100 + position,
      section: groupName,
      icon: info.iconHtml || '<fa fa-cube></fa>',
      label: info.name || `Manager ${id}`,
      index: managerId,
      status_a1: 1,
    };
  });

  return [...baseOptions, ...managerOptions];
}
```

---

## Index-Based Page Selection

When a row is selected in the LithiumTable:

```javascript
async handleOptionSelected(rowData) {
  if (!rowData) return;

  const index = rowData.index;

  // Update section label button
  this.elements.sectionLabelIcon.innerHTML = rowData.icon;
  this.elements.sectionLabelText.textContent = rowData.label;

  // Switch to settings tab
  if (this.currentTab !== 'settings') {
    this.switchTab('settings');
  }

  // Show the settings page (with crossfade)
  await this.settingsHandler.showPage(index, rowData);
}
```

The `SettingsTabHandler` manages:
- Crossfade transitions between pages
- Loading page handlers from the registry
- Calling `onShow()` / `onHide()` lifecycle methods

---

## Configuration

- **Table Definition:** `config/tabulator/profile-manager/user-options.json`
- **Lookup Source:** Lookup 60 (App UI Lists) via QueryRef 26
- **Schema Source:** Lookup 59 (Tabulator Schemas) key_idx 4
- **Storage:** `lithium_preferences` in localStorage

### Table Definition

```json
{
  "$schema": "../tabledef-schema.json",
  "table": "user-options",
  "title": "User Options",
  "readonly": true,
  "selectableRows": 1,
  "layout": "fitColumns",
  "columns": {
    "key_idx": { "title": "ID#", "field": "key_idx", "visible": false },
    "index": { "title": "Index", "field": "index", "visible": false },
    "section": { "title": "Section", "field": "section", "group": 1 },
    "icon": { "title": "<fa fa-wrench></fa>", "field": "icon",
              "formatter": "html", "width": 28 },
    "label": { "title": "Section", "field": "label" }
  }
}
```

---

## Layout Details

| Panel | Content | Width |
|-------|---------|-------|
| Left | LithiumTable for user options | 200-600px (default 314px) |
| Right | Tabbed interface (Settings, Collection) | Flexible |

- **Splitter:** LithiumSplitter component
- **Collapse:** Left panel can collapse via toolbar button
- **State Persistence:** PanelStateManager for width/collapsed state

---

## Event Handling

### Events Fired

| Event | When |
|-------|------|
| `LOCALE_CHANGED` | Language preference changed |
| `THEME_CHANGED` | Theme preference changed |
| `AUTH_RENEWED` | JWT token refreshed after preference save |

### Local Storage Keys

| Key | Content |
|-----|---------|
| `lithium_preferences` | User preferences JSON (managed by ProfileSettingsService) |
| `lithium_themes` | Cached theme list |
| `activeThemeId` | Current theme ID |
| `lithium_lookups_settings` | Lookups Manager settings (legacy — migrate to service) |

---

## Styling Reference

### CSS Classes

| Class | Element |
|-------|---------|
| `#profile-manager-page` | Main page container |
| `.profile-manager-container` | Panels wrapper |
| `#profile-left-panel` | Left panel with table |
| `#profile-right-panel` | Right panel with tabs |
| `#profile-table-container` | Table container |
| `#profile-navigator` | Navigator container |
| `#profile-tab-toolbar` | Tab toolbar |
| `.tab-panel` | Tab content panels |
| `.settings-page` | Individual settings pages |
| `.settings-placeholder` | Placeholder message container |

---

## Example: Lookups Manager Settings

The Lookups Manager (ID 23) demonstrates the complete pattern using the ProfileSettingsService:

**HTML** (`profile-manager.html`):
```html
<div class="settings-page" data-page-index="23" id="settings-page-23">
  <section class="profile-section">
    <h3 class="section-title"><fa fa-list></fa> Lookups Manager</h3>
    <form class="preferences-form">
      <div class="form-group">
        <label class="form-label">Default View</label>
        <select class="form-select" name="defaultView">
          <option value="list">List View</option>
          <option value="grid">Grid View</option>
        </select>
      </div>
      <!-- More fields... -->
    </form>
  </section>
</div>
```

**Handler** (`pages/manager-23.js`):
```javascript
export class LookupsManagerPage extends SimpleSettingsPage {
  constructor(options) {
    super({ ...options, index: 23, managerId: 23, formSelector: 'form' });
  }

  async loadData() {
    // Read from the Settings Service under section "23"
    const section = this.getSectionData();
    const data = section && Object.keys(section).length > 0
      ? { defaultView: this.getSetting('defaultView', 'list'), /* ... */ }
      : this._defaultValues;
    this.setFormData(data);
    this._originalData = JSON.parse(JSON.stringify(data));
  }

  async save() {
    const data = this.getFormData(this.formSelector);
    // Write the entire section via the Settings Service
    this.setSectionData(data, 'Lookups Manager');
    this._originalData = JSON.parse(JSON.stringify(data));
    this.setDirty(false);
    return { success: true, data };
  }
}
```

**Registration** (`pages/page-registry.js`):
```javascript
import LookupsManagerPage from './manager-23.js';

_getManagerHandlerClass(index) {
  switch (index) {
    case 23:
      return LookupsManagerPage;
    default:
      return null;  // Uses PlaceholderPage
  }
}
```

---

## Example: Date Formats Settings (Structured Data)

The Date Formats page (index -9) shows how to store nested data:

**Handler** (`pages/page-date-formats.js`):
```javascript
export class DateFormatsPage extends SimpleSettingsPage {
  constructor(options) {
    super({ ...options, index: -9, formSelector: 'form' });
  }

  async loadData() {
    // Read nested values using dotted paths
    this.setFormData({
      dateFormat: this.getSetting('dates.short', 'MM/DD/YYYY'),
      timeFormat: this.getSetting('times.short', '12h'),
    });
  }

  async save() {
    const data = this.getFormData(this.formSelector);
    // Write structured data as a complete section
    this.setSectionData({
      dates: {
        short: data.dateFormat,
        long: this._inferLongDateFormat(data.dateFormat),
      },
      times: {
        short: data.timeFormat === '12h' ? 'h:mm A' : 'HH:mm',
        long: data.timeFormat === '12h' ? 'h:mm:ss A' : 'HH:mm:ss',
      },
    }, 'Date Formats');
    this.setDirty(false);
    return { success: true, data };
  }
}
```

The resulting JSON under section `"-9"`:
```json
{
  "_name": "Date Formats",
  "dates": {
    "short": "YYYY-MM-DD",
    "long": "MMMM D, YYYY"
  },
  "times": {
    "short": "HH:mm",
    "long": "HH:mm:ss"
  }
}
```

---

## Migration from Legacy Storage

Old pages used separate `localStorage` keys per page (e.g. `lithium_lookups_settings`). New pages should use the ProfileSettingsService. During transition, pages can read from the service first and fall back to legacy keys:

```javascript
async loadData() {
  const section = this.getSectionData();
  const hasData = Object.keys(section).length > 1;

  if (hasData) {
    // Load from new structured storage
    this.setFormData({
      defaultView: this.getSetting('defaultView', 'list'),
    });
  } else {
    // Fallback: migrate from legacy flat storage
    const stored = localStorage.getItem('lithium_lookups_settings');
    if (stored) {
      const parsed = JSON.parse(stored);
      this.setFormData(parsed);
      // Optionally migrate to new storage on save
    }
  }
}
```

When saving, write to the Settings Service **and** optionally update the legacy key for backward compatibility until all consumers are migrated.

---

## Related Documentation

- [LITHIUM-MGR.md](LITHIUM-MGR.md) — Manager system overview
- [LITHIUM-MGR-QUERY.md](LITHIUM-MGR-QUERY.md) — Query Manager (Collection tab pattern)
- [LITHIUM-MGR-LOOKUPS.md](LITHIUM-MGR-LOOKUPS.md) — Lookups Manager
- [LITHIUM-TAB.md](LITHIUM-TAB.md) — LithiumTable component
- [LITHIUM-LUT.md](LITHIUM-LUT.md) — Lookup tables
- [LITHIUM-DEV.md](LITHIUM-DEV.md) — Development guidelines

---

Last updated: April 2026
