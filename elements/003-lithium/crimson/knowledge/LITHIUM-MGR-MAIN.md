# Main Manager

The Main Manager (`src/managers/main/main.js`) handles the main application layout after authentication — the sidebar, manager area, and the slot system for loading other managers.

**Location:** `src/managers/main/`

---

## Overview

| Aspect | Details |
|--------|---------|
| **Type** | Special Menu Manager (wrapper for all authenticated views) |
| **Template** | `/src/managers/main/main.html` |
| **Styles** | `/src/managers/main/main.css` |
| **Key Dependencies** | Event Bus, JWT, Permissions, Icons, Menu Service |

---

## Layout Structure

```diagram
┌─────────────────────────────────────────────────────────────┐
│                        Sidebar (resizable)                  │
│  ┌─────────────────────────────────────────────────────┐  │
│  │ Header (gradient, logo, radar status icon)           │  │
│  ├─────────────────────────────────────────────────────┤  │
│  │ Menu Groups (from QueryRef 046, collapsible)        │  │
│  │  ▼ Administration                                   │  │
│  │    - Style Manager (ID: 1)               [12]       │  │
│  │    - Profile Manager (ID: 2)                        │  │
│  │  ▶ Data Management                                  │  │
│  │  ▶ Analytics                                        │  │
│  ├─────────────────────────────────────────────────────┤  │
│  │ Footer (utility buttons)                             │  │
│  │  - Theme, Profile, Logs, Logout                      │  │
│  └─────────────────────────────────────────────────────┘  │
├─────────────────────────────────────────────────────────────┤
│                     Manager Area (slots)                    │
│  ┌─────────────────────────────────────────────────────┐  │
│  │ Slot Header: Icon + Name | Close                    │  │
│  ├─────────────────────────────────────────────────────┤  │
│  │ Slot Workspace: Manager content                     │  │
│  ├─────────────────────────────────────────────────────┤  │
│  │ Slot Footer: Reports | Actions                       │  │
│  └─────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

---

## Dynamic Menu System

The Main Manager now retrieves menu data dynamically from the server using **QueryRef 046** after JWT authentication. This provides:

- **Grouped menu items** — One level deep grouping with collapsible sections
- **Icons from database** — Each menu item's icon is stored in the `collection` field
- **Count badges** — Optional count values displayed on the right side of menu items
- **Persistent collapse state** — Group collapse states are saved to localStorage

### Menu Data Structure (QueryRef 046)

The actual response from QueryRef 046 uses the `acuranzo_schema_lookup` table structure:

```javascript
// Response from QueryRef 046 - Get Main Menu
[
  {
    grpname: "System",            // Group name
    grpnum: 0,                     // Group ID (value_int)
    modname: "Session Logs",       // Module/manager name
    modnum: 2,                     // Manager ID (key_idx)
    grpsort: 0,                    // Group sort order
    modsort: 0,                    // Module sort order
    entries: 0,                    // Count badge (0 = hidden)
    collection: '{"Icon":"fa-receipt","Index":0}'  // JSON with icon and visibility
  },
  // ... more items
]
```

**Key fields:**

- `key_idx` → Manager ID (modnum): 2-12 for visible managers
- `value_int` → Group ID (grpnum): Groups items together
- `collection` → JSON string with `Icon` (Font Awesome class) and `Index` (visibility)
- `entries` → Count badge value (displayed if > 0)

**Visibility filtering:** Items with negative `Index` values are hidden from the menu (e.g., Main Menu=-2, Login=-1).

**Manager ID Mapping:**

| ID | Manager | Group |
|----|---------|-------|
| 7 | Dashboard Manager | Content (1) |
| 8 | Mail Manager | Content (1) |
| 9 | Profile Manager | User Management (2) |
| 10 | Session Manager | User Management (2) |
| 11 | Version Manager | User Management (2) |
| 12 | Calendar Manager | Shared (3) |
| 13 | Contact Manager | Shared (3) |
| 14 | File Manager | Shared (3) |
| 15 | Document Manager | Shared (3) |
| 16 | Media Manager | Shared (3) |
| 17 | Diagram Manager | Shared (3) |
| 18 | Chat Manager | AI (4) |
| 19 | Notification Manager | Support (5) |
| 20 | Annotation Manager | Support (5) |
| 21 | Ticketing Manager | Support (5) |
| 22 | Style Manager | Configuration (6) |
| 23 | Lookup Manager | Configuration (6) |
| 24 | Report Manager | Configuration (6) |
| 25 | Role Manager | Security (7) |
| 26 | Security Manager | Security (7) |
| 27 | AI Auditor Manager | Security (7) |
| 28 | Job Manager | Security (7) |
| 29 | Query Manager | Security (7) |
| 30 | Sync Manager | Security (7) |
| 31 | Camera Manager | Security (7) |
| 32 | Terminal | Security (7) |

### Menu Service (`src/shared/menu.js`)

The Menu Service handles fetching and caching:

```javascript
import { getMenu, buildManagerIconsRegistry } from '../../shared/menu.js';

// Fetch menu (uses cache if available)
const menuData = await getMenu(api);

// Build manager icons registry
const icons = buildManagerIconsRegistry(menuData);
// Result: { 1: {icon: 'fa-palette', name: 'Style Manager'}, ... }
```

| Function | Purpose |
|----------|---------|
| `getMenu(api, options)` | Fetch menu data (cached) |
| `fetchMenu(api)` | Force fetch from server |
| `clearMenuCache()` | Clear cached menu data |
| `buildManagerIconsRegistry(menuData)` | Build icon registry for managers |
| `getManagerIdsFromMenu(menuData)` | Extract manager IDs |

### Caching

- **In-memory cache** — Persists during session
- **localStorage cache** — 5-minute TTL for persistence across reloads
- **Automatic refresh** — On login and when cache expires

---

## Collapsible Groups

Menu groups are collapsible with the following behavior:

| State | Behavior |
|-------|----------|
| **Expanded (default)** | Shows group header and all menu items |
| **Collapsed** | Shows group header only, items hidden |
| **Sidebar Collapsed** | Groups hidden, flat list of icons only |

### Group Persistence

Group collapse states are saved to `localStorage` key `lithium_collapsed_groups` and restored on page load.

---

## Count Badges

Menu items can display count badges when `entries > 0`:

```css
/* Badge appears on right side of menu item */
.menu-item-count {
  background: var(--accent-secondary);
  color: white;
  font-family: var(--font-mono);
  font-size: var(--font-size-xs);
  border-radius: var(--border-radius-sm);
}
```

Badges are hidden when the sidebar is collapsed.

---

## Features

### Sidebar

- **Resizable width**: 220px - 400px (persisted to localStorage)
- **Collapsible**: Two-stage animation (icons stack horizontally, then vertically)
- **Mobile responsive**: Overlay mode on screens ≤ 768px
- **Manager menu**: Built from `getPermittedManagers()` with Punchcard gating
- **Radar status icon**: Animated SVG in header gradient tracking WS health + REST API requests (see [LITHIUM-WSS.md](LITHIUM-WSS.md#status-indicator))

### Manager Slots

Each loaded manager gets its own "slot" with:

- **Header**: Icon + manager name, close button
- **Workspace**: Manager content (scrollable)
- **Footer**: Reports placeholder, action buttons

Slots crossfade as a unit when switching between managers.

### Logout Panel

- Overlay + panel system
- Multiple logout options (quick, normal, public, global)
- Keyboard shortcuts (ESC, Enter, 1-4)

---

## Architecture

### Class: MainManager

```javascript
export default class MainManager {
  constructor(app, container, permittedManagers) {
    this.app = app;
    this.container = container;
    this.permittedManagers = permittedManagers || [];
    this.currentManagerId = null;
    this.isSidebarCollapsed = false;
    this.isResizing = false;
    
    // Menu data (loaded from QueryRef 046)
    this.menuData = null;
    this.collapsedGroups = new Set();
    
    // Manager icons registry (populated dynamically)
    this.managerIcons = {};
  }
}
```

### Initialization Flow

```diagram
┌─────────────────┐
│   constructor   │
│  (load collapsed│
│   groups state) │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│   init()        │
└────────┬────────┘
         │
         ▼
┌─────────────────┐     ┌─────────────────┐
│   render()      │────▶│ loadMenuData()  │◀── QueryRef 046
└────────┬────────┘     └────────┬────────┘
         │                       │
         │              ┌────────▼────────┐
         │              │ Build icons     │
         │              │ registry from   │
         │              │ menu data       │
         │              └────────┬────────┘
         │                       │
         ▼                       ▼
┌─────────────────┐     ┌─────────────────┐
│ buildSidebar()  │◀────│ Filter permitted│
│ (dynamic groups)│      │ managers        │
└────────┬────────┘     └─────────────────┘
         │
         ▼
┌─────────────────┐
│ loadSidebarState│
│ (apply collapse │
│  state, groups) │
└─────────────────┘
```

### Manager Icons Registry

The registry is now built dynamically from menu data:

```javascript
// Before: Static registry
this.managerIcons = {
  1: { icon: 'fa-palette', name: 'Style Manager' },
  // ...
};

// After: Populated from QueryRef 046 response
this.managerIcons = buildManagerIconsRegistry(this.menuData);
// Result: { 1: {icon: 'fa-palette', name: 'Style Manager'}, ... }
```

### Utility Manager Icons

Utility managers (footer buttons) remain static:

```javascript
this.utilityManagerIcons = {
  'session-log': { icon: 'fa-scroll', name: 'Session Log' },
  'user-profile': { icon: 'fa-user-cog', name: 'User Profile' },
};
```

---

## Sidebar System

### Menu DOM Structure

The sidebar menu has a hierarchical structure:

```html
<nav class="sidebar-menu" id="sidebar-menu">
  <div class="menu-group" data-group-id="1">
    <div class="menu-group-header">
      <span class="group-toggle-icon">
        <fa fa-chevron-down></fa>
      </span>
      <span class="group-name">Administration</span>
    </div>
    <div class="menu-group-items">
      <div class="menu-item" data-manager-id="1">
        <fa fa-palette></fa>
        <span class="menu-item-name">Style Manager</span>
        <span class="menu-item-count">12</span>
      </div>
      <!-- ... more items -->
    </div>
  </div>
  <!-- ... more groups -->
</nav>
```

### Collapse Animation (Two-Stage)

**Stage 1 - Collapse:**

1. Sidebar narrows
2. Icons stack horizontally behind arrow
3. Arrow rotates left → up
4. Menu groups collapse (hide items, show headers)

**Stage 2 - Vertical Stack:**

1. Icons collapse vertically
2. Arrow rotates up → right
3. Menu groups headers are hidden (flat icon list)

**Stage 1 - Expand:**

1. Icons collapse vertically back to stack
2. Arrow rotates right → down
3. Menu groups are restored to saved collapse state

**Stage 2 - Horizontal Fan:**

1. Icons fan out horizontally
2. Arrow rotates down → left
3. Full menu with groups visible

### Menu Collapse Behavior

| Sidebar State | Group Headers | Menu Items | Count Badges |
|---------------|---------------|------------|--------------|
| **Expanded** | Visible, clickable | Per group collapse state | Visible |
| **Collapsed** | Hidden | Flat list of icons only | Hidden |

### Splitter

- Drag to resize sidebar (mouse and touch)
- Width constrained to 220-400px
- Persisted to localStorage

### Mobile Behavior

On screens ≤ 768px:

- Sidebar becomes overlay (slide-in from left)
- Overlay backdrop closes on click
- Reset to expanded state (no collapse)
- Menu always shows full structure with groups expanded

---

## Slot System

### Creating a Slot

```javascript
createSlot(slotId, icon, name) {
  // Creates: <div class="manager-slot" id="${slotId}">
  //   <div class="manager-slot-header">...</div>
  //   <div class="manager-slot-workspace" id="${slotId}-workspace">...</div>
  //   <div class="manager-slot-footer">...</div>
  // </div>
}
```

### Injecting Buttons

Managers can inject buttons into slot headers/footers:

```javascript
// Add to header (before close button)
mainManager.addHeaderButtons(slotId, [
  { icon: 'fa-refresh', title: 'Refresh', onClick: handler }
]);

// Add to footer (before fixed icons)
mainManager.addFooterButtons(slotId, 'left', [
  { icon: 'fa-save', title: 'Save', onClick: handler }
]);
```

---

## Event System

### Events Emitted

| Event | Data | When |
|-------|------|------|
| `logout:initiate` | `{ logoutType }` | User selects logout option |

### Events Listened

| Event | Action |
|-------|--------|
| `network:online` | Hide offline indicator |
| `network:offline` | Show offline indicator |

---

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+Shift+L` | Open logout panel |
| `ESC` (in logout) | Close logout panel |
| `Enter` (in logout) | Execute quick logout |
| `1-4` (in logout) | Select logout option |

---

## State Persistence

### localStorage Keys

| Key | Value | Description |
|-----|-------|-------------|
| `lithium_sidebar_width` | Number | Current sidebar width in pixels |
| `lithium_sidebar_collapsed` | Boolean | Whether sidebar is collapsed |
| `lithium_last_manager` | String | Last used manager key (e.g., 'manager:1' or 'utility:user-profile') |
| `lithium_collapsed_groups` | Array | IDs of collapsed menu groups |
| `lithium_menu_data` | Object | Cached menu data from QueryRef 046 |
| `lithium_menu_cache_time` | Number | Timestamp of menu cache |

### Menu Cache TTL

Menu data is cached for **5 minutes** to reduce server requests while ensuring reasonably fresh data.

---

## Manager Loading

### Flow

1. User clicks sidebar menu item
2. `loadManager(managerId)` called
3. If slot doesn't exist, `createSlot()` creates it
4. App dynamically imports manager module
5. Manager instantiated and `init()` called
6. Manager renders into slot workspace
7. Slot made visible (crossfade if switching)

```javascript
async loadManager(managerId) {
  const { default: ManagerClass } = await import(managerPath);
  const manager = new ManagerClass(app, workspaceEl);
  await manager.init();
}
```

---

## Testing

See [LITHIUM-TST.md](LITHIUM-TST.md) for testing patterns.

### Key Test Areas

- Sidebar resize constraints
- Collapse animation states
- Mobile breakpoint behavior
- Slot creation and management
- Logout panel flow
- **Menu groups** - Collapse/expand functionality
- **Menu data** - Fetch from QueryRef 046, caching
- **Count badges** - Display and hide in collapsed mode
- **Group persistence** - Save/restore collapse state

### Menu Service Testing

```javascript
// Test menu data fetching
const menuData = await fetchMenu(api);
expect(menuData).toBeInstanceOf(Array);
expect(menuData[0]).toHaveProperty('items');

// Test icon registry building
const registry = buildManagerIconsRegistry(menuData);
expect(registry[1]).toHaveProperty('icon');
expect(registry[1]).toHaveProperty('name');

// Test caching
const cached = getCachedMenuData();
expect(cached).toEqual(menuData);
```

### Sidebar Menu Testing

```javascript
// Test group collapse/expand
mainManager._toggleGroup(1);
expect(mainManager.collapsedGroups.has(1)).toBe(true);

// Test collapsed state persistence
mainManager._saveCollapsedGroups();
const stored = localStorage.getItem('lithium_collapsed_groups');
expect(JSON.parse(stored)).toContain(1);
```

---

## Related Documentation

- [LITHIUM-MGR.md](LITHIUM-MGR.md) - Manager overview
- [LITHIUM-MGR-LOGIN.md](LITHIUM-MGR-LOGIN.md) - Login Manager
- [LITHIUM-MGR-QUERY.md](LITHIUM-MGR-QUERY.md) - Query Manager
- [LITHIUM-DEV.md](LITHIUM-DEV.md) - Development setup
- [LITHIUM-JWT.md](LITHIUM-JWT.md) - JWT authentication

## Files Changed

### New Files

| File | Description |
|------|-------------|
| `src/shared/menu.js` | Menu service for fetching and caching menu data from QueryRef 046 |
| `src/managers/session-logs/session-logs.js` | New manager - Session Logs (ID: 2) |
| `src/managers/session-logs/session-logs.css` | Styles for Session Logs manager |
| `src/managers/version-history/version-history.js` | New manager - Version History (ID: 11) |
| `src/managers/version-history/version-history.css` | Styles for Version History manager |
| `src/managers/dashboard/dashboard.js` | New manager - Dashboard (ID: 8) |
| `src/managers/dashboard/dashboard.css` | Styles for Dashboard manager |
| `src/managers/document-library/document-library.js` | New manager - Document Library (ID: 9) |
| `src/managers/document-library/document-library.css` | Styles for Document Library manager |
| `src/managers/media-library/media-library.js` | New manager - Media Library (ID: 10) |
| `src/managers/media-library/media-library.css` | Styles for Media Library manager |
| `src/managers/diagram-library/diagram-library.js` | New manager - Diagram Library (ID: 11) |
| `src/managers/diagram-library/diagram-library.css` | Styles for Diagram Library manager |
| `src/managers/reports/reports.js` | New manager - Reports (ID: 12) |
| `src/managers/reports/reports.css` | Styles for Reports manager |
| `src/managers/chats/chats.js` | New manager - Chats (ID: 6) |
| `src/managers/chats/chats.css` | Styles for Chats manager |
| `src/managers/ai-auditor/ai-auditor.js` | New manager - AI Auditor (ID: 7) |
| `src/managers/ai-auditor/ai-auditor.css` | Styles for AI Auditor manager |

### Updated Files

| File | Description |
|------|-------------|
| `src/app.js` | Updated manager registry and imports for new manager IDs (2-12) |
| `src/core/permissions.js` | Updated permitted managers list to include new IDs |
| `src/managers/main/main.js` | Dynamic menu building with collapsible groups |
| `src/managers/main/main.css` | Styles for groups, badges, collapsed state |
| `docs/Li/LITHIUM-MGR-MAIN.md` | Documentation for dynamic menu system and manager list |
