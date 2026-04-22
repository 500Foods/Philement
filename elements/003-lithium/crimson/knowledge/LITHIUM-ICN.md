# Lithium Icon System

Lithium uses a custom icon system built on Font Awesome with dynamic lookup support. The system replaces custom `<fa>` tags with proper Font Awesome elements.

**Location:** [`src/core/icons.js`](elements/003-lithium/src/core/icons.js)

---

## Overview

| Aspect | Details |
|--------|---------|
| **Library** | Font Awesome 6 (Pro) |
| **Approach** | Custom `<fa>` tag replacement |
| **Configuration** | Via `lithium.json` |
| **Dynamic Icons** | Server-driven icon overrides |

---

## Basic Usage

### HTML Template

```html
<!-- Direct icon -->
<fa fa-palette></fa>

<!-- With modifiers -->
<fa fa-user fa-fw></fa>

<!-- Spinning -->
<fa fa-spinner fa-spin></fa>
```

### JavaScript

```javascript
import { processIcons, setIcon } from '../../core/icons.js';

// Process all <fa> elements in container
processIcons(document.body);

// Programmatically set icon
setIcon(element, 'palette', { classes: ['fa-fw'] });
```

---

## How It Works

### 1. The Icon Rendering Pipeline

Icons go through a **three-stage replacement pipeline**:

```text
Stage 1: HTML Template       →  <fa fa-palette></fa>
Stage 2: Lithium processIcons →  <i class="fa-duotone fa-thin fa-palette"></i>
Stage 3: Font Awesome SVG+JS  →  <svg class="svg-inline--fa fa-palette" ...>...</svg>
```

**Stage 1 — HTML authoring:** You write `<fa>` tags in HTML templates. These are custom pseudo-elements that Font Awesome does not recognise.

**Stage 2 — Lithium processing:** The `processIcons()` function (triggered by a MutationObserver on `document.body`) finds all `<fa>` elements and replaces each one with an `<i>` element. The `<i>` receives the correct Font Awesome CSS classes based on the configured icon set (e.g., `fa-duotone fa-thin`). Element attributes like `id`, `data-*`, `aria-*`, `title`, and inline styles are preserved on the new `<i>`.

**Stage 3 — Font Awesome SVG+JS:** Because Lithium uses the Font Awesome **SVG+JS kit** (not the CSS webfont), Font Awesome's own MutationObserver detects the new `<i class="fa-...">` elements and replaces them with inline `<svg>` elements. The `<i>` is removed from the DOM entirely.

> **Important:** After Stage 3, any cached JavaScript reference to the original `<fa>` or `<i>` element is **stale** — it points to a detached DOM node. See [Icon Rotation Pattern](#icon-rotation-pattern) below for the correct way to handle animated icons.

### 2. Why Three Stages?

| Stage | Who | What | Why |
|-------|-----|------|-----|
| 1 | Developer | `<fa fa-user>` | Clean, readable HTML that doesn't depend on a specific FA set or weight |
| 2 | Lithium `icons.js` | `<i class="fa-duotone fa-thin fa-user">` | Applies the project's configured icon set/weight; resolves lookup-based icons |
| 3 | Font Awesome Kit | `<svg ...>` | Renders the actual vector glyph as an inline SVG |

### 3. Configuration

Icons are configured in `config/lithium.json`:

```json
{
  "icons": {
    "set": "duotone",
    "weight": "100",
    "fallback": "solid"
  }
}
```

### 4. Lookup Integration

Icons can be dynamically overridden from server lookups:

```html
<!-- Use lookup icon -->
<fa Status></fa>

<!-- Lookup provides: <i class="fa fa-flag"></i> -->
```

---

## Icon Sets

The system supports all Font Awesome 6 Pro sets:

| Set | Prefix | Example Class |
|------|--------|---------------|
| Solid | `fas` | `fas fa-user` |
| Regular | `far` | `far fa-user` |
| Light | `fal` | `fal fa-user` |
| Thin | `fat` | `fat fa-user` |
| Duotone | `fad` | `fad fa-user` |
| Sharp Solid | `fass` | `fass fa-user` |
| Brands | `fab` | `fab fa-github` |

### Setting Icon Set

```javascript
// Via config
{
  "icons": {
    "set": "duotone",
    "weight": "100"
  }
}
```

---

## Preserved Classes

These classes are preserved from the original `<fa>` tag:

### Size Classes

```html
<fa fa-user fa-xs></fa>
<fa fa-user fa-sm></fa>
<fa fa-user fa-lg></fa>
<fa fa-user fa-2x></fa>
<!-- ... up to fa-10x -->
```

### Animation Classes

```html
<fa fa-spinner fa-spin></fa>
<fa fa-spinner fa-pulse></fa>
<fa fa-bell fa-beat></fa>
<fa fa-bounce fa-bounce></fa>
```

### Utility Classes

```html
<fa fa-fw></fa>        <!-- Fixed width -->
<fa fa-rotate-90></fa> <!-- Rotate 90° -->
<fa fa-flip-horizontal></fa>
<fa fa-inverse></fa>
```

---

## Programmatic Usage

### Creating Icons

```javascript
import { createIcon, setIcon } from '../../core/icons.js';

// Create new icon element
const icon = createIcon('palette', { 
  set: 'duotone',
  weight: '100',
  classes: ['fa-fw']
});
document.body.appendChild(icon);

// Set icon on existing element
setIcon(existingElement, 'user', { classes: ['fa-2x'] });
```

### Configuration

```javascript
import { getIconConfiguration } from '../../core/icons.js';

const config = getIconConfiguration();
// { set: 'duotone', weight: '100', prefix: 'fad', fallback: 'solid' }
```

---

## Icon Configuration

Icons are configured in `config/lithium.json`:

```json
{
  "icons": {
    "set": "duotone",
    "weight": "thin",
    "prefix": "fa-duotone fa-thin",
    "fallback": "solid"
  }
}
```

| Property | Type | Description |
|----------|------|-------------|
| `set` | string | Font Awesome set name |
| `weight` | string | Font weight for compatible sets |
| `prefix` | string | Full CSS prefix (overrides set/weight) |
| `fallback` | string | Fallback set if primary unavailable |

### Example Configurations

```json
// Duotone thin icons (current)
{ "icons": { "set": "duotone", "weight": "thin", "prefix": "fa-duotone fa-thin" } }

// Solid icons
{ "icons": { "set": "solid", "fallback": "regular" } }

// Sharp Solid
{ "icons": { "set": "sharp-solid", "weight": "100" } }
```

---

## Lookup Integration

### Server-Side Icons

QueryRef 054 provides server-driven icon overrides:

```javascript
// In lookups.js - icons category
{
  "icons": [
    { "lookup_value": "Status", "icon": "<i class='fa fa-fw fa-flag'></i>" },
    { "lookup_value": "Error", "icon": "<i class='fa fa-fw fa-xmark'></i>" }
  ]
}
```

### Usage

```html
<!-- Will use lookup icon if available -->
<fa Status></fa>
```

---

## Events

The icon system listens for events to refresh icon data:

| Event | Action |
|-------|--------|
| `lookups:loaded` | Refresh icon lookups |
| `lookups:icons:loaded` | Refresh icon lookups and reprocess |

```javascript
import { eventBus, Events } from '../../core/event-bus.js';

eventBus.on(Events.LOOKUPS_LOADED, () => {
  // Icons will automatically refresh
});
```

---

## Best Practices

### 1. Use Semantic Names

```html
<!-- Good -->
<fa fa-palette></fa>
<fa fa-user-cog></fa>

<!-- Avoid -->
<fa fa-p></fa>
<fa fa-uc"></fa>
```

### 2. Prefer CSS Classes Over Attributes

```html
<!-- Good -->
<fa fa-spinner fa-spin></fa>

<!-- Works but less clear -->
<fa spinner spin></fa>
```

### 3. Use Fixed Width for Alignment

```html
<!-- Aligned icons -->
<fa fa-user fa-fw></fa>
<fa fa-cog fa-fw></fa>
```

### 4. Accessibly

```html
<!-- With title for accessibility -->
<fa fa-palette title="Open theme settings"></fa>

<!-- Or aria-label -->
<fa fa-search aria-label="Search"></fa>
```

---

## Testing

### Manual Testing

```javascript
import { processIcons } from '../../core/icons.js';

// Process any new icons after dynamic content
processIcons(container);

// Check icon configuration
const config = getIconConfiguration();
console.log('Icon set:', config.set);
```

## Dynamic Table HTML

Lithium tables have one critical icon rule:

- `processIcons()` and the MutationObserver only work when the DOM contains real `<fa>` tags.
- If a data source provides escaped markup such as `&lt;fa fa-user&gt;&lt;/fa&gt;`, the table cell will render literal text and no later icon-processing pass can recover it.

Use the shared normalizer before storing or rendering icon HTML from server data, lookups, or cached menu payloads:

```javascript
import { normalizeIconHtml } from '../../core/icons.js';

const safeIconHtml = normalizeIconHtml(rawIconHtml, '<fa fa-cube></fa>');
```

`normalizeIconHtml()` does two things:

- decodes HTML entities like `&lt;fa ...&gt;`
- closes bare `<fa ...>` tags so they become `<fa ...></fa>`

This is the correct fix for cold-load cases where icon markup sometimes arrives escaped from cache or server JSON.

---

## Icon Rotation Pattern

Because the three-stage pipeline replaces DOM elements, any cached reference to a `<fa>` or `<i>` tag becomes **stale** once Font Awesome finishes. Lithium provides two approaches for handling icon rotation:

### Approach 1: CSS-Based Rotation (Recommended)

For collapse buttons and panel toggles, use CSS classes to control rotation. This approach is used by the shared `panel-collapse.js` utility:

```html
<button class="lithium-collapse-btn" id="my-collapse-btn" data-collapse-target="left">
  <fa fa-angles-left id="my-collapse-icon" class="lithium-collapse-icon"></fa>
</button>
```

```javascript
// Toggle rotation via CSS class
collapseBtn.classList.toggle('collapsed', isCollapsed);
```

```css
/* CSS handles the animation using --transition-duration */
.lithium-collapse-icon,
.lithium-collapse-btn > i,
.lithium-collapse-btn > svg {
  transition: transform var(--transition-duration, 350ms) ease;
  transform: rotate(0deg);
}

.lithium-collapse-btn.collapsed .lithium-collapse-icon,
.lithium-collapse-btn.collapsed > i,
.lithium-collapse-btn.collapsed > svg {
  transform: rotate(180deg);
}
```

> **Why this works:** The CSS targets both `<i>` and `<svg>` elements, so it works regardless of which stage the Font Awesome pipeline has reached. The `.collapsed` class on the button triggers the rotation via descendant selectors.

### Approach 2: JavaScript DOM Query (For Custom Animations)

For cases where you need programmatic control (e.g., accumulating rotation angles), query the DOM at the moment you need the icon element:

```javascript
// ✅ Fresh query each time, works regardless of FA stage
_getMyIconEl() {
  return document.querySelector('#my-icon') ||
         document.querySelector('#my-button')?.firstElementChild;
}

// On click handler
toggleSomething() {
  const iconEl = this._getMyIconEl();
  if (iconEl) {
    this._rotation += 180;
    iconEl.style.transform = `rotate(${this._rotation}deg)`;
  }
}
```

### CSS Setup for Custom Animations

For smooth rotation with custom JS logic, add a CSS transition targeting the icon by ID **and** by possible tag types:

```css
#my-icon,
#my-button > i,
#my-button > svg {
  transition: transform var(--transition-duration, 350ms) ease;
}
```

### Approach 3: Prior-State Pinning (For Host-Regenerated Icons)

Use this approach when the **host widget tears down and rebuilds the icon's DOM** on every state change. The canonical case is Tabulator's group header — every time a group is expanded or collapsed, Tabulator clears the header's child nodes and rebuilds them from the `groupHeader()` callback's HTML string, inserting a brand-new `<fa>` element.

Approach 1 (CSS-only) **fails** here because the new element has no prior computed style to transition from — it appears already in its final rotation and snaps with no animation.

Approach 2 (store-and-apply-transform) **fails** here for the same reason: even if JS caches the previous rotation, the element that rotation was applied to no longer exists.

The working pattern: **remember state in JS, pin the reborn icon to its PRIOR rotation for exactly one frame, then release it so CSS transitions to the new rotation.**

```javascript
// 1. Stable state storage (keyed by something that survives DOM rebuilds —
//    a path of the group's keys, a row id, etc.)
this._iconState = new Map();            // key -> boolean (prior visibility)
this._iconAnimating = new Set();        // HTMLElement -> in-flight guard

// 2. Run after the host widget has finished regenerating. Defer one rAF
//    so the new DOM is attached.
syncIcons() {
  requestAnimationFrame(() => this._syncNow());
}

_syncNow() {
  const toAnimate = [];
  for (const item of this._getHostItems()) {
    const key = this._keyOf(item);
    const rowEl = item.getElement();
    const isVisible = rowEl.classList.contains('host-visible-class');

    // CRITICAL: never touch a row that's already mid-animation.
    // Overlapping host events (renderComplete, tableRedrawn…) fire for a
    // single state change; stripping the anim class off a pinned row
    // cancels the transition before the browser paints the pinned frame.
    if (this._iconAnimating.has(rowEl)) {
      this._iconState.set(key, isVisible);   // still advance bookkeeping
      continue;
    }

    // Clean up any stale anim classes on non-animating rows.
    rowEl.classList.remove('anim-from-collapsed', 'anim-from-expanded');

    const hadPrior = this._iconState.has(key);
    const wasVisible = this._iconState.get(key);
    this._iconState.set(key, isVisible);

    // First time seeing this item OR no state change → no animation.
    if (!hadPrior || wasVisible === isVisible) continue;

    toAnimate.push({ rowEl, fromVisible: wasVisible });
  }

  if (toAnimate.length === 0) return;

  // 3. Pin each row to its PRIOR rotation and mark it animating.
  for (const { rowEl, fromVisible } of toAnimate) {
    this._iconAnimating.add(rowEl);
    rowEl.classList.add(
      fromVisible ? 'anim-from-expanded' : 'anim-from-collapsed'
    );
  }

  // 4. Force a layout+paint of the pinned state. Without this the browser
  //    can coalesce the class-add and the class-remove into a single
  //    paint, so the "from" state never gets committed.
  void this._container.offsetHeight;

  // 5. Next frame: release the pin. Normal CSS rule re-takes over,
  //    transition property is restored, icon animates to the new state.
  requestAnimationFrame(() => {
    for (const { rowEl } of toAnimate) {
      rowEl.classList.remove('anim-from-collapsed', 'anim-from-expanded');
      this._iconAnimating.delete(rowEl);
    }
  });
}
```

**CSS — two starting-position classes overlaid on the normal state rules:**

```css
/* Normal state rules (Approach 1 style). These define the FINAL rotation
   for each state and the transition. */
.host-row .icon             { transform: rotate(0deg);  transition: transform 350ms ease; }
.host-row.host-visible-class .icon { transform: rotate(90deg); }

/* Starting-position classes — added briefly by JS. Same specificity as
   above; source order must place them AFTER the normal rules so they
   win when present. */
.host-row.anim-from-collapsed .icon {
  transform: rotate(0deg);   /* was-closed starting point */
  transition: none;
}
.host-row.anim-from-expanded .icon {
  transform: rotate(90deg);  /* was-open starting point */
  transition: none;
}
```

> **The five traps that make this pattern look broken:**
>
> 1. **Not deferring with `requestAnimationFrame`** — the DOM hasn't been regenerated yet.
> 2. **Skipping the forced reflow** (`void container.offsetHeight`) — the browser coalesces add+remove into one paint and no transition fires.
> 3. **No `_iconAnimating` guard** — overlapping syncs from the host's other events (`renderComplete`, `tableRedrawn`, `dataLoaded` etc.) strip the anim class off a pinned row between the pin frame and the release frame.
> 4. **Inline `style.transform`** — works but is fragile when the FA pipeline replaces the icon element between the pin and release; class-based pinning on the **stable parent** cascades to whichever child (`<fa>`, `<i>`, or `<svg>`) currently exists.
> 5. **Lower specificity on the anim-from-* rules** — if the host's "visible" state rule wins, the pinning has no effect. Match the specificity and rely on source order.

**Canonical implementation:** [`src/tables/lithium-table-base.js`](../../elements/003-lithium/src/tables/lithium-table-base.js) — `_syncGroupIconsNow()` and the `_groupVisibilityState` / `_groupAnimating` fields. **CSS:** [`src/styles/vendor-fixes.css`](../../elements/003-lithium/src/styles/vendor-fixes.css) — the `.li-group-anim-from-collapsed` and `.li-group-anim-from-expanded` rules alongside the `.tabulator-group-visible` rule. **Do not remove or simplify these without first re-reading this section** — they are the product of several failed attempts and the guards exist for concrete reasons.

### Existing Examples

| Location | Element | Pattern |
|----------|---------|---------|
| `panel-collapse.js` | Panel collapse buttons | **Approach 1** — CSS class `.collapsed` toggles rotation (all managers) |
| `main.js` | Sidebar collapse arrow | **Approach 2** — `_getArrowEl()` queries `#collapse-icon` or button's `firstElementChild` |
| `toast.css` | Expand chevron rotation | **Approach 1** — CSS targets both `i` and `svg` for smooth 180° rotation |
| `lithium-table-base.js` | Tabulator group expand/collapse arrow | **Approach 3** — prior-state pinning; Tabulator regenerates the header on every toggle |

### Panel Collapse Buttons

All LithiumTable managers use the shared `panel-collapse.js` utility for panel collapse/expand with animated icons. See [LITHIUM-TAB.md](LITHIUM-TAB.md#collapsible-panels-with-animated-icons) for the complete implementation pattern including:

- HTML structure with `data-collapse-target` attributes
- CSS classes (`lithium-collapse-btn`, `lithium-collapse-icon`)
- JavaScript integration using `togglePanelCollapse()` and `restorePanelState()`

---

## Related Documentation

- [LITHIUM-OTH.md](LITHIUM-OTH.md) - Other utilities
- [LITHIUM-MGR-LOGIN.md](LITHIUM-MGR-LOGIN.md) - Login Manager (uses icons extensively)
- [LITHIUM-LIB.md](LITHIUM-LIB.md) - Libraries
