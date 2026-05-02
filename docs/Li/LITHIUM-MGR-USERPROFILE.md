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

### Left Panel: User Options Table

The left panel displays a grouped LithiumTable of user options. Groups represent logical categories (General, Security, Formatting, Application, and dynamic manager groups).

**Group Expansion Behavior:**
- On initial load, all groups start collapsed
- Only the group containing the currently active section is expanded
- For new users: the "General" group expands (containing the Account section)
- For returning users: the group of their last selected section expands
- Users can manually expand/collapse other groups as needed

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
| -12 | Application | Photo | `<fa fa-camera></fa>` |
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
├── profile-manager.js              # Main manager class (998 lines)
├── profile-manager.html            # HTML template
├── profile-manager.css             # Styles
├── profile-manager-collection.js   # Collection tab handler (318 lines)
├── profile-manager-settings.js     # Settings tab orchestrator (239 lines)
├── profile-manager-table.js        # Table management module (315 lines)
├── profile-settings-service.js     # Centralized JSON storage service (594 lines)
└── pages/                          # Settings page handlers
    ├── page-registry.js            # Registry for loading handlers (446 lines)
    ├── settings-page-base.js       # Base classes for pages (301 lines)
    ├── page-placeholder.js         # Placeholder for unimplemented managers (76 lines)
    ├── page-account.js             # Account page (index -1) (68 lines)
    ├── page-names.js               # Names page (index -2) (49 lines)
    ├── page-addresses.js           # Addresses page (index -3) (38 lines)
    ├── page-email.js               # E-Mail page (index -4) (38 lines)
    ├── page-phone.js               # Phone page (index -5) (19 lines)
    ├── page-authentication.js      # Authentication page (index -6) (19 lines)
    ├── page-tokens.js              # Tokens page (index -7) (17 lines)
    ├── page-language.js            # Language page (index -8) (113 lines)
    ├── page-date-formats.js        # Date Formats page (index -9) (945 lines)
    ├── page-number-formats.js      # Number Formats page (index -10) (19 lines)
    ├── page-startup.js             # Startup page (index -11) (17 lines)
    ├── page-notifications.js       # Notifications page (index -12) (17 lines)
    ├── page-concierge.js           # Concierge page (index -13) (17 lines)
    ├── page-annotations.js         # Annotations page (index -14) (17 lines)
    ├── page-training.js            # Training page (index -16) (17 lines)
    ├── page-login-history.js       # Login History page (index -17) (19 lines)
    ├── manager-5/                  # Manager folders use numeric naming
    │   ├── page-manager-5.html     # Crimson Manager placeholder
    │   ├── page-manager-5.css
    │   └── page-manager-5.js
    ├── manager-6/                  # Tour Manager (implemented)
    │   ├── page-manager-6.html
    │   ├── page-manager-6.css
    │   └── page-manager-6.js
    ├── manager-23/                 # Lookups Manager (implemented)
    │   ├── page-manager-23.html
    │   ├── page-manager-23.css
    │   └── page-manager-23.js
    └── ... (additional manager folders)
```

### Module Responsibilities

| File | Purpose | Lines |
|------|---------|-------|
| `profile-manager.js` | Main manager class, initialization, event handling | 998 |
| `profile-manager-collection.js` | CodeMirror JSON editor for Collection tab | 318 |
| `profile-manager-settings.js` | Page navigation, crossfade transitions | 239 |
| `profile-manager-table.js` | LithiumTable setup and data loading | 315 |
| `profile-settings-service.js` | Centralized JSON read/write with debounced persist | 594 |
| `pages/page-registry.js` | Handler loading, caching, placeholder fallback | 446 |
| `pages/settings-page-base.js` | Base classes: `BaseSettingsPage`, `SimpleSettingsPage` | 301 |
| `pages/page-*.js` | Individual settings page handlers | 19–945 each |

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

The profile JSON is a flat object where each top-level key is a section key. For the Date Formats section (-9), the sub-keys use user-friendly display names (e.g., "Short Date") instead of programmatic identifiers:

```json
{
  "-9": {
    "_name": "Date Formats",
    "dates": {
      "Short Date": "yyyy-MM-dd",
      "Medium Date": "yyyy-MMM-dd",
      "Long Date": "MMMM d, y",
      "Week Number": "yyyy-'W'nn"
    },
    "times": {
      "Short Time": "HH:mm",
      "Medium Time": "H:mm:ss",
      "Long Time": "HH:mm:ss.SSS"
    },
    "datetimes": {
      "Short DateTime": "yyyy-MM-dd HH:mm:ss",
      "Medium DateTime": "yyyy-MMM-dd (EEE) HH:mm:ss",
      "Long DateTime": "MMMM d, y 'at' HH:mm:ss"
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

### GlobalSettingsService — Application-Wide Settings Access

The `GlobalSettingsService` provides application-wide access to user preferences stored in localStorage under the key `lithium_preferences`. This service is available globally via `window.lithiumSettings` and allows any part of the application to read/write user settings consistently.

#### Why Global Access?

- **Centralized storage** — All user preferences in one place
- **Cross-manager access** — Any manager can read/write shared settings
- **Consistent API** — Same interface across the entire application
- **Real-time updates** — Changes are persisted immediately and listeners notified

#### Global Access Pattern

```javascript
// Read a setting
const theme = window.lithiumSettings.get('theme', 'dark');
const dateFormat = window.lithiumSettings.get('dates.short', 'yyyy-MM-dd');

// Write a setting
window.lithiumSettings.set('theme', 'light');
window.lithiumSettings.set('custom.timezone', 'America/New_York');

// Listen for changes
const unsubscribe = window.lithiumSettings.onChange((newSettings) => {
  console.log('Settings changed:', newSettings);
  // Update UI accordingly
});
```

#### API Reference

| Method | Purpose |
|--------|---------|
| `get(path, defaultValue)` | Read a dotted-path value |
| `set(path, value)` | Write a dotted-path value |
| `getAll()` | Get entire settings object |
| `onChange(callback)` | Subscribe to settings changes |
| `static getInstance()` | Get singleton instance |

#### Settings Structure

The global settings follow the same structure as the Profile Manager's JSON:

```json
{
  "theme": "dark",
  "language": "en-US",
  "dates": {
    "short": "yyyy-MM-dd",
    "medium": "yyyy-MMM-dd",
    "long": "MMMM d, y",
    "week": "yyyy-'W'nn"
  },
  "times": {
    "short": "HH:mm",
    "medium": "H:mm:ss",
    "long": "HH:mm:ss.SSS"
  },
  "datetimes": {
    "short": "yyyy-MM-dd HH:mm:ss",
    "medium": "yyyy-MMM-dd (EEE) HH:mm:ss",
    "long": "MMMM d, y 'at' HH:mm:ss"
  },
  "collection": {
    "font": {
      "fontSize": "13px",
      "fontFamily": "\"Vanadium Mono\", var(--font-mono, monospace)",
      "fontWeight": "normal"
    }
  }
}
```

**Note:** The global settings service provides direct access to the same data managed by the Profile Manager. Changes made through either interface are synchronized.

#### Usage Examples

```javascript
// In any manager or component
export class MyManager {
  init() {
    // Read current theme
    const theme = window.lithiumSettings.get('theme', 'dark');
    this.applyTheme(theme);

    // Listen for theme changes
    this.themeUnsubscribe = window.lithiumSettings.onChange((settings) => {
      if (settings.theme !== this.currentTheme) {
        this.applyTheme(settings.theme);
      }
    });
  }

  applyTheme(theme) {
    this.currentTheme = theme;
    this.container.classList.toggle('light-theme', theme === 'light');
  }

  destroy() {
    // Clean up listener
    if (this.themeUnsubscribe) {
      this.themeUnsubscribe();
    }
  }
}
```
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

1. **Profile Manager** creates a settings service wrapper in its constructor:
   ```javascript
   const globalSettings = window.lithiumSettings;
   this.settingsService = {
     getSection: (sectionKey, path, defaultValue) => {
       const fullPath = path ? `${sectionKey}.${path}` : sectionKey;
       return globalSettings.get(fullPath, defaultValue);
     },
     setSection: (sectionKey, path, value) => {
       const fullPath = path ? `${sectionKey}.${path}` : sectionKey;
       globalSettings.set(fullPath, value);
       // Sync to collection tab
       this.collectionHandler?.setData(globalSettings.getAll());
     },
     get: (path, defaultValue) => globalSettings.get(path, defaultValue),
     set: (path, value) => globalSettings.set(path, value),
     getSetting: (path, defaultValue) => globalSettings.get(path, defaultValue),
     getAll: () => globalSettings.getAll(),
     onChange: (callback) => globalSettings.onChange(callback),
   };
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

The `SettingsPageRegistry` handles loading page handlers and assets dynamically:

```javascript
// Internal pages (negative indices) - statically imported JS handlers
const INTERNAL_PAGE_MAP = {
  '-1': AccountPage,
  '-2': NamesPage,
  // ... etc
};

// Manager pages (positive indices) - conditionally imported
_getManagerHandlerClass(index) {
  switch (index) {
    case 23: // Lookups Manager
      return LookupsManagerPage;
    default:
      return null;  // Uses PlaceholderPage
  }
}

// Dynamic HTML/CSS loading for each section
async _loadPageAssets(pageName, container) {
  // Fetch HTML from /src/managers/profile-manager/pages/{pageName}/page-{pageName}.html
  // For managers, pageName = 'manager-' + index (e.g., 'manager-23')
  const htmlResponse = await fetch(`/src/managers/profile-manager/pages/${pageName}/page-${pageName}.html`);
  const htmlContent = await htmlResponse.text();

  // Parse and inject HTML into container
  const tempDiv = document.createElement('div');
  tempDiv.innerHTML = htmlContent;
  container.appendChild(tempDiv.firstElementChild);

  // Fetch and inject CSS if it exists
  const cssResponse = await fetch(`/src/managers/profile-manager/pages/${pageName}/page-${pageName}.css`);
  if (cssResponse.ok) {
    const cssContent = await cssResponse.text();
    const styleElement = document.createElement('style');
    styleElement.textContent = cssContent;
    document.head.appendChild(styleElement);
  }
}
```

**Dynamic Loading Benefits:**
- **On-demand loading**: HTML/CSS/JS for each section loads only when accessed
- **Reduced initial bundle**: Unused sections don't impact load time
- **User permissions**: Sections unavailable to certain users never load
- **Modular development**: Each section is self-contained with its own assets

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

1. Create `pages/manager-{id}/` directory

2. Create `pages/manager-{id}/page-manager-{id}.html` with `data-page-index="{managerId}"`

3. Create `pages/manager-{id}/page-manager-{id}.js` with handler class

4. Create `pages/manager-{id}/page-manager-{id}.css` (optional)

5. Add case to `_getManagerHandlerClass()` in `page-registry.js`

6. If no handler exists, `PlaceholderPage` is shown automatically with generated HTML

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

The placeholder is automatically shown for any manager ID that doesn't have a handler registered in `_getManagerHandlerClass()`. The `PlaceholderPage` dynamically creates the appropriate DOM structure with the correct `data-page-index` and displays the placeholder message.

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

## Style Preferences

This section documents the established styling patterns and preferences for Profile Manager settings pages, based on the Date Formats implementation.

### Table Layout and Styling

#### Container and Wrapper
- **Max width**: 900px for table wrappers (matches token table reference)
- **Border**: `1px solid var(--accent-alt-primary)` with `var(--border-radius-md)` rounding
- **Padding**: Use `var(--space-2)` for cell padding consistently

#### Header Styling
- **Background**: `var(--accent-alt-primary)` with white text
- **Font**: Mixed case (not uppercase) - use `text-transform: none`
- **Weight**: 600 (semibold)
- **Size**: `var(--font-size-xs)` for headers, `var(--font-size-sm)` for body text

#### Row and Cell Styling
- **Zebra stripes**: Use `var(--bg-row-odd)` and `var(--bg-row-even)` for alternating rows
- **Borders**: All cells have borders on all sides (`border: 1px solid var(--bg-row-border)`) - no perimeter exceptions
- **Vertical alignment**: Use `vertical-align: middle` for consistent alignment

#### Column Structure
- **First column**: 40px wide, centered, for icons (lock for built-in, trash for custom)
- **Icon buttons**: `df-delete-btn` styling with hover effects
- **Descriptive headers**: Use clear, descriptive column names (e.g., "Date Formats", "Time Formats" instead of generic "Name")

#### Form Elements
- **Inputs**: Include `border-radius: var(--border-radius-sm)` for consistency
- **Buttons**: Left-justified in table footers

### Data Handling Patterns

#### Adding New Items
- When adding custom entries, copy values from the bottom-most existing custom row instead of prompting for input
- Use structured JSON storage with nested objects for related settings

#### Live Saving
- Implement live saving for immediate user feedback
- Use the ProfileSettingsService for all data persistence

#### Icon Conventions
- **Lock icons** (`<fa fa-lock></fa>`) for built-in, non-editable items
- **Trash icons** (`<fa fa-trash></fa>`) for custom, deletable items
- Icons centered horizontally in dedicated column

### Implementation Notes

- Avoid special color styling for cell contents unless functionally necessary
- Maintain consistent spacing and alignment across all table-based settings pages
- Use the established patterns from Date Formats as the reference for future table implementations

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

## Date Formats Settings (Index -9)

The Date Formats page is a comprehensive date/time formatting configuration interface that provides live previews, timezone management, and custom format editing. It features three main sections: real-time datetime display, timezone controls, and format token reference tables.

### UI Layout

The page consists of four main sections arranged vertically:

1. **Top Preview Section** - Three side-by-side tables showing current datetime, timezone controls, and sample datetime
2. **Date Formats Table** - Built-in and custom date format patterns
3. **Time Formats Table** - Built-in and custom time format patterns
4. **DateTime Formats Table** - Built-in and custom datetime format patterns
5. **Token Reference** - LithiumTable showing available Luxon format tokens

### Top Preview Section

The top section provides real-time datetime display and timezone management:

#### Current DateTime Table
- **Caption**: "Current DateTime"
- **Purpose**: Shows current date/time in the selected timezone
- **Format**: `yyyy-MM-dd'T'HH:mm:ss.SSSZZ '('ZZZZ')'` (Luxon format with abbreviated timezone)
- **Updates**: Every second automatically
- **Features**: Two rows showing different timezone perspectives

#### Timezones Table
- **Caption**: "Timezones"
- **Purpose**: Controls timezone selection and display
- **Browser Timezone**: Shows detected browser timezone with abbreviation (e.g., "America/Vancouver (PDT)")
- **Override Timezone**: Custom dropdown picker with advanced features

#### Sample DateTime Table
- **Caption**: "Sample DateTime"
- **Purpose**: Shows a sample datetime for format testing
- **Default Sample**: `2020-01-01T14:03:02`
- **Picker**: Flatpickr date/time picker with seconds support
- **Features**: Two rows showing different timezone perspectives

### Timezone Picker Features

The custom timezone picker includes advanced functionality:

#### Dropdown Interface
- **Searchable**: Filter input with clear button (X)
- **Grouped Display**: Timezones organized by region (Africa, America, Asia, etc.) plus Abbreviations group
- **Scrollable**: OverlayScrollbars for smooth scrolling with improved speed
- **Resizable**: Drag bottom-right corner to resize (min 250x200px)
- **Abbreviations**: Common timezone abbreviations (PST, EST, GMT, etc.) shown as separate entries with full names

#### Selection Behavior
- **Default**: Shows "Browser Local" initially
- **Pre-fill**: When opened, shows current timezone in filter
- **Scroll**: Automatically scrolls to selected timezone
- **Format**: Displays as "TimezoneName (Abbrev)" (e.g., "America/New_York (EDT)")
- **Abbreviations**: Can select by abbreviation (e.g., "PST (Pacific Standard Time)") which maps to the correct timezone

#### Animation & UX
- **Popup Animation**: Scales from 50% with top-right transform origin
- **Smooth Transitions**: 350ms ease timing matching other Lithium popups
- **Filter Feedback**: Real-time filtering with region grouping
- **Keyboard Support**: Escape key closes, focus management

### Real-Time Display Features

#### Timezone Comparison
The top tables show two perspectives:
- **Primary Row**: Current/sample time in browser timezone
- **Secondary Row**: Current/sample time in override timezone (if different) or UTC

#### Update Timing
- **Current Time**: Updates every second
- **Sample Time**: Updates when changed via picker or timezone selection
- **Timezone Changes**: Immediately updates all displays

### Flatpickr Integration

#### Date/Time Picker
- **Trigger**: Calendar icon button next to sample datetime
- **Features**: Full date/time selection with seconds
- **Positioning**: Popup positioned below button with custom wrapper
- **Animation**: Matches popup animation timing
- **Format**: Accepts and outputs ISO datetime strings

#### Technical Implementation
- **Positioning**: Flatpickr with custom positioning and wrapper management
- **Wrapper Management**: Calendar moved to custom wrapper for styling and animation
- **Close Handling**: Coordinated close timing to match popup animation
- **State Sync**: Updates sample datetime setting and refreshes all previews
- **Reliability**: Enhanced reopening logic with retry mechanisms

### Format Tables Structure

#### Built-in Formats
Each table contains 3-4 built-in formats:
- **Dates**: Short, Medium, Long, Week Number
- **Times**: Short, Medium, Long
- **DateTimes**: Short, Medium, Long

#### Custom Formats
- **Add Button**: Plus icon in header to add custom formats
- **Editable**: Direct input editing with live preview
- **Delete Button**: Trash icon for custom formats
- **Storage**: Saved under descriptive names in settings JSON

#### Live Preview
- **Example Column**: Shows format applied to sample datetime
- **Current Column**: Shows format applied to current datetime
- **Real-time Updates**: Previews update instantly when formats change

### Token Reference

#### LithiumTable Integration
- **Data Source**: Format token definitions from Lookup tables
- **Navigation**: Full LithiumTable navigator for browsing tokens
- **Search/Filter**: Built-in table search and filtering
- **Grouping**: Tokens organized by category (dates, times, timezones)

### Settings Storage Structure

The Date Formats section stores data in a structured JSON format:

```json
{
  "_name": "Date Formats",
  "timezone": "America/New_York",
  "sample": "2020-01-01T14:03:02.000Z",
  "dates": {
    "Short Date": "yyyy-MM-dd",
    "Medium Date": "yyyy-MMM-dd",
    "Long Date": "MMMM d, y",
    "Week Number": "yyyy-'W'nn"
  },
  "times": {
    "Short Time": "HH:mm",
    "Medium Time": "H:mm:ss",
    "Long Time": "HH:mm:ss.SSS"
  },
  "datetimes": {
    "Short DateTime": "yyyy-MM-dd HH:mm:ss",
    "Medium DateTime": "yyyy-MMM-dd (EEE) HH:mm:ss",
    "Long DateTime": "MMMM d, y 'at' HH:mm:ss"
  }
}
```

### Implementation Architecture

#### Page Handler (`pages/page-date-formats.js`)
- Extends `BaseSettingsPage` (not `SimpleSettingsPage` due to complex UI)
- Manages multiple table interactions and real-time updates
- Handles Flatpickr integration and timezone picker
- Coordinates between timezone selection and display updates

#### Key Components
- **TimezonePicker Class**: Custom dropdown with search, grouping, and resize
- **Flatpickr Integration**: Custom wrapper and positioning
- **Real-time Updates**: Interval-based current time updates
- **Table Management**: Dynamic format tables with live editing

#### Event Handling
- **Timezone Changes**: Updates all displays and previews
- **Format Editing**: Live updates to example and current columns
- **Sample Changes**: Updates all format previews
- **Popup Management**: Proper cleanup and animation timing

### Advanced Features

#### Timezone Intelligence
- **Browser Detection**: Automatically detects IANA timezone from browser
- **Fallback Handling**: Falls back to UTC if timezone detection fails
- **Override Logic**: Provides override with smart defaults

#### Performance Optimizations
- **Debounced Updates**: Format changes update previews efficiently
- **Selective Rendering**: Only updates changed elements
- **DOM Reuse**: Keeps dropdown in DOM for reuse

#### Accessibility
- **Keyboard Navigation**: Full keyboard support in all controls
- **Screen Reader**: Proper labels and ARIA attributes
- **Focus Management**: Logical tab order and focus restoration

### Recent Enhancements

#### Version History
- **Initial Implementation**: Basic format tables with static previews
- **Real-time Updates**: Added live current time display
- **Timezone Management**: Introduced timezone picker and override
- **Custom Picker**: Advanced dropdown with search, grouping, and resize
- **Sample Picker**: Flatpickr integration with seconds support
- **Animation Polish**: Smooth popup animations matching Lithium patterns
- **Dual Display**: Timezone comparison in primary/secondary rows

This implementation serves as a reference for complex settings pages requiring real-time displays, custom controls, and sophisticated user interactions.

---

## Photo Settings (Index -12)

The Photo page provides a user photo editor with upload, camera capture, and precise positioning controls. Users can adjust their photo and save a 200x200 PNG to settings.

### UI Layout

```
┌─────────────────────────────────────────────────────────────┐
│  [Upload Photo] [Capture Photo] [Remove]           │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐     ┌─────────────┐                   │
│  │  ↑/↓ (Y)  │     │  Scale      │                   │
│  │  Fa icon   │     │  Fa icon   │                   │
│  └─────────────┘     └─────────────┘                   │
│          ←  ←  ←  [Photo Area]  →  →  →                    │
│          [Left/Right (X) with Fa icon]                     │
│                                                       │
│  ┌─────────────────────────────────────────────┐      │
│  │  Dimming Layer (50% opacity)              │      │
│  │  ┌───────────────────────────┐          │      │
│  │  │  400x400 Preview (rounded)  │          │      │
│  │  │  ┌─────────────────┐      │          │      │
│  │  │  │     Photo       │      │          │      │
│  │  │  │  (draggable)   │      │          │      │
│  │  │  └─────────────────┘      │          │      │
│  │  └───────────────────────────┘          │      │
│  └─────────────────────────────────────────────┘      │
│                                                       │
│          [Rotation] with Fa icon                        │
│          ←  ←  ←  ←  →  →  →  →                    │
│                                                       │
│  ┌──────┐                                            │
│  │Resize│ (bottom-right corner only)                  │
│  └──────┘                                            │
├─────────────────────────────────────────────────────────────┤
│  [Save Photo]  Status message                          │
└─────────────────────────────────────────────────────────────┘
```

### Features

#### Photo Editor Control (600x600 base)
- **Anchored to top-left**: Only bottom-right corner resizes via custom handle
- **Aspect ratio maintained**: 1:1 (square) when resizing
- **Min/Max size**: 300px to 800px

#### Custom Sliders with Icon Thumbs
All sliders use custom implementation (not native range inputs) to support icons on thumbs:

| Slider | Position | Icon | Direction | Range | Step | Purpose |
|--------|----------|------|-----------|-------|-------|
| X Position | Top | `<fa fa-left-right></fa>` | Left/Right | -100% to 100% | 1 | Horizontal positioning |
| Y Position | Left | `<fa fa-up-down></fa>` | Up/Down | -100% to 100% | 1 | Vertical positioning |
| Scale | Right | `<fa fa-up-down-left-right></fa>` | Vertical | 0.0 to 4.0 | 0.01 | Zoom control |
| Rotation | Bottom | `<fa fa-rotate></fa>` | Horizontal | -180 to 180 | 1 | Rotation control |

- **Tooltip display**: Shows current value when dragging or hovering (positioned outside image area)
- **Consistent direction**: Slider up = image up (Y-axis inverted for consistency with horizontal slider)

#### Interaction Methods
1. **Slider adjustment**: Drag slider thumb with icon to reposition/scale/rotate
2. **Direct image drag**: Click and drag the photo itself to reposition (updates X/Y sliders)
3. **Camera capture**: Uses `navigator.mediaDevices.getUserMedia()` with front-facing camera
4. **File upload**: Accepts any image/* with 10MB limit

#### Preview Overlay
- **400x400 rounded rectangle** centered in 600x600 editor
- **Scales proportionally** when editor is resized (maintains aspect ratio)
- **Dimming layer**: 50% opacity overlay with transparent cutout for preview area
- **Mask CSS**: Uses `mask-composite: exclude` for clean cutout effect

### Settings Storage Structure

```json
{
  "_name": "Photo",
  "photo": "iVBORw0KGgo...",  // 200x200 PNG base64 (no data: prefix)
  "original": "iVBORw0KGgo...", // Original uploaded photo base64
  "originalWidth": 1920,        // Original image width (pixels)
  "originalHeight": 1080,       // Original image height (pixels)
  "timestamp": "2026-04-30T19:00:00.000Z",
  "xPct": 0,                    // Horizontal offset (% of displayed bbox width, -100 to 100)
  "yPct": 0,                    // Vertical offset (% of displayed bbox height, -100 to 100)
  "scale": 1.0,              // Scale factor (1.0 = 100%)
  "rotation": 0               // Rotation in degrees (-180 to 180)
}
```

### File Structure

```
pages/photo/
├── page-photo.html    # Editor HTML (multi-line format for minifier compatibility)
├── page-photo.css     # Styles (custom sliders, tooltips, editor layout)
└── page-photo.js      # Handler (PhotoPage extends BaseSettingsPage)
```

### Implementation Details

#### Custom Slider System
The native range inputs were replaced with custom sliders to support icons on thumbs:

```javascript
// Custom slider creation
_createCustomSlider(config) {
  const wrapper = document.createElement('div');
  wrapper.className = `photo-custom-slider photo-custom-slider-${config.orientation}`;

  const track = document.createElement('div');
  const thumb = document.createElement('div');
  thumb.innerHTML = `<fa ${config.icon}></fa>`;  // Icon on thumb

  const tooltip = document.createElement('div');
  tooltip.className = 'photo-slider-tooltip';
  tooltip.textContent = value;  // Shows on drag/hover
}
```

#### Image Transform Pipeline
```
1. User uploads/captures photo → stored in this.photoData
2. Transform applied: translate(center + xPct% of bbox, center + yPct% of bbox) scale(scale) rotate(rotation)
   Note: xPct/yPct are percentages of displayed bounding box (factoring scale and rotation)
3. On save: Create temp canvas with full editor dimensions
4. Apply same transforms to temp canvas
5. Crop to preview overlay area (scaled to editor size)
6. Draw cropped area to 200x200 output canvas
7. Export as PNG base64 (data URL prefix stripped)
```

#### Loading Saved State
```javascript
// Load existing photo with all positions
async _loadExistingPhoto() {
  const section = this.getSectionData();  // Returns photoData object directly
  if (section?.photo) {
    this.photoData = `data:image/png;base64,${section.photo}`;
    this.originalPhotoData = section.original
      ? `data:image/png;base64,${section.original}`
      : this.photoData;

    // Restore slider positions (percentages relative to displayed bbox)
    this.scale = section.scale || 1;
    this.rotation = section.rotation || 0;

    // Restore percentages (migrate from old pixel values if needed)
    if (section.xPct !== undefined && section.yPct !== undefined) {
      this.xPct = section.xPct;
      this.yPct = section.yPct;
    } else {
      // Old format: approximate conversion
      const bbox = this._getDisplayedBBox();
      this.xPct = bbox.w ? ((section.x || 0) / bbox.w) * 100 : 0;
      this.yPct = bbox.h ? ((section.y || 0) / bbox.h) * 100 : 0;
    }

    // Restore original dimensions
    this.originalWidth = section.originalWidth || null;
    this.originalHeight = section.originalHeight || null;

    // Update custom sliders (y inverted for display)
    this._updateSliderPosition(this._customSliders.x, this.xPct);
    this._updateSliderPosition(this._customSliders.y, -this.yPct);
  }
}
```

### Toolbar
A narrow toolbar (`.photo-toolbar`) appears below the editor:
- **Width**: `fit-content` (not full width like standard LithiumToolbar)
- **Contains**: Save button + status message
- **Save button**: Disabled when no photo loaded, enables after upload/capture

### Build Considerations
- **HTML format**: Multi-line (not minified) to avoid `html-minifier-terser` parse errors with `<fa>` custom elements
- **CSS lint**: Uses modern `rgb()` syntax, no vendor prefixes
- **File size**: All files under 1000 lines (page-photo.js: ~670 lines)

### Related Files
- **Handler**: `src/managers/profile-manager/pages/photo/page-photo.js`
- **Styles**: `src/managers/profile-manager/pages/photo/page-photo.css`
- **HTML**: `src/managers/profile-manager/pages/photo/page-photo.html`
- **Registry**: `src/managers/profile-manager/pages/page-registry.js` (maps -12 to PhotoPage)

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

Last updated: April 27, 2026
