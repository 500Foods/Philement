# Tour Manager

The Tour Manager provides in-app guided tours using [Shepherd.js](https://shepherdjs.dev/). Tours are stored in **Lookup #43** (Tours) and are lazy-loaded on first use.

**Location:** `src/managers/tour/`

**Manager ID:** 6 (System Manager, Group 0)

**Type:** Utility / Popup Manager (not a traditional slot-based manager)

---

## Overview

| Aspect | Details |
|--------|---------|
| **Library** | Shepherd.js (dynamic import) |
| **Data Source** | Lookup #43 — lazy-loaded on first tour request |
| **Activation** | Tour button in manager toolbars or Crimson suggestions |
| **Key Files** | `tour.js`, `tour.css` |

The Tour Manager is a singleton that wraps Shepherd.js. It does not occupy a workspace slot — tours display as modal overlays with step-by-step highlighting of UI elements.

---

## Lookup #43: Tours Schema

Each tour is a row in Lookup #43 with the tour definition stored in the `collection` JSON column.

### Tour JSON Structure

```json
{
  "icon": "<fa fa-clipboard-list></fa>",
  "manager": "023.Lookup Manager",
  "steps": [
    {
      "title": "Step Title",
      "text": "Step description with <b>HTML</b> support.",
      "element": "#dom-selector",
      "position": "bottom"
    }
  ]
}
```

### Top-Level Fields

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `icon` | `string` | Yes | Font Awesome `<fa>` tag for the tour's icon |
| `manager` | `string` | Yes | Manager identifier using lithium.json naming convention (e.g., `"023.Lookup Manager"`). Used to filter tours by accessible managers and to open the manager before the tour starts. |
| `steps` | `array` | Yes | Array of step objects |

### Step Object Fields

The tour JSON uses a simple format that is extracted and converted to Shepherd's `addStep()` options at runtime.

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `title` | `string` | No | Step heading (displayed as `<h3>`) |
| `text` | `string` | No | Step content (supports HTML) |
| `element` | `string` | No | CSS selector for the target element. If omitted or the element is not found, Shepherd displays the step **centered** on screen **without** the modal overlay/mask. |
| `position` | `string` | No | Placement relative to target: `"top"`, `"bottom"`, `"left"`, `"right"`, `"auto"`. Defaults to `"bottom"`. |

### Shepherd Conversion

At runtime, each step is converted to a Shepherd step:

```javascript
// Tour JSON step
{ "title": "Welcome", "text": "Hello!", "element": "#app", "position": "bottom" }

// Becomes Shepherd addStep() options:
{
  title: "Welcome",
  text: "Hello!",
  attachTo: { element: "#app", on: "bottom" },
  buttons: [ /* auto-generated */ ]
}
```

Button options (Back, Next, Close, Done) are auto-generated based on step position in the array.

### Manager Field Convention

The `manager` field follows the same naming convention as `lithium.json`:

```
"###.Manager Name"
```

Examples:

| Value | Meaning |
|-------|---------|
| `"023.Lookup Manager"` | Tour for the Lookup Manager (ID 23) |
| `"029.Query Manager"` | Tour for the Query Manager (ID 29) |
| `"022.Style Manager"` | Tour for the Style Manager (ID 22) |

This field serves two purposes:

1. **Access filtering** — Only show tours for managers the user has permission to access
2. **Auto-open** — Before launching the tour, open the specified manager

---

## Lazy Loading Strategy

Tour data is **not** loaded at startup. The Lookup #43 data is fetched only when:

1. A user clicks a tour button in a manager toolbar
2. Crimson suggests a tour
3. The user requests a tour from the tour list popup

```javascript
// First request triggers fetch
async function getTours(api) {
  if (!_toursCache) {
    // QueryRef 26 with LOOKUPID 43 to get tour entries
    const entries = await authQuery(api, 26, {
      INTEGER: { LOOKUPID: 43 }
    });
    _toursCache = entries
      .filter(e => e.collection) // Only tours with definitions
      .map(e => ({
        id: e.key_idx,
        name: e.value_txt,
        definition: e.collection, // The JSON object
      }));
  }
  return _toursCache;
}
```

Subsequent calls return the cached array. A `refreshTours()` function can be exposed to force a re-fetch if needed.

---

## Tour UI Controls

Shepherd's default buttons are customized:

### Button Set

| Button | Icon/Label | Action |
|--------|-----------|--------|
| **Back** | `<` | Go to previous step |
| **Next** | `>` | Go to next step |
| **Close** | `<fa fa-xmark></fa>` | Cancel/end the tour |
| **Tour List** | `<fa fa-list></fa>` | Open popup showing all available tours |

The **Tour List** button is injected into Shepherd's header or footer area. Clicking it opens a lightweight popup listing all tours the user can access (filtered by their permitted managers).

### Tour List Popup

```
┌─────────────────────────────────────────────┐
│  Available Tours                     [✕]   │
├─────────────────────────────────────────────┤
│  <fa fa-clipboard-list></fa> Lookups Tour   │
│  <fa fa-database></fa> Query Tour           │
│  <fa fa-palette></fa> Style Tour            │
│  ...                                        │
└─────────────────────────────────────────────┘
```

Selecting a tour from the list:

1. Ends the current tour (if running)
2. Opens the tour's target manager (if different from the current manager)
3. Launches the selected tour

---

## Element Targeting Behavior

When a step's `element` field is:

- **Present and matches a DOM element** — Shepherd highlights that element with a modal overlay around it
- **Present but the element is not found** — Shepherd displays the step **centered** on screen **without** the modal mask
- **Omitted** — Same as not found: centered without mask

This means tour authors can include informational steps (e.g., "Welcome to the Lookups Manager") that float in the center of the screen without masking any UI.

---

## Integration Points

### Manager Footer Tours Button

Each manager's footer includes a **Tours** button (alongside Print, Email, Export). Clicking it triggers the tour flow:

```javascript
// In footer setup:
const toursBtn = footer.querySelector('.slot-footer-tours');
toursBtn.addEventListener('click', () => {
  this._handleTours();
});

async _handleTours() {
  const managerName = '023.Lookup Manager'; // Matches lithium.json key
  const tours = await getTours(this.app.api);
  
  // Filter tours for this manager
  const managerTours = tours.filter(t => t.definition.manager === managerName);
  
  if (managerTours.length === 1) {
    // Single tour — launch directly
    await launchTour(managerTours[0]);
  } else if (managerTours.length > 1) {
    // Multiple tours — show list filtered to this manager
    showTourList(managerTours);
  } else {
    // No manager-specific tours — show all available tours
    showTourList(tours);
  }
}
```

### Tour-to-Manager Mapping

Tours are mapped to managers via the `manager` field in the tour JSON definition:

```json
{
  "manager": "023.Lookup Manager",
  "icon": "<fa fa-clipboard-list></fa>",
  "steps": [...]
}
```

The `manager` value uses the **lithium.json naming convention** — the zero-padded ID prefix followed by the manager name (e.g., `"023.Lookup Manager"`). This field serves two purposes:

1. **Filtering** — When a user clicks the Tours button on a specific manager, only tours with a matching `manager` field are shown
2. **Auto-open** — Before launching a tour, the system ensures the target manager is loaded and active

### Fallback Behavior

| Scenario | Behavior |
|----------|----------|
| One tour matches current manager | Launch tour directly |
| Multiple tours match current manager | Show tour list filtered to those tours |
| No tours match current manager | Show tour list with all available tours |
| Tours not yet loaded | Fetch from Lookup #43 (QueryRef 26) on first click, then filter |
| Tour ID not found | Show tour list with all available tours |

### Crimson Integration

Crimson can suggest tours in its response metadata:

```json
{
  "suggestions": {
    "offerTours": [
      { "tourId": 1, "name": "Lookups Tour" }
    ]
  }
}
```

See [LITHIUM-MGR-CRIMSON.md](LITHIUM-MGR-CRIMSON.md) — Response Structure section.

### Global Keyboard Shortcut (Optional)

A global shortcut could trigger the tour list:

```javascript
document.addEventListener('keydown', (e) => {
  if ((e.ctrlKey || e.metaKey) && e.shiftKey && e.key === 'T') {
    e.preventDefault();
    openTourList();
  }
});
```

---

## Architecture

### Singleton Pattern

Like Crimson, the Tour Manager uses a singleton pattern:

```javascript
import Shepherd from 'shepherd.js';

let tourInstance = null;

export function getTourManager() {
  if (!tourInstance) {
    tourInstance = new TourManager();
  }
  return tourInstance;
}
```

### Class Structure

```javascript
class TourManager {
  constructor() {
    this.shepherd = null;   // Shepherd Tour instance
    this.tours = [];        // Cached tour definitions
    this.isLoaded = false;  // Lookup fetch complete
  }

  async loadTours() { /* Fetch from Lookup 43 */ }
  async launchTour(tourId) { /* Build Shepherd tour and start */ }
  showTourList() { /* Open tour selection popup */ }
  endTour() { /* Clean up current tour */ }
}
```

### Shepherd Tour Construction

Each tour definition is converted to a Shepherd tour:

```javascript
function buildShepherdTour(definition) {
  const tour = new Shepherd.Tour({
    useModalOverlay: true,
    defaultStepOptions: {
      cancelIcon: { enabled: false }, // We provide our own close button
      classes: 'lithium-tour-step',
      scrollTo: { behavior: 'smooth', block: 'center' },
    },
  });

  // Add custom buttons to each step
  definition.steps.forEach((stepDef, index) => {
    const buttons = [];

    if (index > 0) {
      buttons.push({ action: tour.back, classes: 'lithium-tour-back', text: 'Back' });
    }
    if (index < definition.steps.length - 1) {
      buttons.push({ action: tour.next, classes: 'lithium-tour-next', text: 'Next' });
    }

    // Tour list button (only on first step)
    if (index === 0) {
      buttons.push({ action: () => this.showTourList(), classes: 'lithium-tour-list', text: '<fa fa-list></fa>' });
    }

    // Close button
    buttons.push({ action: tour.cancel, classes: 'lithium-tour-close', text: '<fa fa-xmark></fa>' });

    tour.addStep({
      title: stepDef.title,
      text: stepDef.text,
      attachTo: stepDef.element ? { element: stepDef.element, on: stepDef.position || 'bottom' } : undefined,
      buttons,
    });
  });

  return tour;
}
```

---

## CSS Architecture

Tour styles use a **monochromatic black/white palette** via tour-specific CSS variables (`--tour-*`). These variables are completely independent of the main Lithium theme, allowing tours to overlay cleanly on any theme without visual conflict.

### Variable Namespace

All tour variables use the `--tour-` prefix. None of the main Lithium theme variables are used. To restyle tours, override these variables:

```css
/* Example: dark tour theme */
:root {
  --tour-bg: #1a1a1a;
  --tour-text: #ffffff;
  --tour-border: #444444;
  --tour-btn-bg: #333333;
  --tour-btn-text: #ffffff;
  /* ... */
}
```

### Variable Reference

| Variable | Default | Purpose |
|----------|---------|---------|
| `--tour-bg` | `#ffffff` | Panel background |
| `--tour-bg-secondary` | `#f5f5f5` | Footer background |
| `--tour-text` | `#000000` | Body text color |
| `--tour-text-muted` | `#666666` | Muted text (counters, manager names) |
| `--tour-border` | `#000000` | Primary border color |
| `--tour-border-light` | `#cccccc` | Light border (list items) |
| `--tour-border-width` | `2px` | Border thickness |
| `--tour-border-radius` | `8px` | Panel corner radius |
| `--tour-bar-bg` | `#000000` | Header bar background |
| `--tour-bar-text` | `#ffffff` | Header bar text |
| `--tour-btn-bg` | `#000000` | Button background |
| `--tour-btn-text` | `#ffffff` | Button text |
| `--tour-btn-hover-bg` | `#333333` | Button hover state |
| `--tour-btn-disabled-bg` | `#cccccc` | Disabled button |
| `--tour-shadow` | `0 8px 24px rgba(0,0,0,0.35)` | Panel drop shadow |
| `--tour-overlay` | `rgba(0,0,0,0.45)` | Modal backdrop |
| `--tour-z-step` | `30000` | Step panel z-index |
| `--tour-z-popup` | `30001` | Tour list popup z-index |

### Key Selectors

| Selector | Purpose |
|----------|---------|
| `.lithium-tour-step .shepherd-element` | Main step panel |
| `.lithium-tour-step .shepherd-header` | Black header bar |
| `.lithium-tour-step .shepherd-text` | Step content |
| `.lithium-tour-step .shepherd-button` | Black navigation buttons |
| `.lithium-tour-step .shepherd-footer` | Light gray button bar |
| `.lithium-tour-list-popup` | Tour selection popup |
| `.lithium-tour-list-item` | Individual tour in list |
| `.shepherd-modal-overlay-class` | Modal backdrop |

See `src/managers/tour/tour.css` for the complete stylesheet.

---

## File Structure

```
src/managers/tour/
├── tour.js          # TourManager class + exports
├── tour.css         # Shepherd theme overrides + tour list popup styles
```

No HTML template is needed — Shepherd generates its own DOM, and the tour list popup is created dynamically (same pattern as Crimson).

---

## Dependencies

### NPM

| Package | Purpose |
|---------|---------|
| `shepherd.js` | Guided tour library |

Import dynamically (heavy library):

```javascript
const shepherdModule = await import('shepherd.js');
const Shepherd = shepherdModule.default;
```

### Internal

| Module | Purpose |
|--------|---------|
| `shared/conduit.js` | `authQuery()` for Lookup #43 fetch |
| `core/log.js` | Logging |
| `core/icons.js` | `processIcons()` for `<fa>` tags |

---

## Related Documentation

| Document | Relevance |
|----------|-----------|
| [LITHIUM-MGR-CRIMSON.md](LITHIUM-MGR-CRIMSON.md) | Crimson can suggest tours via WebSocket responses |
| [LITHIUM-LUT.md](LITHIUM-LUT.md) | Lookup system — how Lookup #43 is structured |
| [LITHIUM-MGR.md](LITHIUM-MGR.md) | Manager system overview, Manager ID registry |
| [LITHIUM-LIB.md](LITHIUM-LIB.md) | Library conventions (dynamic imports) |
| [LITHIUM-CSS.md](LITHIUM-CSS.md) | CSS variables and theming |

---

Last updated: April 2026
