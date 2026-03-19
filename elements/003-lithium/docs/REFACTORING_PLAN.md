# Lithium Codebase Refactoring Plan

## Executive Summary

This document outlines a comprehensive refactoring plan to split files exceeding 1,000 lines into smaller, more testable, and more modular components. The goal is to improve code maintainability, testability, and adherence to single responsibility principles.

## Current State Analysis

### Files Exceeding 1,000 Lines

| File | Lines | Type | Primary Concerns |
|------|-------|------|------------------|
| `src/managers/login/login.js` | 1,913 | JavaScript | Authentication, panels, language selection, logs viewer, password manager handling |
| `src/managers/queries/queries.css` | 1,730 | CSS | Query manager styling, table layouts, editor styles |
| `src/managers/main/main.js` | 1,608 | JavaScript | Sidebar management, slot system, menu building, logout handling |
| `src/app.js` | 1,597 | JavaScript | App bootstrap, auth flow, manager registry, token renewal |
| `src/managers/lookups/lookups.css` | 1,563 | CSS | Lookups manager styling, three-panel layout |
| `src/managers/main/main.css` | 1,266 | CSS | Main layout, sidebar, slot styling |
| `src/managers/lookups/lookups.js` | 1,259 | JavaScript | Parent/child tables, detail view, editors |

### Total Lines to Refactor: **9,957 lines**

---

## Detailed Refactoring Plans

### 1. `src/managers/login/login.js` (1,913 lines → ~400-500 lines)

#### Current Structure Analysis

The login manager handles multiple distinct responsibilities:

- Authentication flow (login/logout)
- Panel management (login, theme, logs, language, help)
- Language selection with Tabulator table
- Log viewer with CodeMirror
- Password manager UI suppression
- Keyboard shortcuts
- Version display

#### Proposed Module Structure

```structure
src/managers/login/
├── login.js                    # Main manager (~400 lines)
├── panels/
│   ├── login-panel.js          # Login form panel (~200 lines)
│   ├── language-panel.js       # Language selection (~300 lines)
│   ├── logs-panel.js           # Log viewer (~200 lines)
│   ├── theme-panel.js          # Theme selector (~100 lines)
│   └── help-panel.js           # Help panel (~100 lines)
├── services/
│   ├── password-manager.js     # Password manager suppression (~150 lines)
│   └── auth-service.js         # Authentication API calls (~150 lines)
└── utils/
    └── keyboard-shortcuts.js   # Keyboard handling (~100 lines)
```

#### Refactoring Steps

1. **Extract Panel Classes** (Priority: High)
   - Create base `LoginPanel` class with common panel functionality
   - Each panel becomes a separate class extending the base
   - Panels handle their own rendering, event listeners, and cleanup

2. **Extract Password Manager Service** (Priority: High)
   - Move all password manager suppression logic to a dedicated service
   - Makes the logic testable in isolation

3. **Extract Auth Service** (Priority: Medium)
   - Move API calls to `auth-service.js`
   - Centralize error handling

4. **Extract Keyboard Shortcuts** (Priority: Medium)
   - Move shortcut handling to utility module
   - Makes shortcuts configurable and testable

#### Expected Outcome

- **login.js**: ~400-500 lines (main orchestrator)
- **Testability**: Each panel can be unit tested independently
- **Maintainability**: Changes to one panel don't affect others

---

### 2. `src/managers/main/main.js` (1,608 lines → ~400-500 lines)

#### Main Manager Current Structure Analysis

The main manager handles:

- Sidebar management (collapsible, resizable)
- Menu building from server data
- Manager slot creation and management
- Utility manager handling
- Logout panel
- Mobile responsiveness

#### Main Manager Proposed Module Structure

```structure
src/managers/main/
├── main.js                     # Main manager (~400 lines)
├── components/
│   ├── sidebar.js              # Sidebar component (~350 lines)
│   ├── slot-manager.js         # Manager slot handling (~300 lines)
│   └── logout-panel.js         # Logout panel (~150 lines)
├── services/
│   └── menu-service.js         # Menu data fetching (~150 lines)
└── utils/
    ├── splitter.js             # Resizable splitter (~150 lines)
    └── storage.js              # localStorage helpers (~50 lines)
```

#### Main Manager Refactoring Steps

1. **Extract Sidebar Component** (Priority: High)
   - Move all sidebar logic (collapse, expand, menu building) to `sidebar.js`
   - Handle mobile sidebar separately
   - Makes sidebar behavior testable

2. **Extract Slot Manager** (Priority: High)
   - Move slot creation, header/footer button injection to `slot-manager.js`
   - Centralizes slot lifecycle management

3. **Extract Splitter Utility** (Priority: Medium)
   - Move splitter drag logic to reusable utility
   - Can be used by other managers (lookups, queries)

4. **Extract Menu Service** (Priority: Medium)
   - Move menu data fetching and caching to service
   - Makes menu loading testable

#### Main Manager Expected Outcome

- **main.js**: ~400-500 lines
- **Reusability**: Slot manager and splitter can be used by other managers
- **Testability**: Each component can be tested in isolation

---

### 3. `src/app.js` (1,597 lines → ~400-500 lines)

#### App Current Structure Analysis

The main app file handles:

- CSS error suppression patches
- Application initialization
- Authentication flow
- Manager loading and crossfading
- Token renewal and expiration
- PWA install handling
- Global event listeners

#### App Proposed Module Structure

```structure
src/
├── app.js                      # Main app (~400 lines)
├── core/
│   ├── css-patches.js          # CSS error suppression (~200 lines)
│   └── app-init.js             # Initialization sequence (~200 lines)
├── services/
│   ├── auth-flow.js            # Login/logout flow (~250 lines)
│   ├── token-manager.js        # Token renewal/expiry (~200 lines)
│   └── manager-loader.js       # Manager loading (~200 lines)
└── utils/
    └── pwa.js                  # PWA install handling (~50 lines)
```

#### App Refactoring Steps

1. **Extract CSS Patches** (Priority: High)
   - Move all CSS patching logic to `css-patches.js`
   - Makes patches testable and versionable

2. **Extract Token Manager** (Priority: High)
   - Move all JWT renewal, expiration warning, countdown logic
   - Centralizes token lifecycle management
   - Makes token handling testable

3. **Extract Manager Loader** (Priority: High)
   - Move manager import map, loading, crossfading logic
   - Handles slot-based manager system

4. **Extract Auth Flow** (Priority: Medium)
   - Move login/logout transition handling
   - Coordinate between LoginManager and MainManager

#### App Expected Outcome

- **app.js**: ~400-500 lines
- **Security**: Token management in one place
- **Testability**: Each service can be unit tested

---

### 4. `src/managers/lookups/lookups.js` (1,259 lines → ~350-400 lines)

#### Lookups Manager Current Structure Analysis

The lookups manager handles:

- Three-panel layout with splitters
- Parent table (lookups list)
- Child table (lookup values)
- Detail view with tabs (JSON, Summary, Preview)
- Font popup
- Footer controls

#### Lookups Manager Proposed Module Structure

```structure
src/managers/lookups/
├── lookups.js                  # Main manager (~350 lines)
├── components/
│   ├── parent-table.js         # Lookups list table (~150 lines)
│   ├── child-table.js          # Lookup values table (~150 lines)
│   └── detail-panel.js         # Right panel with tabs (~250 lines)
├── editors/
│   ├── json-editor.js          # JsonTree wrapper (~100 lines)
│   ├── summary-editor.js       # SunEditor wrapper (~100 lines)
│   └── preview-renderer.js     # Preview content (~100 lines)
├── ui/
│   ├── font-popup.js           # Font settings popup (~100 lines)
│   └── footer-controls.js      # Footer buttons (~150 lines)
└── utils/
    └── content-processor.js    # Content highlighting (~80 lines)
```

#### Lookups Manager Refactoring Steps

1. **Extract Table Components** (Priority: High)
   - Create `ParentTable` and `ChildTable` classes
   - Each handles its own initialization, data loading, events
   - Reuse LithiumTable component

2. **Extract Detail Panel** (Priority: High)
   - Move tab handling, editor management to `detail-panel.js`
   - Handles JSON, Summary, Preview tabs

3. **Extract Editor Wrappers** (Priority: Medium)
   - Create consistent API for JsonTree and SunEditor
   - Makes editors swappable

4. **Extract Footer Controls** (Priority: Medium)
   - Move print, email, export logic to separate module
   - Makes footer reusable across managers

#### Lookups Manager Expected Outcome

- **lookups.js**: ~350-400 lines
- **Consistency**: Editor wrappers provide consistent API
- **Reusability**: Footer controls can be used by queries manager

---

### 5. CSS Files Refactoring

#### Current CSS Structure Issues

- Large CSS files mix layout, component, and theme styles
- No clear separation of concerns
- Difficult to maintain and override

#### Proposed CSS Architecture

```structure
src/styles/
├── base/                       # Base styles
│   ├── reset.css
│   ├── variables.css           # CSS custom properties
│   └── typography.css
├── layout/                     # Layout systems
│   ├── sidebar.css
│   ├── panels.css
│   ├── slots.css
│   └── splitters.css
├── components/                 # Reusable components
│   ├── tables.css
│   ├── editors.css
│   ├── buttons.css
│   ├── tabs.css
│   └── popups.css
└── managers/                   # Manager-specific styles
    ├── login/
    │   ├── login.css
    │   ├── language-panel.css
    │   └── logs-panel.css
    ├── main/
    │   └── main.css
    ├── lookups/
    │   └── lookups.css
    └── queries/
        └── queries.css
```

#### CSS Refactoring Steps

1. **Extract Common Components** (Priority: High)
   - Move table styles to `components/tables.css`
   - Move editor styles to `components/editors.css`
   - Move button styles to `components/buttons.css`

2. **Extract Layout Styles** (Priority: High)
   - Move sidebar styles to `layout/sidebar.css`
   - Move splitter styles to `layout/splitters.css`
   - Move slot styles to `layout/slots.css`

3. **Manager-Specific Styles** (Priority: Medium)
   - Keep only manager-specific overrides
   - Import common components

#### CSS Expected Outcome

- **queries.css**: ~600-700 lines (from 1,730)
- **lookups.css**: ~500-600 lines (from 1,563)
- **main.css**: ~400-500 lines (from 1,266)
- **Maintainability**: Changes to common components affect all managers

---

## Implementation Priority

### Phase 1: High Impact, Low Risk (Week 1-2)

1. Extract CSS patches from `app.js`
2. Extract password manager service from `login.js`
3. Extract splitter utility (used by lookups, queries, main)
4. Begin CSS component extraction

### Phase 2: Core Components (Week 3-4)

1. Extract token manager from `app.js`
2. Extract sidebar component from `main.js`
3. Extract table components from `lookups.js`
4. Extract panel classes from `login.js`

### Phase 3: Manager Refactoring (Week 5-6)

1. Complete `login.js` refactoring
2. Complete `main.js` refactoring
3. Complete `lookups.js` refactoring
4. Complete `app.js` refactoring

### Phase 4: CSS Consolidation (Week 7-8)

1. Complete CSS component extraction
2. Update all managers to use new CSS structure
3. Remove old CSS files

---

## Testing Strategy

### Unit Testing

- Each extracted module should have corresponding unit tests
- Mock dependencies for isolated testing
- Test both success and error paths

### Integration Testing

- Test module interactions
- Verify manager functionality after refactoring
- Test crossfade transitions

### Visual Regression Testing

- Compare UI before and after CSS refactoring
- Ensure no visual changes (or intentional improvements)

---

## Migration Strategy

### Backward Compatibility

- Keep old files during transition
- Use feature flags to switch between old and new implementations
- Gradual rollout per manager

### Code Reviews

- Each refactoring PR should be < 400 lines changed
- Reviewers should verify:
  - No functionality changes
  - Proper test coverage
  - Documentation updates

---

## Success Metrics

1. **File Size**: No JavaScript file > 500 lines (excluding imports/comments)
2. **Test Coverage**: > 80% coverage for all extracted modules
3. **Code Duplication**: < 5% duplication (measured by jscpd)
4. **Maintainability**: Improved SonarQube maintainability rating
5. **Bundle Size**: No significant increase (±5% acceptable)

---

## Appendix: Module APIs

### Login Panel Base Class

```javascript
export class LoginPanel {
  constructor(container, options = {}) {}
  async render() {}
  show() {}
  hide() {}
  teardown() {}
}
```

### Sidebar Component

```javascript
export class SidebarComponent {
  constructor(container, options = {}) {}
  async init() {}
  buildMenu(menuData) {}
  toggleCollapse() {}
  setWidth(width) {}
  teardown() {}
}
```

### Token Manager

```javascript
export class TokenManager {
  constructor(api) {}
  scheduleRenewal(token) {}
  async renew() {}
  showExpirationWarning(seconds) {}
  clearTimers() {}
}
```

### Table Component

```javascript
export class ManagerTable {
  constructor(options) {}
  async init() {}
  async loadData(params) {}
  selectRow(id) {}
  teardown() {}
}
```

---

## Conclusion

This refactoring plan will transform the Lithium codebase from a monolithic structure to a modular, maintainable architecture. The key benefits are:

1. **Improved Testability**: Small, focused modules are easier to test
2. **Better Maintainability**: Changes are localized to specific modules
3. **Enhanced Reusability**: Common components can be shared across managers
4. **Easier Onboarding**: New developers can understand the codebase faster
5. **Reduced Risk**: Smaller files mean smaller blast radius for bugs

The phased approach ensures minimal disruption to ongoing development while steadily improving code quality.
