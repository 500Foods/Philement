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
| **Key Dependencies** | Event Bus, JWT, Permissions, Icons |

---

## Layout Structure

```diagram
┌─────────────────────────────────────────────────────────────┐
│                        Sidebar (resizable)                  │
│  ┌─────────────────────────────────────────────────────┐  │
│  │ Header (gradient, logo)                              │  │
│  ├─────────────────────────────────────────────────────┤  │
│  │ Menu Items (from getPermittedManagers())             │  │
│  │  - Style Manager (ID: 1)                            │  │
│  │  - Profile Manager (ID: 2)                          │  │
│  │  - Dashboard (ID: 3) [placeholder]                  │  │
│  │  - Lookups (ID: 4) [placeholder]                   │  │
│  │  - Queries (ID: 5) [placeholder]                    │  │
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

## Features

### Sidebar

- **Resizable width**: 220px - 400px (persisted to localStorage)
- **Collapsible**: Two-stage animation (icons stack horizontally, then vertically)
- **Mobile responsive**: Overlay mode on screens ≤ 768px
- **Manager menu**: Built from `getPermittedManagers()` with Punchcard gating

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
  }
}
```

### Manager Icons Registry

```javascript
this.managerIcons = {
  1: { icon: 'fa-palette', name: 'Style Manager' },
  2: { icon: 'fa-user-cog', name: 'Profile Manager' },
  3: { icon: 'fa-chart-line', name: 'Dashboard' },
  4: { icon: 'fa-list', name: 'Lookups' },
  5: { icon: 'fa-search', name: 'Queries' },
};

this.utilityManagerIcons = {
  'session-log': { icon: 'fa-scroll', name: 'Session Log' },
  'user-profile': { icon: 'fa-user-cog', name: 'User Profile' },
};
```

---

## Sidebar System

### Collapse Animation (Two-Stage)

**Stage 1 - Collapse:**

1. Sidebar narrows
2. Icons stack horizontally behind arrow
3. Arrow rotates left → up

**Stage 2 - Vertical Stack:**

1. Icons collapse vertically
2. Arrow rotates up → right

**Stage 1 - Expand:**

1. Icons collapse vertically back to stack
2. Arrow rotates right → down

**Stage 2 - Horizontal Fan:**

1. Icons fan out horizontally
2. Arrow rotates down → left

### Splitter

- Drag to resize sidebar (mouse and touch)
- Width constrained to 220-400px
- Persisted to localStorage

### Mobile Behavior

On screens ≤ 768px:

- Sidebar becomes overlay (slide-in from left)
- Overlay backdrop closes on click
- Reset to expanded state (no collapse)

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

---

## Related Documentation

- [LITHIUM-MGR.md](LITHIUM-MGR.md) - Manager overview
- [LITHIUM-MGR-LOGIN.md](LITHIUM-MGR-LOGIN.md) - Login Manager
- [LITHIUM-DEV.md](LITHIUM-DEV.md) - Development setup
