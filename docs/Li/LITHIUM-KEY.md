# Lithium Keyboard Shortcuts

Consolidated documentation for all keyboard shortcuts in Lithium.

---

## Global Shortcuts (Main Manager)

These work anywhere in the app, regardless of which manager is active.

| Shortcut | Action | Implementation |
|----------|--------|----------------|
| `Ctrl+Shift+X` | Close current manager | Clicks `.manager-ui-close-btn` |
| `Ctrl+Shift+Z` | Toggle zoom popup | Calls `toggleZoomPopup()` |
| `Ctrl+Shift+?` | Show shortcuts popup | Clicks `.manager-ui-shortcuts-btn` |
| `Ctrl+Shift+.` | Open manager settings | Clicks `.manager-ui-manager-menu-btn` |
| `Ctrl+Shift+F` | Focus search input | Focuses `.slot-header-search-input` |
| `Ctrl+Shift+L` | Open logout panel | Calls `showLogoutPanel()` |
| `F11` | Toggle fullscreen | Browser native |

### Implementation

```javascript
// In main.js - setup with capture phase
setupGlobalKeyboardShortcuts() {
  // Register shortcuts for tooltips
  registerShortcut('close', 'Ctrl+Shift+X', 'Close manager', () => this.handleCloseManager());
  registerShortcut('zoom', 'Ctrl+Shift+Z', 'Toggle zoom', () => this._handleZoomShortcut());
  // ... etc
  
  // Add listener in CAPTURE mode
  document.addEventListener('keydown', this.handleGlobalKeyDown, true);
}

handleGlobalKeyDown(event) {
  const isCtrlOrCmd = event.ctrlKey || event.metaKey;
  
  // Skip input fields (except search)
  const isInput = target.tagName === 'INPUT' || target.tagName === 'TEXTAREA';
  if (isInput && !(isCtrlOrCmd && event.shiftKey && event.key === 'F')) return;
  
  if (isCtrlOrCmd && event.shiftKey && event.key === 'X') {
    event.preventDefault();
    event.stopPropagation();
    this.handleCloseManager();
  }
  // ... etc
}
```

### Finding the Active Slot

```javascript
// Correct class: slot-visible (NOT visible or manager-slot-visible)
const activeSlot = document.querySelector('.manager-slot.slot-visible');
const closeBtn = activeSlot?.querySelector('.manager-ui-close-btn');
const searchInput = activeSlot?.querySelector('.slot-header-search-input');
```

### Common Pitfalls

1. **Wrong class name** — Use `.slot-visible`, not `.visible` or `.manager-slot-visible`
2. **Capture mode** — Use `{ capture: true }` on the listener
3. **stopPropagation()** — Call to prevent other handlers from intercepting

---

## Manager-Specific Shortcuts

### Login Manager

| Shortcut | Action |
|----------|--------|
| `Ctrl+Shift+U` | Focus username field |
| `Ctrl+Shift+P` | Focus password field |
| `Ctrl+Shift+I` | Click language button |
| `Ctrl+Shift+T` | Click theme button |
| `Ctrl+Shift+L` | Click log button |

### Queries Manager

| Shortcut | Action |
|----------|--------|
| `Ctrl+Shift+Z` | Undo |
| `Ctrl+Shift+Shift+Z` | Redo |
| `Ctrl+Shift+P` | Prettify |
| `Ctrl+Shift+1` | Section mode |
| `Ctrl+Shift+2` | JSON mode |

### Crimson

| Shortcut | Action |
|----------|--------|
| `Ctrl+Shift+C` | Toggle Crimson panel |
| `Escape` | Close panel |

### Style Manager

| Shortcut | Action |
|----------|--------|
| `Ctrl+Z` | Undo |
| `Ctrl+Shift+Z` | Redo |
| `Ctrl+P` | Prettify |
| `Ctrl+1` | Section mode |
| `Ctrl+2` | JSON mode |

### Tour

| Shortcut | Action |
|----------|--------|
| `ArrowLeft` | Previous step |
| `ArrowRight` | Next step |
| `Escape` | End tour |

---

## Editor Shortcuts (CodeMirror)

| Shortcut | Action |
|----------|--------|
| `F11` | Toggle fullscreen |
| `Ctrl+ Space` | Autocomplete |

---

## Tooltip Integration

Shortcuts registered with `registerShortcut()` automatically appear in button tooltips.

```javascript
import { registerShortcut } from '../core/manager-ui.js';

registerShortcut('logout', 'Ctrl+Shift+L', 'Open logout panel', () => {
  eventBus.emit('logout:show');
});
```

```html
<button data-tooltip="Logout" data-shortcut-id="logout">
  <fa fa-sign-out-alt></fa>
</button>
```

The tooltip displays: `Logout<br><span class="li-tip-kbd">Ctrl+Shift+L</span>`

### Adding Shortcuts to Header Buttons

```javascript
// In manager-ui.js - add data-shortcut-id to buttons
const closeBtn = createCloseButton(onClose);
closeBtn.dataset.shortcutId = 'close';
closeBtn.dataset.tooltip = 'Close';
```

The buttons need both:
- `data-shortcut-id` — matches the ID passed to `registerShortcut()`
- `data-tooltip` — the label shown in the tooltip

---

## Related Docs

- [LITHIUM-TIP.md](LITHIUM-TIP.md) — Tooltip system with keyboard shortcut integration
- [LITHIUM-MGR.md](LITHIUM-MGR.md) — Manager documentation
- [LITHIUM-DEV.md](LITHIUM-DEV.md) — Developer guide