# Login Manager

The Login Manager (`src/managers/login/login.js`) handles user authentication and provides pre-authentication panels for theme selection, language, logs, and help.

**Location:** `src/managers/login/`

---

## Overview

| Aspect | Details |
|--------|---------|
| **Type** | Special Menu Manager (shown before authentication) |
| **Template** | `/src/managers/login/login.html` |
| **Styles** | `/src/managers/login/login.css` |
| **Key Dependencies** | Event Bus, JWT, Config, Tabulator, CodeMirror 6 |

---

## Features

### Authentication

- POST to Hydrogen `/api/auth/login`
- Handles 401 (invalid credentials), 429 (rate limit), 5xx, network errors
- JWT token storage via `core/jwt.js`
- Auto-login via URL query parameters (`?USER=...&PASS=...`)

### Panels

The login manager has multiple panels accessible from the login page:

| Panel | Purpose | Key Components |
|-------|---------|----------------|
| **Login** | Primary authentication form | Username, password, submit, error display |
| **Theme** | Theme selection before login | Theme preview, apply button |
| **Language** | Language/locale selection | Tabulator table with flags |
| **Logs** | View session logs | CodeMirror read-only editor |
| **Help** | Application information | Version, build date, keyboard shortcuts |

### UI Features

- FOUC (Flash of Unstyled Content) prevention
- Auto-focus username field
- Loading spinner on submit
- Fade-out transition on successful login
- Password visibility toggle
- CAPS LOCK detection
- Username persistence in localStorage

### Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `ESC` | Clear fields and focus username (on login panel) |
| `Ctrl+Shift+U` | Focus and select username |
| `Ctrl+Shift+P` | Focus and select password |
| `F1` | Open help panel |
| `Ctrl+Shift+I` | Open language panel |
| `Ctrl+Shift+T` | Open theme panel |
| `Ctrl+Shift+L` | Open logs panel |

---

## Architecture

### Class: LoginManager

```javascript
export default class LoginManager {
  constructor(app, container) {
    this.app = app;
    this.container = container;
    this.elements = {};
    this.isSubmitting = false;
    this.currentPanel = 'login'; // 'login', 'theme', 'logs', 'language', 'help'
  }
}
```

### Lifecycle Methods

#### init()

1. Load HTML template
2. Cache DOM elements
3. Set up event listeners
4. Set up lookup listeners (enable buttons when data arrives)
5. Load remembered username
6. Show panel with fade-in
7. Check for auto-login URL parameters

#### render()

- Fetches `/src/managers/login/login.html`
- Falls back to inline HTML if fetch fails
- Caches all DOM element references

#### setupEventListeners()

- Form submission
- Panel button clicks (language, theme, logs, help)
- Close buttons
- Keyboard shortcuts
- Password toggle, username clear

#### teardown()

- Destroy CodeMirror editor (if present)
- Destroy Tabulator table (if present)
- Remove keyboard listeners
- Clean up password manager observer
- Remove lookup event listeners

---

## Panel System

### Switching Panels

```javascript
async switchPanel(targetPanel) {
  // 1. Cancel any pending focus timer
  // 2. Toggle password manager UI
  // 3. Enable transition overlay
  // 4. Populate logs/language panels if needed
  // 5. Perform crossfade transition
  // 6. Update state and focus management
}
```

### Crossfade Transition

The panel system uses a crossfade effect:

1. Both panels positioned absolutely in container center
2. Outgoing panel fades from opacity 1 → 0
3. Incoming panel fades from opacity 0 → 1 (with slight delay)
4. CSS `display` property toggled after transition

---

## Language Panel

### Language Features

- Tabulator table with country flags
- Filter by language/country/locale
- Keyboard navigation (arrows, Enter, Home, End, Page Up/Down)
- Auto-detection of best guess locale via IP info
- Saves preference to localStorage

### Data Flow

1. `getLanguageData()` - Get all supported locales
2. `getBestGuessLocale()` - Detect user locale via IP info (optional)
3. `getSavedLocale()` - Check localStorage for saved preference
4. Initialize Tabulator with sorted data
5. `selectLanguage(locale)` - Save and emit event

---

## Logs Panel

### Logs Features

- CodeMirror 6 read-only editor
- One Dark theme
- Zero-padded line numbers (4 digits)
- Auto-populates on panel open

### CodeMirror Setup

```javascript
const extensions = [
  lineNumbers({ formatNumber: (n) => String(n).padStart(4, '0') }),
  highlightActiveLine(),
  history(),
  oneDark,
  EditorState.readOnly.of(true),
  // Custom theme for scrollbars, fonts
];
```

---

## Password Manager Integration

The login manager handles 1Password and similar password manager browser extensions:

### Suppression

- MutationObserver watches for injected elements
- Applies `display: none`, `visibility: hidden`, `opacity: 0`, `pointer-events: none`
- Reconnects observer to catch re-injections

### Cleanup

- After successful login, removes all password manager elements from DOM
- Prevents orphan elements in post-login view

---

## Event System

### Events Emitted

| Event | Data | When |
|-------|------|------|
| `auth:login` | `{ userId, username, roles, expiresAt }` | Login success |
| `locale:changed` | `{ lang, previousLang }` | Language selected |

### Events Listened

| Event | Action |
|-------|--------|
| `lookups:lookup_names:loaded` | Enable language button |
| `lookups:themes:loaded` | Enable theme button |
| `startup:complete` | Enable logs button |

---

## Testing

See [LITHIUM-TST.md](LITHIUM-TST.md) for testing patterns.

### Key Test Areas

- Form validation (empty credentials)
- Error handling (401, 429, network errors)
- Panel transitions
- Language table filtering
- Keyboard shortcuts

---

## Related Documentation

- [LITHIUM-MGR.md](LITHIUM-MGR.md) - Manager overview
- [LITHIUM-LIB.md](LITHIUM-LIB.md) - Libraries (Tabulator, CodeMirror)
- [LITHIUM-DEV.md](LITHIUM-DEV.md) - Development setup
