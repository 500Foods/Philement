# Lithium UI Reference

This document describes the user interface of the Lithium web application for the Crimson AI Agent. It includes DOM references, component descriptions, and typical usage patterns.

**Location:** `elements/003-lithium/crimson/LITHIUM.md`  
**Related:** See also the comprehensive documentation in `docs/Li/`

---

## Introduction

This file serves as a structured index of the Lithium application's UI components. It is designed to help AI agents (like Crimson) understand:

- What UI elements exist and where to find them in the DOM
- How users typically interact with different parts of the application
- The relationship between visual elements and their underlying functionality
- Common navigation patterns and workflows

When helping users, refer to the DOM selectors provided here to guide them to the correct UI elements.

---

## Login

The login screen is the entry point to Lithium. It appears before authentication and provides access to several pre-authentication panels.

### Main Login Panel

**DOM:** `#login-panel` (within `#login-page`)

The login form consists of:

| Element | DOM Selector | Purpose |
|---------|--------------|---------|
| Username Input | `#login-username` | Enter username (autofocus on load) |
| Password Input | `#login-password` | Enter password |
| Show/Hide Password | `#login-toggle-password` | Toggle password visibility |
| Clear Username | `#login-clear-username` | Clear the username field |
| Login Button | `#login-submit` | Submit credentials |
| Error Display | `#login-error` | Shows authentication errors |

### Footer Button Group

**DOM:** `.login-btn-group` inside `#login-form`

| Button | DOM Selector | Purpose | Keyboard Shortcut |
|--------|--------------|---------|-------------------|
| Language | `#login-language-btn` | Open language selector | `Ctrl+Shift+I` |
| Theme | `#login-theme-btn` | Open theme selector | `Ctrl+Shift+T` |
| Login | `#login-submit` | Authenticate | Enter |
| Logs | `#login-logs-btn` | View session logs | `Ctrl+Shift+L` |
| Help | `#login-help-btn` | Show help panel | `F1` |

### Subpanels

All subpanels share the class `.login-subpanel` and are initially hidden (`display: none`).

#### Theme Panel (`#theme-panel`)

- Lists available themes
- Each theme has an "Apply" button
- Close button: `#theme-close-btn`

#### Language Panel (`#language-panel`)

- Contains a filter input (`#language-filter`) for searching
- Tabulator table for language selection
- Shows current locale in `#language-current-code`
- Clear filter: `#language-clear-filter`
- Close button: `#language-close-btn`

#### Logs Panel (`#logs-panel`)

- CodeMirror editor displaying session logs
- Coverage report link: `#logs-coverage-btn`
- Close button: `#logs-close-btn`

#### Help Panel (`#help-panel`)

- Account section with password reset
- About section with version info (`#help-app-version`, `#help-build-date`)
- Close button: `#help-close-btn`

### Typical Login Flow

1. User enters username and password
2. Clicks Login button or presses Enter
3. On success: fade-out transition to Main Manager
4. On failure: error message appears in `#login-error`

---

## Main Menu

After authentication, the Main Manager provides the application layout with a sidebar and manager area.

### Layout Structure

```structure
#main-layout
├── .main-sidebar (left, resizable)
│   ├── .sidebar-header-gradient (logo)
│   ├── .sidebar-menu (nav items)
│   └── .sidebar-footer (utility buttons)
├── .sidebar-splitter (drag handle)
└── .manager-area (right, dynamic content)
    └── .manager-slot (per-manager container)
        ├── .manager-slot-header
        ├── .manager-slot-workspace
        └── .manager-slot-footer
```

### Sidebar

**DOM:** `#sidebar` (`.main-sidebar`)

#### Header

- Logo: `.sidebar-logo` (Lithium SVG)
- Title: `.sidebar-title`

#### Menu (`#sidebar-menu`)

Dynamically built from QueryRef 046. Menu is organized in collapsible groups:

```html
<div class="menu-group" data-group-id="1">
  <div class="menu-group-header">
    <span class="group-toggle-icon"><fa fa-chevron-down></fa></span>
    <span class="group-name">Group Name</span>
  </div>
  <div class="menu-group-items">
    <div class="menu-item" data-manager-id="4">
      <fa fa-database></fa>
      <span class="menu-item-name">Query Manager</span>
      <span class="menu-item-count">12</span>
    </div>
  </div>
</div>
```

**Manager IDs:**

| ID | Manager | Group |
|----|---------|-------|
| 2 | Session Logs | System |
| 3 | Version History | System |
| 4 | Queries | Data & Queries |
| 5 | Lookups | Data & Queries |
| 6 | Chats | AI |
| 7 | AI Auditor | AI |
| 8 | Dashboard | Content |
| 9 | Document Library | Content |
| 10 | Media Library | Content |
| 11 | Diagram Library | Content |
| 12 | Reports | Data & Queries |

#### Sidebar Footer (`#sidebar-icon-group`)

Five icon buttons arranged horizontally:

| Position | Button ID | Icon | Purpose |
|----------|-----------|------|---------|
| 0 | `#sidebar-collapse-btn` | `fa-angles-left` | Toggle sidebar collapse |
| 1 | `#sidebar-profile-btn` | `fa-user-cog` | Open Profile Manager |
| 2 | `#sidebar-theme-btn` | `fa-palette` | Open Style Manager |
| 3 | `#sidebar-logs-btn` | `fa-scroll` | View Session Log |
| 4 | `#sidebar-logout-btn` | `fa-sign-out-alt` | Open logout panel |

### Sidebar Splitter

**DOM:** `#sidebar-splitter`

- Drag to resize sidebar width (220px - 400px)
- Width persists in localStorage (`lithium_sidebar_width`)
- On mobile (≤768px), sidebar becomes overlay

### Mobile Menu Button

**DOM:** `#mobile-menu-btn`

- Only visible on mobile
- Opens sidebar as slide-in overlay

---

## Managers

A "Manager" is a modular component that handles a specific domain of functionality. Each manager is loaded dynamically into a "slot" in the manager area.

### Common Manager Structure

When a manager is loaded, it gets a slot with this structure:

```html
<div class="manager-slot" id="slot-manager-{id}">
  <div class="manager-slot-header">
    <div class="subpanel-header-group">
      <button class="subpanel-header-btn subpanel-header-primary">
        <fa fa-icon></fa>
        <span>Manager Name</span>
      </button>
      <button class="subpanel-header-btn subpanel-header-close slot-close-btn">
        <fa fa-xmark></fa>
      </button>
    </div>
  </div>
  <div class="manager-slot-workspace" id="slot-manager-{id}-workspace">
    <!-- Manager content injected here -->
  </div>
  <div class="manager-slot-footer">
    <div class="subpanel-header-group">
      <!-- Manager-specific footer buttons -->
      <button class="subpanel-header-btn slot-notifications-btn">
        <fa fa-bell></fa>
      </button>
      <!-- ... more buttons -->
    </div>
  </div>
</div>
```

### Manager Header

Common elements found in manager slot headers:

| Element | Class/Selector | Purpose |
|---------|----------------|---------|
| Manager Icon | `.subpanel-header-primary fa` | Visual identifier |
| Manager Name | `.subpanel-header-primary span` | Display name |
| Close Button | `.slot-close-btn` | Close this manager slot |
| Toolbar Buttons | `.subpanel-header-close` | Manager-specific actions |

### Manager Footer

Common elements found in manager slot footers:

| Button | Class | Purpose |
|--------|-------|---------|
| Reports | `.slot-reports-btn` | Access reports (placeholder in many managers) |
| Notifications | `.slot-notifications-btn` | View notifications |
| Tickets | `.slot-tickets-btn` | Support tickets |
| Annotations | `.slot-annotations-btn` | Notes/annotations |
| Tours | `.slot-tours-btn` | Interactive tours |
| LMS | `.slot-lms-btn` | Learning Management System |

### Switching Managers

1. User clicks a menu item in the sidebar
2. Current slot crossfades out
3. New manager loads (or existing slot shown)
4. New slot crossfades in

---

## Tables

Most data-heavy managers use the LithiumTable component built on Tabulator. Tables share common navigation and control patterns.

### Table Structure

```structure
.table-container (e.g., #queries-table-container)
├── .tabulator (Tabulator root)
│   ├── .tabulator-header
│   │   └── .tabulator-headers (column headers)
│   └── .tabulator-tableholder
│       └── .tabulator-table (data rows)
└── .table-loader (loading spinner, if active)
```

### Navigator

Each table has a Navigator control bar below it. The navigator has four blocks:

**DOM:** `.queries-nav-wrapper` or `.lookups-nav-wrapper`

#### Control Block

| Button | ID Pattern | Purpose |
|--------|------------|---------|
| Refresh | `#*-nav-refresh` | Reload table data |
| Filter Toggle | `#*-nav-filter` | Show/hide column filters |
| Menu | `#*-nav-menu` | Table options popup |
| Width | `#*-nav-width` | Panel width presets |
| Layout | `#*-nav-layout` | Table layout modes |
| Templates | `#*-nav-template` | Save/load column templates |

#### Move Block

| Button | ID Pattern | Purpose | Icon |
|--------|------------|---------|------|
| First | `#*-nav-first` | Go to first record | `fa-backward-fast` |
| Prev Page | `#*-nav-pgup` | Previous page | `fa-backward-step` |
| Previous | `#*-nav-prev` | Previous record | `fa-caret-left` |
| Next | `#*-nav-next` | Next record | `fa-caret-right` |
| Next Page | `#*-nav-pgdn` | Next page | `fa-forward-step` |
| Last | `#*-nav-last` | Last record | `fa-forward-fast` |

#### Manage Block

| Button | ID Pattern | Purpose | Icon |
|--------|------------|---------|------|
| Add | `#*-nav-add` | Add new record | `fa-plus` |
| Duplicate | `#*-nav-duplicate` | Copy selected | `fa-copy` |
| Edit | `#*-nav-edit` | Toggle edit mode | `fa-pen-to-square` |
| Save | `#*-nav-save` | Save changes | `fa-floppy-disk` |
| Cancel | `#*-nav-cancel` | Cancel changes | `fa-ban` |
| Delete | `#*-nav-delete` | Delete record | `fa-trash-can` |

**Note:** Save/Cancel are disabled until in edit mode AND changes exist.

#### Search Block

| Element | ID Pattern | Purpose |
|---------|------------|---------|
| Search Icon | `#*-search-btn` | Trigger search |
| Search Input | `#*-search-input` | Enter search terms |
| Clear Button | `#*-search-clear-btn` | Clear search |

### Column Features

#### Selector Column

First column (usually hidden width, 16px) showing:

- Empty: row not selected
- `▸` (caret-right): row selected
- `I` cursor: row in edit mode

Clicking the selector column header opens the **Column Chooser** popup.

#### Column Chooser

**DOM:** `.queries-col-chooser-popup` (dynamically created)

- Lists all columns with checkboxes
- Toggle visibility per column
- Scrollable if many columns

#### Header Filters

When enabled (via Filter Toggle button), columns show filter inputs:

```html
<div class="tabulator-header-filter">
  <input class="queries-header-filter-input" placeholder="filter...">
  <span class="queries-header-filter-clear">×</span>
</div>
```

### Edit Mode

Tables support inline editing:

1. Select a row (click on it)
2. Click Edit button or double-click row
3. Selector shows I-cursor icon
4. Cells become editable
5. Save/Cancel buttons enable when changes made

**Visual indicators:**

- Selected row: highlighted background
- Editing row: amber/yellow highlight

---

## Query Manager

**Manager ID:** 4  
**Path:** `src/managers/queries/`

A dual-panel interface for managing database queries. Left panel shows query list, right panel shows query details in tabs.

### Layout

```structure
.queries-manager-container
├── .queries-left-panel
│   ├── #queries-table-container (Tabulator table)
│   └── #queries-navigator-container (Navigator)
├── .queries-splitter (draggable)
└── .queries-right-panel
    ├── .queries-tabs-header
    └── .queries-tabs-content
```

### Left Panel (Query List)

- Table shows all queries from QueryRef 25
- Columns: Ref, Name, Type, Dialect, Status (configurable)
- Collapsible via `#queries-collapse-btn`
- Default width: 314px (compact), resizable

### Right Panel (Query Details)

#### Tab Buttons

| Tab | Data Attribute | Icon | Content |
|-----|----------------|------|---------|
| SQL | `data-tab="sql"` | `fa-database` | SQL editor |
| Test | `data-tab="test"` | `fa-flask` | Query tester |
| Summary | `data-tab="summary"` | `fa-file-lines` | Markdown editor |
| Preview | `data-tab="preview"` | `fa-eye` | Rendered Markdown |
| Collection | `data-tab="collection"` | `fa-code` | JSON editor |

#### Toolbar Buttons

| Button | ID | Purpose | Keyboard |
|--------|----|---------|----------|
| Undo | `#queries-undo-btn` | Undo in SQL editor | `Ctrl+Z` |
| Redo | `#queries-redo-btn` | Redo in SQL editor | `Ctrl+Shift+Z` |
| Font | `#queries-font-btn` | Font settings popup | - |
| Prettify | `#queries-prettify-btn` | Format SQL | `Ctrl+P` |
| Crimson | `.crimson-btn` | Open AI chat | `Ctrl+Shift+C` |

#### Tab Panes

**SQL Tab (`#queries-tab-sql`)**

- CodeMirror editor (`#queries-sql-editor`)
- SQL syntax highlighting
- Line numbers (padded to 4 digits)
- Read-only until edit mode

**Summary Tab (`#queries-tab-summary`)**

- CodeMirror with Markdown support
- Editable in edit mode

**Preview Tab (`#queries-tab-preview`)**

- Renders Markdown from Summary tab
- Uses `marked` library

**Collection Tab (`#queries-tab-collection`)**

- vanilla-jsoneditor tree view
- Editable JSON structure

### Footer Controls

The Query Manager adds custom footer buttons:

| Button | ID | Purpose |
|--------|----|---------|
| Print | `#queries-footer-print` | Print table |
| Email | `#queries-footer-email` | Email data |
| Export | `#queries-footer-export` | Export popup (PDF/CSV/TXT/XLS) |
| Data Source | `#queries-footer-datasource` | View vs Data mode |

### Typical Usage

1. Select a query from the left panel
2. SQL appears in SQL tab
3. Edit SQL (enter edit mode first)
4. Switch to Preview to see rendered Summary
5. Save changes with Save button or `Ctrl+S`

---

## Lookups Manager

**Manager ID:** 5  
**Path:** `src/managers/lookups/`

A three-panel interface for managing lookup tables (enumerations, status codes, reference data).

### Lookups Layout

```structure
.lookups-manager-container
├── .lookups-left-panel
│   ├── #lookups-parent-table-container (Parent table)
│   └── #lookups-parent-navigator
├── .lookups-splitter-left
├── .lookups-middle-panel
│   ├── .lookups-child-header
│   ├── #lookups-child-table-container (Child table)
│   └── #lookups-child-navigator
├── .lookups-splitter-right
└── .lookups-right-panel
    ├── .lookups-tabs-header
    └── .lookups-tabs-content
```

### Left Panel (Parent Table)

- Lists all lookup tables (QueryRef 30)
- Columns: LOOKUPID, LOOKUPNAME
- Selecting a row loads its values in middle panel
- Collapsible via `#lookups-collapse-left-btn`

### Middle Panel (Child Table)

- Shows values for selected lookup (QueryRef 34)
- Header shows: `#lookups-child-title` = "LookupName (ID)"
- Columns: LOOKUPVALUEID, KEY_IDX, VALUE_TXT, SORT_SEQ
- Independent Navigator (full CRUD)
- Collapsible via `#lookups-collapse-middle-btn`

### Right Panel (Details)

#### Lookups Tab Buttons

| Tab | Data Attribute | Icon | Content |
|-----|----------------|------|---------|
| JSON | `data-tab="json"` | `fa-code` | JSON editor |
| Summary | `data-tab="summary"` | `fa-file-lines` | Markdown editor |
| Preview | `data-tab="preview"` | `fa-eye` | Rendered preview |

#### Toolbar

- Font button: `#lookups-font-btn`
- Crimson button: `.crimson-btn`

### Parent-Child Relationship

1. User selects a lookup in left panel
2. Middle panel title updates to "LookupName (LOOKUPID)"
3. Child table loads values for that lookup
4. First value is auto-selected
5. Right panel shows details for selected value

### Lookups Typical Usage

1. Find and select lookup table in left panel
2. Browse/edit values in middle panel
3. Edit value details in right panel tabs
4. Use child table navigator for CRUD operations

---

## Common Keyboard Shortcuts

### Global Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+Shift+C` | Toggle Crimson AI chat |
| `Ctrl+Shift+L` | Open logout panel |

### Login Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `ESC` | Clear fields, focus username |
| `Ctrl+Shift+U` | Focus username |
| `Ctrl+Shift+P` | Focus password |
| `Enter` | Submit login |
| `F1` | Open help |

### Tables (when focused)

| Key | Action |
|-----|--------|
| `↑` / `↓` | Previous/Next record |
| `Page Up` / `Page Down` | Previous/Next page |
| `Home` / `End` | First/Last record |
| `Enter` | Enter edit mode |
| `Ctrl+S` | Save (when editing) |
| `ESC` | Cancel (when editing) |

### SQL Editor

| Shortcut | Action |
|----------|--------|
| `Ctrl+Z` | Undo |
| `Ctrl+Shift+Z` | Redo |
| `Ctrl+P` | Prettify SQL |

---

## Crimson AI Integration

The Crimson AI chat interface is accessible from manager toolbars.

### Opening Crimson

- Click the robot icon button (`.crimson-btn`) in any manager toolbar
- Press `Ctrl+Shift+C` globally

### Chat Window

**DOM:** `.crimson-popup`

```structure
.crimson-popup
├── .crimson-header (draggable)
│   └── .subpanel-header-group
├── .crimson-conversation
│   ├── .crimson-welcome (initial state)
│   └── .crimson-message (chat bubbles)
├── .crimson-input-area
│   ├── .crimson-input (textarea)
│   └── .crimson-send-btn
└── .crimson-resize-handle
```

### Features

- **Draggable:** Drag header to reposition
- **Resizable:** Drag bottom-right corner
- **ESC:** Close popup
- **Enter:** Send message
- **Shift+Enter:** New line in input

---

## DOM Reference Quick Index

### Global Elements

| Element | Selector |
|---------|----------|
| Login Page | `#login-page` |
| Main Layout | `#main-layout` |
| Sidebar | `#sidebar` |
| Manager Area | `#manager-area` |
| Logout Overlay | `#logout-overlay` |
| Logout Panel | `#logout-panel` |

### Common Patterns

| Pattern | Selector Example |
|---------|------------------|
| Slot Header | `.manager-slot-header` |
| Slot Workspace | `.manager-slot-workspace` |
| Slot Footer | `.manager-slot-footer` |
| Subpanel Header Group | `.subpanel-header-group` |
| Primary Button | `.subpanel-header-primary` |
| Close Button | `.subpanel-header-close` |
| Tab Button | `[data-tab="{tab-id}"]` |
| Tab Pane | `#{prefix}-tab-{tab-id}` |
| Navigator Wrapper | `.{prefix}-nav-wrapper` |
| Table Container | `#{prefix}-table-container` |
| Navigator Container | `#{prefix}-navigator-container` |

### Manager-Specific Prefixes

| Manager | Prefix | Example |
|---------|--------|---------|
| Queries | `queries` | `#queries-table-container` |
| Lookups (parent) | `lookups-parent` | `#lookups-parent-navigator` |
| Lookups (child) | `lookups-child` | `#lookups-child-table-container` |

---

## Data Attributes for Scripting

Elements often have data attributes for programmatic access:

| Attribute | Example | Usage |
|-----------|---------|-------|
| `data-manager-id` | `data-manager-id="4"` | Identify manager in menu |
| `data-group-id` | `data-group-id="2"` | Menu group identification |
| `data-tab` | `data-tab="sql"` | Tab button identification |
| `data-logout-type` | `data-logout-type="quick"` | Logout option type |
| `data-index` | `data-index="0"` | Icon position in footer |

---

## State Classes

Classes applied dynamically to indicate state:

| Class | Applied To | Meaning |
|-------|------------|---------|
| `.active` | Tab buttons, menu items | Currently selected/active |
| `.collapsed` | Panels, sidebars | Panel is collapsed |
| `.queries-edit-mode` | Table container | Table in edit mode |
| `.visible` | Popups, overlays | Element is shown |
| `.dragging` | Popup headers | Currently being dragged |
| `.resizing` | Splitters | Currently being resized |
| `.queries-nav-btn-active` | Filter button | Filters are visible |
| `.queries-nav-btn-danger` | Delete button | Destructive action |
| `.queries-nav-btn-has-popup` | Menu buttons | Has dropdown menu |
