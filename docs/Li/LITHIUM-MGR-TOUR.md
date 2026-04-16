# Tour Manager

The Tour Manager provides in-app guided tours using [Shepherd.js](https://shepherdjs.dev/). Tours are stored in **Lookup #43** (Tours) and are lazy-loaded on first use.

**Location:** `src/managers/tour/`

**Manager ID:** 6 (System Manager, Group 0, hidden from main menu)

**Type:** Utility / Popup Manager (not a traditional slot-based manager)

---

## Overview

| Aspect | Details |
|--------|---------|
| **Library** | Shepherd.js (dynamic import, bundled separately) |
| **Data Source** | Lookup #43 — lazy-loaded on first tour request via QueryRef 26 |
| **Activation** | Tours button in manager footer, or Crimson suggestions |
| **Key Files** | `tour.js`, `tour.css` |
| **Z-Index** | 40000 (steps), 40001 (tour list popup) — above all UI |
| **Panel Width** | 500px fixed |

The Tour Manager is a singleton that wraps Shepherd.js. It does not occupy a workspace slot — tours display as modal overlays with step-by-step highlighting of UI elements. The design is monochromatic black/white to overlay cleanly on any theme.

---

## Lookup #43: Tours Schema

Each tour is a row in Lookup #43 with the tour definition stored in the `collection` JSON column.

**Important:** The `collection` field is stored as a JSON **string**, not an object. It must be parsed before use.

### Tour JSON Structure

```json
{
  "icon": "<fa fa-clipboard-list></fa>",
  "manager": "023.Lookup Manager",
  "sidebar": true,
  "steps": [
    {
      "text": "This Module is used to view or edit various app Lookups.",
      "title": "Lookups Module",
      "element": "#divLookupsHolder",
      "position": "right"
    }
  ]
}
```

### Top-Level Fields

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `icon` | `string` | Yes | Font Awesome `<fa>` tag for the tour's icon (shown in tour header and list). See [LITHIUM-ICN.md](LITHIUM-ICN.md) for icon format. |
| `manager` | `string` | No | Single manager identifier using lithium.json naming convention (e.g., `"023.Lookup Manager"`). For a single manager target. |
| `managers` | `array` | No | Array of manager identifiers for multiple managers (e.g., `["029.Query Manager", "023.Lookup Manager"]`). Each entry uses the same convention as `manager`. When this field is present, the tour appears in the Tours list for all listed managers. |
| `sidebar` | `boolean` | No | If `true`, expands the sidebar if it's collapsed. Use for tours that reference sidebar elements. Default: `false` (no action). |
| `steps` | `array` | No* | Array of step objects. Required for step-based tours, mutually exclusive with `video`. |
| `video` | `string` | No* | Path to MP4 video file (e.g., `"/assets/videos/scarlett-en-US-welcome-to-lithium.mp4"`). Required for video tours, mutually exclusive with `steps`. |

**Note:** Tours must have either `steps` (step-based tour) OR `video` (video tour), but not both.

### Step Object Fields

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `title` | `string` | No | Step heading (not displayed — tour name is used in header instead) |
| `text` | `string` | No | Step content (supports HTML) |
| `element` | `string` | No | CSS selector for the target element. If omitted or element not found, step displays **centered** without modal mask. |
| `position` | `string` | No | Placement: `"top"`, `"bottom"`, `"left"`, `"right"`, `"auto"`. Defaults to `"bottom"`. |

### Video Tours

Video tours play MP4 videos in a custom styled popup player instead of step-by-step guided tours.

**Example Video Tour JSON:**

```json
{
  "icon": "<fa fa-play></fa>",
  "managers": ["029.Query Manager", "023.Lookup Manager"],
  "video": "/assets/videos/scarlett-en-US-welcome-to-lithium.mp4",
  "captions": [
    ["0.000", "2.500", "Hello and welcome to Lithium"],
    ["2.500", "5.000", "Our web application"]
  ]
}
```

#### Optional: Captions

Video tours can include captions stored in the `captions` field. This is an array of triples `[startTime, endTime, text]` where times are in seconds (can use leading zeros like "000.500"):

| Field | Type | Description |
|-------|------|-------------|
| `captions` | `array` | Array of `[startSeconds, endSeconds, "text"]` triples |

#### Video Tour Features

- **Custom player** — Themed to match the Lithium design system (similar to Terminal popup styling)
- **Draggable** — Click and drag the header to reposition
- **Resizable** — Corner handles for resizing with aspect ratio preservation
  - Top-left: bottom-right stays fixed
  - Top-right: bottom-left stays fixed
  - Bottom-left: top-right stays fixed
  - Bottom-right: top-left stays fixed
- **State persistence** — Popup position, size, volume, mute, playback speed, and captions toggle are saved to localStorage and restored on next tour launch
- **Custom controls:**
  - Overlay buttons in 8-position compass layout:
    - Top: Subtitles toggle
    - Top-right: Forward 10s
    - Right: Forward to end
    - Bottom-right: Forward 30s
    - Bottom: Repeat toggle
    - Bottom-left: Back 30s
    - Left: Back to start
    - Top-left: Back 10s
  - Center: Play/Pause button
  - Scrubber for seeking
  - Time display (current / duration)
  - Volume slider + mute toggle
  - Playback speed selector (0.5x, 0.75x, 1x, 1.25x, 1.5x, 2x)
- **Keyboard shortcuts:**
  - `Space` — Play/Pause
  - `←` — Back 10 seconds (`Shift+←` for 30s)
  - `→` — Forward 10 seconds (`Shift+→` for 30s)
  - `R` — Toggle repeat
  - `S` — Toggle subtitles
  - `Esc` — Close popup
- **Caption transitions** — 0.5s fade in/out with crossfade support for overlapping timestamps
- **Initial positioning** — Centered in viewport, or restored from saved position
- **Overlay visibility:**
  - Hidden when video is playing and mouse is not hovering
  - Shown when video is paused (and not at end)
  - Shown when video has ended
  - Shown on hover

### Manager Field Convention

The `manager` field follows the lithium.json naming convention:

```
"###.Manager Name"
```

| Value | Manager Type | Notes |
|-------|--------------|-------|
| `"003.Profile"` | **Utility Manager** | User profile page (sidebar footer) |
| `"004.Session Log"` | **Utility Manager** | Session log popup (sidebar footer) |
| `"023.Lookup Manager"` | **Main Menu Manager** | Standard slot-based manager |
| `"029.Query Manager"` | **Main Menu Manager** | Standard slot-based manager |

**Utility Managers (001-006):** These are sidebar footer utilities, not main menu items. The tour manager:
1. Extracts the numeric ID from the manager field
2. Looks up in `utilityManagerRegistry` by ID
3. Calls `app.loadUtilityManager(id)` to activate

**Main Menu Managers (007+):** These are regular managers that appear in the sidebar menu. The tour manager:
1. Extracts the numeric ID from the manager field  
2. Looks up in `managerRegistry` by ID
3. Calls `app.loadManager(id)` to activate

If no `manager` field is specified, the tour launches without switching managers.

---

## Lazy Loading Strategy

Tour data is **not** loaded at startup. Lookup #43 is fetched only when a user requests a tour.

```javascript
async function getTours(api) {
  if (_toursCache) return _toursCache;

  // QueryRef 26 with LOOKUPID 43
  const entries = await authQuery(api, 26, {
    INTEGER: { LOOKUPID: 43 }
  });

  _toursCache = entries
    .filter(e => e.collection)
    .map(e => ({
      id: e.key_idx,        // Numeric tour ID (key_idx from lookup)
      name: e.value_txt,     // Display name
      code: e.code || String(e.key_idx),  // Support lookup by code
      // collection is a JSON STRING — must parse it
      definition: typeof e.collection === 'string'
        ? JSON.parse(e.collection)
        : e.collection,
    }));

  return _toursCache;
}
```

Tour lookup supports multiple identifiers:
- **Numeric ID:** `launchTour(3, api)` — matches `key_idx`
- **Code:** `launchTour("welcome", api)` — matches `code` field
- **Name:** `launchTour("Welcome to Lithium", api)` — matches `value_txt`

Use `refreshTours(api)` to force a re-fetch if needed.

---

## Tour UI Controls

### Layout

```
┌────────────────────────────────────────────────┐
│  [icon] Tour Name                  [↗] [✕]   │  ← Header (black bg, white text)
├────────────────────────────────────────────────┤
│                                                │
│  Step content goes here with HTML support.     │  ← Content (white bg, black text)
│  <b>Bold</b>, <i>italic</i>, etc.              │
│                                                │
├────────────────────────────────────────────────┤
│  [<]                      3 / 15          [>]  │  ← Footer (gray bg, black buttons)
└────────────────────────────────────────────────┘
```

### Header

The header displays:
- **Tour icon** (from `definition.icon`, left side)
- **Tour name** (from `value_txt`)
- **Tour List button** (`<fa fa-signs-post></fa>`) — ends tour, shows tour list
- **Close button** (`<fa fa-xmark></fa>`) — ends tour

### Footer Buttons

| Button | Icon | Position | Action |
|--------|------|----------|--------|
| **Back** | `<fa fa-chevron-left></fa>` | Left | Go to previous step (disabled on first step) |
| **Step Counter** | `3 / 15` | Center | Shows current step / total steps |
| **Next** | `<fa fa-chevron-right></fa>` | Right | Go to next step |
| **Done** | `<fa fa-check></fa>` | Right | Complete tour (replaces Next on last step) |

**Icons:** All icons use the `<fa>` tag format per [LITHIUM-ICN.md](LITHIUM-ICN.md).

### Keyboard Controls

| Key | Action |
|-----|--------|
| `←` (Left Arrow) | Previous step |
| `→` (Right Arrow) | Next step |
| `Esc` | End tour |

### Transitions

Steps fade in/out using CSS transitions:

- **Fade in:** `opacity: 0 → 1` (pure opacity, no transform animation)
- **Duration:** Controlled by `--transition-duration` CSS variable
- **Between steps:** Previous step fades out, then next step fades in
- **Step element:** May require retry logic to find DOM element (async rendering)
- **Tour end:** Last step fades out before Shepherd cleanup occurs

The CSS selector is `.shepherd-element.lithium-tour-step` (both classes on same element).

**Important CSS note:** Because Shepherd's un-layered CSS (`.shepherd-enabled.shepherd-element { opacity: 1 }`) overrides `@layer` styles, the tour CSS uses `!important` on `opacity` and `visibility` to maintain control. The `.lithium-tour-visible` class is added in a double-`requestAnimationFrame` callback after Floating UI has positioned the element, preventing any visible position jump.

---

## Tour List Popup

```
┌─────────────────────────────────────────────┐
│  Recommended Tours                   [✕]   │  ← Header (black bg)
├─────────────────────────────────────────────┤
│  <icon> Welcome to Lithium                  │
│  <icon> AI Auditor Module 1.0               │  ← Clickable items
│  <icon> Lookups Tour                        │
│  ...                                        │
└─────────────────────────────────────────────┘
```

Each item shows:
- Tour icon (from `definition.icon`)
- Tour name (from `value_txt`)

The list uses **numeric IDs** internally (from `key_idx`) but displays only friendly names to users.

Selecting a tour:
1. Closes the tour list popup
2. Expands sidebar if collapsed (for tours that reference sidebar elements)
3. Switches to the tour's target manager (if specified)
4. Launches the selected tour

---

## Element Targeting Behavior

| Scenario | Result |
|----------|--------|
| Element found | Highlights element with modal overlay |
| Element not found | Step displays **centered**, no modal mask |
| No element specified | Step displays **centered**, no modal mask |

This allows informational steps (e.g., "Welcome to the Lookups Manager") to float in the center of the screen.

**Note:** After manager switching, there is a 300ms delay + 600ms settle time to ensure the DOM is ready before the tour starts.

---

## Integration Points

### Manager Footer Tours Button

Each manager's footer includes a Tours button. Clicking it:

```javascript
async _handleTours() {
  const managerName = '023.Lookup Manager';
  const tours = await getTours(this.app.api);

  // Filter tours for this manager
  const managerTours = tours.filter(t =>
    t.definition.manager === managerName
  );

  if (managerTours.length === 1) {
    launchTour(managerTours[0].id, api, { anchor: toursBtn });
  } else {
    showTourList(managerTours, toursBtn, {
      onSelect: (id) => launchTour(id, api)
    });
  }
}
```

### Fallback Behavior

| Scenario | Behavior |
|----------|----------|
| One tour matches manager | Launch directly |
| Multiple tours match manager | Show filtered tour list ("Recommended Tours") |
| No tours match manager | Show all available tours |
| Tour ID not found | Show all available tours |
| Tours not loaded | Fetch from Lookup #43, then proceed |

---

## Architecture

### Key Exports

```javascript
// Tour loading
getTours(api)              // Get all tours (cached)
getToursForManager(api, managerName)  // Get tours for specific manager
refreshTours(api)          // Force reload from Lookup #43

// Tour execution
launchTour(tourId, api, options)  // Launch a tour by ID, code, or name
showTourList(tours, anchor, options)  // Show tour selection popup
hideTourList()             // Close the popup
buildShepherdTour(tour, options)  // Build Shepherd instance from tour JSON

// Manager UI integration (from manager-ui.js)
setManagerTours(managerId, tours)  // Set tours for a manager (with icons)
loadManagerTours(managerId)        // Load tours from Lookup #43 for a manager
```

### Manager Footer Tours Button

The Tours button in each manager's footer uses the `manager-ui.js` module. When clicked, it:
1. Calls `loadManagerTours(managerId)` to fetch tours from Lookup #43 (if not already cached)
2. Displays a popup with tour names and their icons from the tour's `definition.icon`
3. Launches the selected tour via `launchTour()`

The popup shows each tour with its unique icon (e.g., `<fa fa-clipboard-list></fa>`) instead of the stock tour icon.

### Build Shepherd Tour

The `buildShepherdTour()` function converts tour JSON to a Shepherd tour:

1. Creates `Shepherd.Tour` with `useModalOverlay: true`
2. Custom header injected into each step's `text` (replaces Shepherd header)
3. Custom footer with Back/Counter/Next buttons injected into each step's `text`
4. Document-level click handler captures button clicks via event delegation
5. Keyboard navigation set up on tour start
6. Fade-in class added on step show (with retry logic for element detection)

### Button Click Handling

Uses **event delegation** at the document level (capture phase) for reliability:

```javascript
document.addEventListener('click', (e) => {
  const button = e.target.closest('[data-tour-action]');
  if (!button) return;

  // Verify click is within active step
  const stepEl = shepherd.getCurrentStep()?.getElement();
  if (!stepEl || !stepEl.contains(button)) return;

  // Handle action: close, list, back, next
}, true); // Capture phase
```

This ensures clicks are captured even with Shepherd's DOM manipulation.

### Helper Functions

| Function | Purpose |
|----------|---------|
| `createHeaderHTML(tour, options)` | Generates header with icon, title, signs-post, xmark |
| `createFooterHTML(currentStep, totalSteps)` | Generates footer with back, counter, next |
| `setupDocumentClickHandler(shepherd, totalSteps, options)` | Document-level click delegation |
| `setupKeyboardNav(shepherd, totalSteps)` | Sets up ←, →, Esc keyboard handlers |
| `switchToManager(managerName)` | Opens target manager (handles both main and utility) |
| `expandSidebarIfCollapsed()` | Expands sidebar if needed for tour visibility |
| `findUtilityKeyById(id)` | Maps numeric ID to utility manager key |
| `getTransitionDuration()` | Reads CSS transition duration for timing |
| `cleanupActiveTour()` | Force-removes Shepherd DOM artefacts (overlay, step elements) |
| `cancelWithTransition(shepherd)` | Fades out current step, then calls `shepherd.cancel()` |
| `completeWithTransition(shepherd)` | Fades out current step, then calls `shepherd.complete()` |
| `navigateWithTransition(shepherd, dir)` | Fades out current step, then navigates next/back |

---

## CSS Architecture

Tour styles use **tour-specific CSS variables** (`--tour-*`) that are independent of the Lithium theme. Default values create a monochromatic black/white design.

### Variable Reference

| Variable | Default | Purpose |
|----------|---------|---------|
| `--tour-bg` | `#ffffff` | Panel background |
| `--tour-bg-secondary` | `#f5f5f5` | Footer background |
| `--tour-text` | `#000000` | Body text color |
| `--tour-text-muted` | `#666666` | Muted text (counters) |
| `--tour-border` | `#000000` | Primary border |
| `--tour-border-light` | `#cccccc` | Light border (list items) |
| `--tour-border-width` | `2px` | Border thickness |
| `--tour-border-radius` | `8px` | Panel corner radius |
| `--tour-header-bg` | `#000000` | Header background |
| `--tour-header-text` | `#ffffff` | Header text |
| `--tour-btn-bg` | `#000000` | Button background |
| `--tour-btn-text` | `#ffffff` | Button text |
| `--tour-btn-hover-bg` | `#333333` | Button hover |
| `--tour-btn-disabled-bg` | `#cccccc` | Disabled button |
| `--tour-shadow` | `0 8px 24px rgba(0,0,0,0.35)` | Drop shadow |
| `--tour-overlay` | `rgba(0,0,0,0.45)` | Modal backdrop |
| `--tour-z-step` | `40000` | Step z-index |
| `--tour-z-popup` | `40001` | List popup z-index |
| `--tour-max-width` | `500px` | Panel width (fixed) |
| `--tour-step-delay` | `0.15s` | Delay between step transitions |

**Note:** Transitions use `--transition-duration` from the main Lithium theme for consistency.

### Key Selectors

**Important:** The CSS uses combined selectors (`.class1.class2`) not descendant selectors (`.class1 .class2`) because Shepherd adds both classes to the same element:

```css
/* Correct - both classes on same element */
.shepherd-element.lithium-tour-step { ... }
.shepherd-element.lithium-tour-step.lithium-tour-visible { ... }

/* Incorrect - this looks for descendant */
.lithium-tour-step .shepherd-element { ... }
```

| Selector | Purpose |
|----------|---------|
| `.shepherd-element.lithium-tour-step` | Main panel (with fade transition) |
| `.shepherd-element.lithium-tour-step.lithium-tour-visible` | Visible state |
| `.lithium-tour-header` | Custom header bar |
| `.lithium-tour-header-icon-title` | Flex container for icon + title |
| `.lithium-tour-header-btn` | Header buttons (signs-post, xmark) |
| `.lithium-tour-content` | Step content area |
| `.lithium-tour-footer` | Custom footer bar |
| `.lithium-tour-footer-btn` | Footer buttons (back, next) |
| `.lithium-tour-step-counter` | Step counter display |
| `.lithium-tour-list-popup` | Tour selection popup |
| `.lithium-tour-list-item` | Tour item in list |
| `.shepherd-modal-overlay-class` | Modal backdrop |

---

## File Structure

```
src/managers/tour/
├── tour.js       # Tour functions: getTours, launchTour, buildShepherdTour, etc.
├── tour.css      # Monochromatic Shepherd theme + tour list popup styles
```

No HTML template needed — Shepherd generates its own DOM, tour list popup is created dynamically.

---

## Implementation Notes

### Step Element Detection

Shepherd creates step elements asynchronously. The 'show' event may fire before the element is in the DOM. The implementation uses retry logic:

```javascript
const processStep = (retryCount = 0) => {
  const currentStepEl = step?.getElement();

  if (!currentStepEl && retryCount < 5) {
    setTimeout(() => processStep(retryCount + 1), 50);
    return;
  }
  // Process icons, add visible class, etc.
};
```

### Icon Processing

Icons use the Lithium `<fa>` tag system per [LITHIUM-ICN.md](LITHIUM-ICN.md):

1. HTML contains `<fa fa-icon-name></fa>` tags
2. `processIcons()` converts to Font Awesome `<i>` elements
3. Font Awesome SVG+JS kit replaces with inline SVG

### Manager Switching Flow

1. If `definition.sidebar === true`, expand sidebar via `expandSidebarIfCollapsed()`
2. Parse manager ID from tour JSON (e.g., "003.profile" → 3)
3. Check `utilityManagerRegistry` first (IDs 1-6)
4. Fall back to `managerRegistry` (IDs 7+)
5. Call appropriate loader (`loadUtilityManager` or `loadManager`)
6. Wait 900ms total for UI to settle (300ms + 600ms)
7. Build and start the tour

---

## Dependencies

### NPM

| Package | Version | Purpose |
|---------|---------|---------|
| `shepherd.js` | `^15.2.2` | Guided tour library (dynamic import) |

### Internal

| Module | Exports Used |
|--------|--------------|
| `shared/conduit.js` | `authQuery()` |
| `core/log.js` | `log()`, `Subsystems`, `Status` |
| `core/event-bus.js` | `eventBus` |
| `core/icons.js` | `processIcons()` |

---

## Video Tour Implementation Notes

### Size Calculation

Video tour popup dimensions are calculated from video metadata plus overhead for header/scrubber/controls:

- **Video aspect ratio** — Detected from video metadata (`videoWidth / videoHeight`)
- **Default aspect ratio** — 2:3 (vertical video) with 92px overhead for header (~44px), scrubber row (~32px), and controls (~16px)
- **Minimum dimensions** — 450px width, calculated height (~767px)
- **State restoration** — Saved dimensions are validated with explicit type checks (`typeof === 'number'`) before use

### Resize Handle Behavior

Each corner resize handle maintains the opposite corner's position:

| Handle | Fixed Corner |
|--------|---------------|
| Bottom-right (br) | Top-left |
| Top-right (tr) | Bottom-left |
| Bottom-left (bl) | Top-right |
| Top-left (tl) | Bottom-right |

### Caption Transitions

Caption fade animations require proper DOM timing:

1. **Fade in:** Element added to DOM (opacity: 0), force reflow (`void element.offsetWidth`), then add `.visible` class to trigger CSS transition
2. **Fade out:** Remove `.visible` class, wait for transition duration (500ms), then remove element from DOM
3. **Overlapping captions:** Use `position: absolute` on captions within a relative container to allow overlap for crossfade effect

### State Persistence

Video tour state is saved to localStorage with these keys:

```
lithium_video_captions_enabled
lithium_video_playback_speed
lithium_video_volume
lithium_video_muted
lithium_video_x
lithium_video_y
lithium_video_width
lithium_video_height
```

### Repeat Behavior

When the repeat toggle is enabled:
- Video loops indefinitely until:
  - The toggle is turned off
  - The video is paused
  - The popup is closed

When the repeat toggle is off:
- Video plays once and stops at the end

**Important:** When saving, validate all numeric values to prevent NaN/Infinity. When loading, use explicit type checks (`typeof === 'number' && > 0`) rather than truthy checks.

### Event Handler Best Practices

When adding click listeners to elements inside a container that also has a click handler, use `e.stopPropagation()` to prevent event bubbling issues. For example, the speed button click handler should stop propagation to avoid being caught by the popup's delegated click handler.

---

## Related Documentation

| Document | Relevance |
|----------|-----------|
| [LITHIUM-ICN.md](LITHIUM-ICN.md) | Icon system — `<fa>` tag format |
| [LITHIUM-MGR-CRIMSON.md](LITHIUM-MGR-CRIMSON.md) | Crimson can suggest tours |
| [LITHIUM-LUT.md](LITHIUM-LUT.md) | Lookup system (Lookup #43) |
| [LITHIUM-MGR.md](LITHIUM-MGR.md) | Manager system, ID registry |
| [LITHIUM-LIB.md](LITHIUM-LIB.md) | Library conventions |

---

Last updated: April 13, 2026
