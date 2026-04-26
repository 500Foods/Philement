# OverlayScrollbars Integration (Lithium OSB)

This document records the current OverlayScrollbars state in Lithium, what is working, what was attempted for Tabulator, what failed, and what must be true before the next attempt.

Current status:

- **Tabulator / LithiumTable**: OverlayScrollbars enabled by default for all tables
- **CodeMirror**: OverlayScrollbars active
- **SunEditor**: OverlayScrollbars active
- **Popups / dropdowns**: OverlayScrollbars active
- **Tabulator + OverlayScrollbars**: rolled out to all LithiumTable instances

Important:

- Tabulator + OverlayScrollbars is **not considered impossible**.
- It is **not yet production-ready** in Lithium.
- The current probe now produces generated OSB chrome, themed custom properties, and preserved scrolling on the test table.
- The current work should be treated as a validated narrow probe, not as proof that all Tabulator tables are safe to convert.

---

## Current Baseline

As of April 21, 2026:

- All LithiumTable instances use OverlayScrollbars by default
- Individual tables can opt out with `useOverlayScrollbars: false`
- The Date Formats token table was the original probe that validated the implementation
- OverlayScrollbars base CSS is loaded through a dedicated vendor layer
- OverlayScrollbars theme CSS remains in Lithium's `vendor-fixes` layer
- `LithiumTable` has `useOverlayScrollbars` option (defaults to true)

This is the current default behavior after probe validation.

---

## Files

| File | Purpose |
|------|---------|
| `src/core/scrollbar-manager.js` | Centralized OverlayScrollbars initialization and presets |
| `src/styles/vendor-overlayscrollbars.css` | Imports OSB base CSS into a lower vendor layer |
| `src/styles/scrollbars.css` | Lithium OSB theme rules and OSB-related styling |
| `src/styles/vendor-fixes.css` | Native Tabulator scrollbar styling and compatibility overrides |
| `src/styles/base.css` | Cascade layer order, design tokens, scrollbar vars |

---

## Cascade And Loading

This was one of the main things learned during the failed probe.

### Correct OSB loading pattern

OverlayScrollbars base CSS should be loaded as vendor CSS in a lower cascade layer.

Current implementation:

- `src/styles/base.css` declares `vendors.overlayscrollbars`
- `src/styles/vendor-overlayscrollbars.css` imports:

```css
@import url("overlayscrollbars/styles/overlayscrollbars.css") layer(vendors.overlayscrollbars);
```

- `src/app.js` loads styles in this order:

```javascript
import './styles/base.css';
import './styles/vendor-overlayscrollbars.css';
import './styles/vendor-fixes.css';
import './styles/scrollbars.css';
```

### Why this matters

- OSB base CSS must be below Lithium overrides
- Unlayered vendor CSS is the wrong fit for Lithium's cascade architecture
- Vendor CSS belongs in a vendor layer, not as a random side-effect import from a JS module

This part is now corrected.

---

## OverlayScrollbars v2 Notes

The current online docs are for OverlayScrollbars v2. Lithium must follow those docs, not old v1 patterns.

### Correct theme mechanism

Use `scrollbars.theme`.

Example:

```javascript
scrollbars: {
  theme: 'os-theme-lithium'
}
```

### Do not rely on undocumented v1-style assumptions

The following were part of the confusion during debugging and should not be treated as the primary theming path:

- `className` as the main theme mechanism
- `resize` as if it were required for normal element theming
- old v1 CSS variable names like `--os-thumb-bg` and `--os-padding`

### Correct v2 CSS custom properties

Lithium's theme should use v2 names such as:

- `--os-size`
- `--os-padding-axis`
- `--os-padding-perpendicular`
- `--os-track-bg`
- `--os-track-border-radius`
- `--os-handle-bg`
- `--os-handle-bg-hover`
- `--os-handle-bg-active`
- `--os-handle-border-radius`
- `--os-handle-perpendicular-size`

The current `src/styles/scrollbars.css` has already been updated to use these names.

---

## What Is Working Today

### Tabulator / LithiumTable

Tabulator now uses OverlayScrollbars by default for all LithiumTable instances.

All LithiumTable tables use OSB:

- User Profile Manager (all tables)
- Query Manager
- Lookups Manager
- All other managers with LithiumTable

Tables can opt out by setting `useOverlayScrollbars: false` in the constructor.

Current styling in `src/styles/vendor-fixes.css`:

```css
.lithium-table-container .tabulator-tableholder {
  overflow: auto !important;
  scrollbar-width: auto;
  scrollbar-color: var(--accent-alt-primary) var(--bg-secondary);
}

.lithium-table-container .tabulator-tableholder::-webkit-scrollbar {
  width: 8px;
  height: 8px;
}
```

Current practical behavior:

- Firefox shows native scrollbars, still visually weak compared to what we want
- WebKit browsers get more control via pseudo-elements
- This remains the stable fallback state for LithiumTable generally
- The Date Formats token table is now the only exception

### CodeMirror

OverlayScrollbars is still managed through `scrollbarManager.initCodeMirror()`.

### SunEditor

OverlayScrollbars is still managed through `scrollbarManager.initSunEditor()`.

### Popups / dropdowns

OverlayScrollbars is still managed through `scrollbarManager.initPopup()`.

---

## Tabulator Probe Summary

The current probe uses the Date Formats token table in User Profile Manager as the test surface.

Target table:

- User Profile Manager
- Date Formats section
- token reference table

### Goal

Replace Tabulator's native scrollbars with fixed OverlayScrollbars that:

- are always visible when overflow exists
- do not overlay table content
- preserve Tabulator virtual scrolling
- preserve header/body sync
- preserve frozen-region alignment

### Probe states we tried

#### 1. CSS-only gutter reservation

Added `scrollbar-gutter: stable` to Tabulator scroll holders.

Result:

- safe
- did not break anything
- not sufficient for Firefox goals
- useful only as a minimal native baseline

#### 2. Firefox native scrollbar tuning

Changed Tabulator Firefox styling from `scrollbar-width: thin` to `scrollbar-width: auto`.

Result:

- safe
- made Firefox native bars slightly more visible
- still not the desired look

#### 3. Direct OSB on Tabulator holder (initial failed variants)

Attempted to initialize OSB directly against `.tabulator-tableholder`.

Result:

- custom scrollbar not visibly rendered
- virtual scrolling became unstable in some variants
- not viable as attempted

#### 4. Wrapper / viewport experiments

Attempted wrapper host strategies where OSB owned a wrapper while Tabulator holder was reused as viewport or proxied.

Observed failure modes:

- no visible OSB scrollbar chrome
- scroll snapping / reset behavior
- blank regions while scrolling through virtual rows
- complete loss of scrolling in some variants

#### 5. Page-specific token-table hookup (successful current probe)

OverlayScrollbars is now wired into the Date Formats token table page again, but with a narrower and more stable structure than the previous failed attempts.

Current implementation details:

- `LithiumTable` has a `useOverlayScrollbars` option
- the Date Formats token table enables that option
- `lithium-table-base.js` initializes OSB only after Tabulator has built the table
- OSB is initialized against the existing `.tabulator-tableholder`
- the existing `.tabulator-table` is reused as the content element
- the probe does **not** reparent the table holder into a custom wrapper
- OSB updates are triggered after table data loads / refreshes

Current result:

- scrolling works again
- generated `.os-scrollbar` elements are present in the DOM
- `os-theme-lithium` variables are populated on real generated scrollbar elements
- horizontal and vertical tracks are visible when needed
- the probe reserves space so scrollbars do not overlap table content
- inactive / unusable scrollbars are hidden again
- the bottom-right corner is explicitly filled when both bars are visible

Remaining status:

- this is good enough to continue testing
- this is not yet broad-rollout ready
- frozen columns, wider table variants, and more managers still need verification before expansion

---

## Critical Lessons Learned

These points should be assumed true for the next attempt unless disproven with a tight isolated repro.

### 1. Start from a live generated scrollbar element, not from the base CSS rule

Seeing this in DevTools:

```css
.os-scrollbar {
  --os-size: 0;
  --os-track-bg: none;
}
```

is not enough to prove anything by itself.

That is the library baseline.

The meaningful inspection target is an actual generated OSB scrollbar element that also has the theme class, for example something like:

```html
<div class="os-scrollbar os-scrollbar-vertical os-theme-lithium ...">
```

Only then should Lithium's theme overrides win.

### 2. If there are no generated `.os-scrollbar` elements, theming is irrelevant

During the failed probe, we repeatedly reached a state where:

- scrolling was broken or absent
- no usable visible OSB elements were found

In that state, CSS variable debugging was secondary. First confirm DOM generation.

### 3. Tabulator scroll ownership is the hard part

The core risk is not styling. It is scroll ownership.

Tabulator virtual rendering depends on the effective scroll viewport and its live geometry. If OSB changes that ownership, Tabulator may:

- stop rendering deeper rows
- reset scroll position
- desync row rendering from scroll offset
- lose horizontal sync behavior

The current probe confirms an important constraint:

- preserving `.tabulator-tableholder` as the actual scroll viewport is much safer than wrapper-based host ownership
- DOM reparenting was a major cause of the earlier failures

### 4. Do not mix multiple unknowns in one probe

The failed probe mixed several moving parts at once:

- CSS variable naming
- CSS load order / cascade layering
- OSB host / viewport structure
- Tabulator scroll forwarding / geometry assumptions

The next attempt should isolate these concerns.

### 5. The page-specific hookup was a bad place to keep iterating

Using the User Profile token table as the visual test point was useful.

Leaving broken OSB integration active there was not useful.

The next attempt should prove structure first, then wire it into a real page.

### 6. Preserve the restored native baseline until the new probe is proven

The current restored state is valuable. Do not replace it broadly until a new OSB pattern is verified.

### 7. Placement and sizing are separate from initialization

Once OSB DOM generation was working, the remaining problems were mostly CSS / layout problems rather than initialization failures.

Key follow-up fixes that mattered:

- anchoring OSB chrome to `.tabulator-tableholder` instead of the outer `.tabulator` shell
- not forcing generic width / height on OSB tracks in both axes
- letting OSB control handle length and axis travel
- letting OSB state classes control visibility instead of forcing tracks visible
- reserving viewport edge space only when a scrollbar is actually active
- explicitly styling the bottom-right corner when both axes are visible

---

## Current Working Probe

This section records the current narrow implementation that is working well enough for continued manual validation.

### Files involved

- `src/core/scrollbar-manager.js`
- `src/tables/lithium-table-base.js`
- `src/tables/data/data-loading.js`
- `src/managers/profile-manager/pages/page-date-formats.js`
- `src/styles/scrollbars.css`

### Current implementation shape

- `scrollbarManager.initTabulator()` initializes OSB directly on `.tabulator-tableholder`
- `.tabulator-tableholder` remains the effective Tabulator scroll viewport
- `.tabulator-table` is reused as the OSB content element
- `paddingAbsolute: true` is still used for gutter-style reserved space behavior
- `scrollbars.visibility` remains `auto`
- `clickScroll` is disabled for the Tabulator probe
- the Tabulator probe theme currently uses:
  - darker track color `#121212`
  - square track corners
  - `12px` bar size
  - `8px` thumb perpendicular size

### Current observed behavior

- the test table scrolls
- themed OSB bars appear only when overflow exists
- bars are anchored to the correct holder area
- bars do not overlap visible table content
- when both axes are present, the corner area is filled

### Open validation items before rollout

- behavior across more Tabulator tables
- behavior with frozen columns
- behavior with grouped rows or calc/footer heavy tables
- redraw / resize edge cases
- drag interactions across browsers

---

## Recommended Next Attempt

This is now the recommended sequence for rollout from the current successful probe.

### Step 1. Finish validating the Date Formats token-table probe

Before any rollout, test:

- vertical virtual scrolling
- horizontal scrolling
- header/body sync
- frozen columns
- redraw after filtering or reload
- resize behavior

### Step 2. Expand to one or two additional real LithiumTable surfaces

Prefer tables that differ structurally from the token table, such as:

- a wider horizontally scrollable table
- a table with frozen columns or different footer/calc behavior

### Step 3. Roll out behind the per-table opt-in

Keep `useOverlayScrollbars` as the rollout gate until enough table classes have been verified.

### Step 4. Convert the default only after the opt-in is proven broadly

Only after the above passes should Lithium consider moving from per-table opt-in to wider default behavior.

---

## Current Scrollbar Manager State

`src/core/scrollbar-manager.js` currently contains:

- working generic `init()` path
- working CodeMirror / SunEditor / popup initialization paths
- a partially validated `initTabulator()` probe that should still be treated as controlled rollout code, not final production-wide behavior

Important:

- `initTabulator()` existing in the file does **not** mean Tabulator OSB is enabled everywhere
- the live page-specific hookup is active only for the Date Formats token table
- the general user-visible behavior remains native Tabulator scrolling outside that probe

---

## Known Native Tabulator Baseline

Current baseline for Tabulator in `vendor-fixes.css`:

- `overflow: auto !important`
- Firefox: `scrollbar-width: auto`
- Firefox: `scrollbar-color: var(--accent-alt-primary) var(--bg-secondary)`
- WebKit: `8px` track with rounded thumb

This is the stable fallback and should be treated as the rollback point if a new probe fails.

---

## Troubleshooting Checklist For Next Time

When OSB appears to fail, inspect in this order:

1. Is OSB initialized on the target at all?
2. Are `.os-scrollbar` elements actually generated?
3. Do those generated elements have `os-theme-lithium`?
4. Do computed CSS custom properties on the generated themed scrollbar element show Lithium values instead of the library defaults?
5. Is the actual scroll viewport still the element Tabulator expects?
6. Can the end of the dataset be reached without reset or blank rows?

If step 2 fails, stop debugging theme CSS and debug initialization structure first.

---

## Related Documentation

- [LITHIUM-TAB.md](LITHIUM-TAB.md)
- [LITHIUM-TAB-TABLES.md](LITHIUM-TAB-TABLES.md)
- [LITHIUM-CSS.md](LITHIUM-CSS.md)
- [OverlayScrollbars Documentation](https://kingsora.github.io/OverlayScrollbars/)

---

## Implementation History

**April 2026:** Initial OSB integration

- Added `overlayscrollbars` package
- Added `scrollbar-manager.js`
- Added `scrollbars.css`
- Enabled OSB for non-Tabulator scrollable contexts

**April 2026:** Tabulator investigation phase

- Tested native gutter reservation and Firefox native tuning
- Tested direct-holder OSB initialization for Tabulator
- Tested wrapper / viewport variants for Tabulator
- Observed broken scrolling, invisible OSB chrome, and virtual-render failures
- Removed failed live token-table hookup and restored native scrolling

**April 21, 2026:** Baseline reset for next attempt

- Added `vendor-overlayscrollbars.css`
- Moved OSB base CSS into `vendors.overlayscrollbars`
- Kept Lithium OSB theme CSS above vendor layers
- Restored live Tabulator tables to native scrolling
- Recorded lessons learned in this document for the next clean attempt

**April 21, 2026:** Narrow live Tabulator probe restored

- Added `useOverlayScrollbars` opt-in to `LithiumTable`
- Re-enabled OSB only for the User Profile Manager Date Formats token table
- Initialized OSB directly on `.tabulator-tableholder` while preserving Tabulator scroll ownership
- Added OSB update hooks after table data refreshes
- Confirmed generated OSB chrome and populated `--os-*` variables on real scrollbar elements
- Fixed placement so scrollbars anchor to the holder rather than the outer Tabulator shell
- Reserved holder edge space so bars do not overlap table content
- Hid inactive tracks again using OSB state classes
- Tuned current probe visuals: square corners, darker track, wider bars, filled bottom-right corner

 **April 25, 2026:** Timezone Picker Popup OSB Attempt (Failed)

- **Target:** Timezone dropdown in Date Formats page (`page-date-formats.js`)
- **Goal:** Fix overlay scrollbars in the timezone picker popup
- **Changes made:**
  - Added `scrollbarManager` import to `page-date-formats.js`
  - Replaced manual `window.OverlayScrollbars()` initialization with `scrollbarManager.initPopup(this.listContainer)`
  - Removed incorrect theme `'os-theme-light'` and invalid v1 options (`autoHide: 'scroll'`, `autoHideDelay: 800`)
  - `scrollbarManager.initPopup()` applies correct theme `os-theme-lithium os-theme-lithium-popup` with proper v2 configuration
  - Popup config sets `visibility: 'visible'`, `autoHide: 'never'`, `paddingAbsolute: false`
- **Result:** Did not work — scrollbars still not functioning correctly in timezone picker popup
- **Status:** Failed attempt — centralized init works for other popups but timezone picker has additional issues (possibly related to `position: absolute` dropdown, dynamic show/hide cycle, or `document.body` append strategy)

---

**April 25, 2026:** Timezone Picker Popup OSB Attempt #3 (COMPLETED - SUCCESSFUL)

- **Target:** Timezone dropdown in Date Formats page (`page-date-formats.js`)
- **Goal:** Fix overlay scrollbars in the timezone picker popup
- **Previous attempts failed because:**
  - Attempt #1: Used manual `window.OverlayScrollbars()` with incorrect theme/options
  - Attempt #2: Fixed `initPopup()` to let OSB create its own structure, updated `filterTimezones()` to use `.os-content`, but still no scrollbars or scrolling
- **Final successful approach:**
  - Initialize OSB on the `.df-timezone-list` element directly after content is populated
  - Added `overflow-y: auto !important` to `.df-timezone-list` for fallback scrolling during initialization
  - Added abbreviations support with full names (PDT → "PDT (Pacific Daylight Time)")
  - Fixed `transform-origin: top left` for popup animation
  - Implemented proper DOM cleanup on popup close
  - Used `requestAnimationFrame` to ensure layout is complete before OSB initialization
- **Key fixes implemented:**
  - ✅ OverlayScrollbars working with proper initialization timing
  - ✅ Abbreviations displayed with full names
  - ✅ Correct popup animation transform-origin
  - ✅ Proper DOM cleanup and state management
  - ✅ Consistent popup display behavior
- **Status:** COMPLETED - All timezone popup issues resolved

---

**Last Updated:** April 26, 2026
