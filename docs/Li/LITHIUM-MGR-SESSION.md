# Session Log Manager

Utility Manager for viewing the current session's client-side action log.

---

## Overview

The Session Log Manager is a **Utility Manager** (not a Menu Manager) that provides a read-only view of the current session's client-side logging output. It is accessible from the sidebar footer via the scroll icon.

| Property | Value |
|----------|-------|
| **Location** | `src/managers/session-log/` |
| **Type** | Utility Manager |
| **Key** | `session-log` |
| **Access** | Sidebar footer button (always available) |

---

## Features

- **CodeMirror Viewer** — Syntax-highlighted log display with One Dark theme
- **Toolbar** — Action buttons for refresh, font settings, and coverage report
- **Font Selector** — Adjustable font size, family, and weight for the log viewer
- **Auto-refresh** — Automatically refreshes and scrolls to bottom when opened
- **Animated Refresh** — Refresh button spins during operation (matches LithiumTable navigator)

---

## Architecture

### Files

| File | Purpose |
|------|---------|
| `session-log.js` | Main manager class with toolbar, font popup, and CodeMirror integration |
| `session-log.html` | Template with toolbar and log viewer container |
| `session-log.css` | Styles for layout, toolbar, and CodeMirror viewer |

### Dependencies

- `log.js` — Session logging system (`getRawLog`, `formatLogText`)
- `manager-ui.js` — `createFontPopup` for font settings
- `codemirror.js` — CodeMirror editor components
- `codemirror-setup.js` — Shared editor configuration

---

## Toolbar Layout

```
┌─────────────────────────────────────────────────────────────────┐
│ [Refresh]                [Font] [Coverage]                     │
└─────────────────────────────────────────────────────────────────┘
```

| Button | Side | Action |
|--------|------|--------|
| **Refresh** | Left | Reloads log content with spinning animation |
| **Font** | Right | Opens font selector popup (size/family/weight) |
| **Coverage** | Right | Opens `/coverage/index.html` in new tab |

---

## Font Selector Popup

The font popup provides:

- **Size** — Number input (8-32px)
- **Family** — Dropdown with available fonts
- **Style** — Normal/Bold toggle
- **Preview** — Live preview of selected font
- **Actions** — Save, Cancel, Reset to Default

Font settings are persisted to localStorage and applied immediately to the CodeMirror editor.

---

## Auto-Refresh on Open

When the Session Log Manager is opened:

1. Log content is automatically refreshed
2. CodeMirror editor scrolls to the bottom
3. User sees the most recent log entries immediately

---

## Refresh Animation

The refresh button uses the same animation as the LithiumTable navigator:

```css
/* Adds spin class during refresh */
.session-log-refresh-spin > i,
.session-log-refresh-spin > svg {
  animation: session-log-refresh-spin 0.75s ease-in-out;
}

@keyframes session-log-refresh-spin {
  0%   { transform: rotate(0deg); }
  100% { transform: rotate(360deg); }
}
```

---

## CSS Classes

| Class | Purpose |
|-------|---------|
| `#session-log-page` | Main container (fills workspace) |
| `#session-log-toolbar` | Toolbar container |
| `.session-log-viewer` | CodeMirror container |
| `.session-log-refresh-spin` | Applied to refresh button during animation |

---

## Related Documentation

- [LITHIUM-MGR.md](LITHIUM-MGR.md) — Manager system overview
- [LITHIUM-MGR-QUERY.md](LITHIUM-MGR-QUERY.md) — Query Manager (similar toolbar/font pattern)
- [LITHIUM-BAR.md](LITHIUM-BAR.md) — Toolbar system documentation
- [LITHIUM-DEV.md](LITHIUM-DEV.md) — Session logging for debugging

---

Last updated: April 2026
