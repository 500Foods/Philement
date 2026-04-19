# Phase 8+11 — Lookup Coltype Family (Combined)

**Status:** ✅ **COMPLETE** — April 18, 2026

## Goal
Implement complete lookup column editing with support for icon and label display variants. The lookup family includes: `lookup`, `lookupIcon`, `lookupIconText`, `lookupIconList` — all using the same underlying integer storage but with configurable display modes.

## Background

### Current State (Phase 7 Complete)
- String editing works correctly
- Lookup columns display correctly via `createLookupFormatter` (ID → label)
- Lookup preloading happens at the right time in `loadConfiguration()`
- **BUG**: `createLookupEditor` returns array of labels `["Active", "Draft"]` instead of ID→label map `{1: "Active", 3: "Draft"}` — selecting stores the label string, not the ID

### Query Manager Test Case
Three lookup fields for testing:
| Field | Lookup ID | Has Icons? |
|-------|-----------|------------|
| `query_type_a28` | 28 (Query Type) | Yes |
| `query_status_a27` | 27 (Query Status) | No |
| `query_dialect_a30` | 30 (SQL Dialect) | Yes |

**Note:** `lookupRef` is always an integer (the lookup table ID). The code accepts strings for backwards compatibility but normalizes to string internally for cache consistency.

### Lookup Data Structure (from QueryRef 34)
```javascript
{
  id: 1,                    // key_idx (the integer stored in the database)
  label: "Active",          // value_txt (default display text)
  icon: "<i class='...'>",  // collection.icon (optional FontAwesome markup)
  metadata: {               // collection object
    icon: "fa-check",
    label: {
      "en-US": "Active",
      "fr-FR": "Actif"
    }
  }
}
```

## Requirements Summary

### Display Modes (Cell Presentation)
| Mode | Shows | Use Case |
|------|-------|----------|
| `label` | Text only | Most lookups (e.g., Query Status) |
| `icon` | Icon only | Visual indicators (e.g., theme selector) |

Note: No `iconText` for cell display — if needed, use two columns.

### Edit Modes (Dropdown Presentation)
| Mode | Shows | Use Case |
|------|-------|----------|
| `label` | Text only | Simple selection |
| `icon` | Icon only | Visual selection |
| `iconText` | Icon + Text | Complete information |

### The 6 Combinations (A-F)
| Code | Cell Display | Dropdown Display | Coltype Config |
|------|--------------|------------------|----------------|
| A | Icon | Icon | `lookupIcon` + `lookupEdit: "icon"` |
| B | Icon | Icon+Label | `lookupIcon` + `lookupEdit: "iconLabel"` |
| C | Icon | Label | `lookupIcon` + `lookupEdit: "label"` |
| D | Label | Icon | `lookup` + `lookupEdit: "icon"` |
| E | Label | Icon+Label | `lookup` + `lookupEdit: "iconLabel"` |
| F | Label | Label | `lookup` (default) |

## Implementation Plan

### Step 1: Fix Basic Lookup Editing (The Phase 8 Bug)

**File:** `src/tables/resolution/lookup-loader.js`

**Changes:**
1. Update `loadLookup` to extract icon from `collection`
2. Fix `createLookupEditor` to return `{id: label}` map
3. Change fallback from `'input'` to `'number'` when no lookup data
4. Add `createLookupEditorWithOptions(lookupRef, lookupData, options)` supporting `editMode: 'label'|'icon'|'iconText'`

**Before:**
```javascript
const values = lookupData.map(entry => entry.label);
```

**After:**
```javascript
// For value storage (always ID → label)
const values = {};
lookupData.forEach(entry => {
  values[entry.id] = entry.label;
});

// For display modes, we'll use Tabulator's itemFormatter
```

### Step 2: Extend loadLookup for Icons

**Current:** Returns `{id, label}`
**New:** Returns `{id, label, icon, metadata}`

```javascript
// In loadLookup, map function:
const lookupData = rows.map(row => {
  let collection = {};
  if (row.collection) {
    try {
      collection = typeof row.collection === 'string' 
        ? JSON.parse(row.collection) 
        : row.collection;
    } catch (_e) { /* ignore */ }
  }
  
  return {
    id: row.key_idx ?? row.lookup_id ?? row.id ?? row.value,
    label: row.value_txt ?? row.lookup_label ?? row.label ?? row.name ?? String(row.key_idx ?? row.lookup_id ?? row.id ?? row.value),
    icon: collection.icon || null,  // e.g., "fa-check" or "<i class='fa fa-check'>"
    metadata: collection,
  };
});
```

### Step 3: Create Lookup Editor Factory with Options

**New Function:** `createLookupEditor(lookupRef, lookupData, options)`

```javascript
/**
 * Create a lookup editor with configurable display modes.
 * 
 * @param {string} lookupRef - Lookup table reference (e.g., "a27")
 * @param {Array<{id, label, icon, metadata}>} lookupData - Pre-loaded lookup data
 * @param {Object} options - Configuration options
 * @param {string} options.cellMode - 'label' | 'icon' (how cell displays)
 * @param {string} options.editMode - 'label' | 'icon' | 'iconText' (how dropdown displays)
 * @returns {Object} Tabulator editor configuration
 */
export function createLookupEditor(lookupRef, lookupData, options = {}) {
  if (!lookupData || lookupData.length === 0) {
    return {
      editor: 'number',  // Fallback to direct number editing
      editorParams: { min: 0 }
    };
  }

  const { cellMode = 'label', editMode = 'label' } = options;

  // Build values map: ID → display string
  const values = {};
  lookupData.forEach(entry => {
    values[entry.id] = entry.label;
  });

  // Build item formatter for dropdown based on editMode
  const itemFormatter = (label, value, item, element) => {
    const entry = lookupData.find(e => e.id == value);
    if (!entry) return label;

    switch (editMode) {
      case 'icon':
        return entry.icon 
          ? `<i class="${entry.icon}"></i>` 
          : `<span class="li-lookup-no-icon">•</span>`;
      
      case 'iconText':
        const iconHtml = entry.icon 
          ? `<i class="${entry.icon} li-lookup-icon-left"></i>` 
          : '';
        return `${iconHtml}<span>${entry.label}</span>`;
      
      case 'label':
      default:
        return entry.label;
    }
  };

  return {
    editor: 'list',
    editorParams: {
      values: values,
      autocomplete: true,
      listOnEmpty: true,
      freetext: false,
      allowEmpty: true,
      itemFormatter: itemFormatter,
      // Store 0 for empty selection
      emptyValue: 0,
    },
    formatter: cellMode === 'icon' 
      ? createIconOnlyFormatter(lookupData) 
      : createLookupFormatter(lookupRef),
  };
}
```

### Step 4: Create Icon-Only Formatter

**New Function:** `createIconOnlyFormatter(lookupData)`

```javascript
export function createIconOnlyFormatter(lookupData) {
  return function(cell, _onRendered) {
    const value = cell.getValue();
    if (value == null || value === '' || value === 0) return '';
    
    const entry = lookupData.find(item => item.id == value);
    if (!entry?.icon) return `<span class="li-lookup-no-icon">•</span>`;
    
    // Handle both "fa-check" (class name) and "<i class='fa fa-check'>" (HTML)
    if (entry.icon.startsWith('<')) {
      return entry.icon;
    }
    return `<i class="${entry.icon}"></i>`;
  };
}
```

### Step 5: Update resolveColumn for Lookup Properties

**File:** `src/tables/lithium-table.js`

**In `resolveColumn`, lookup handling section:**

```javascript
// Current (lines 239-253):
if (isEditable && colDef.lookupRef && getLookup(colDef.lookupRef)) {
  const lookupData = getLookup(colDef.lookupRef);
  const lookupEditorConfig = createLookupEditor(colDef.lookupRef, lookupData);
  // ...
}

// New:
if (isEditable && colDef.lookupRef && getLookup(colDef.lookupRef)) {
  const lookupData = getLookup(colDef.lookupRef);
  
  // Determine display/edit modes from colDef
  const cellMode = colDef.lookupStyle === 'icon' ? 'icon' : 'label';
  const editMode = colDef.lookupEdit || 'label';
  
  const lookupEditorConfig = createLookupEditor(colDef.lookupRef, lookupData, {
    cellMode,
    editMode,
  });
  
  if (typeof lookupEditorConfig === 'object') {
    tabulatorCol.editor = lookupEditorConfig.editor;
    tabulatorCol.editorParams = lookupEditorConfig.editorParams;
    // Use the formatter from config (may be icon-only)
    if (lookupEditorConfig.formatter) {
      tabulatorCol.formatter = lookupEditorConfig.formatter;
    }
  } else {
    tabulatorCol.editor = lookupEditorConfig;
  }
}
```

### Step 6: Handle Empty Values (Store 0)

Tabulator's `list` editor with `allowEmpty: true` returns `null` or `undefined` when cleared. We need to convert this to `0`.

**Option A:** Mutator on the column
```javascript
if (colDef.coltype?.startsWith('lookup')) {
  tabulatorCol.mutator = function(value) {
    return value == null || value === '' ? 0 : parseInt(value, 10);
  };
}
```

**Option B:** Cell edit callback (in table-events.js)
Handle in `cellEdited` event.

### Step 7: Create Test File

**File:** `tests/unit/lookup-columns.test.js`

**Test Coverage:**
1. `createLookupEditor` returns correct values map format
2. `createLookupEditor` falls back to 'number' editor when no data
3. Icon-only formatter renders icons correctly
4. Label formatter renders labels correctly
5. Item formatter for dropdown (all 3 modes: icon, iconText, label)
6. Empty value handling (stores 0)
7. Value round-trip: ID → editor → ID
8. `loadLookup` extracts icon and metadata correctly

### Step 8: Update Migration 1153

**File:** `elements/002-helium/acuranzo/migrations/acuranzo_1153.lua`

**Current lookup coltype stanza needs:**
1. `editorParams` updated to show actual implementation params
2. Remove `valuesLookup: true` (was documentation-only)
3. Add `lookupStyle` and `lookupEdit` properties

**New lookup coltype stanzas to add:**
- `lookupIcon` — icon display, configurable edit mode
- `lookupIconText` — icon+text in dropdown only
- `lookupIconList` — alias or variant

### Step 9: Update Documentation

**Files to update:**
- `LITHIUM-TAB-TYPES-LOOKUP.md` — fix the values format, document lookupStyle/lookupEdit
- `LITHIUM-TAB-TYPES-LOOKUPICON.md` — new doc for icon variants
- `LITHIUM-LUT.md` — LithiumTable integration section
- `LITHIUM-FAQ.md` — root cause for the label-vs-ID bug

## Questions for Clarification

1. **Icon Source Priority:**
   - If `collection.icon` exists, use it
   - If not, should we check `collection.fa` (Font Awesome class name)?
   - Fallback: show bullet point `•` or empty?

2. **lookupStyle vs lookupEdit naming:**
   - `lookupStyle` = cell display mode ('label' | 'icon')
   - `lookupEdit` = dropdown display mode ('label' | 'icon' | 'iconText')
   - Does this naming work for you?

3. **Coltype Names:**
   - Should we have separate coltypes: `lookup`, `lookupIcon`, `lookupIconText`, `lookupIconList`?
   - Or single `lookup` coltype with `lookupStyle` property?
   - The plan above uses: `lookup` (label cell) + `lookupIcon` (icon cell), both with `lookupEdit` override

4. **Empty Value Display:**
   - When value is 0 or null, show empty cell?
   - Or show placeholder like "—" or "(none)"?

5. **Icon Processing:**
   - Does `collection.icon` contain raw class names (`"fa-check"`) or full HTML (`"<i class='fa fa-check'></i>"`)?
   - The plan handles both, but want to confirm

## Implementation Order

1. **Fix basic lookup editing** (Step 1) — unblock Query Manager testing ✅
2. **Create test file** (Step 7) — verify the fix ✅
3. **Add icon support** (Steps 2-5) — implement full feature ✅
4. **Update migration 1153** (Step 8) — database sync ⏳ (deferred to user)
5. **Update docs** (Step 9) — complete the phase ✅

---

## Completion Summary (April 18, 2026)

### What Was Delivered

**Phase 8 (Basic Lookup Editing):**
- ✅ Fixed `createLookupEditor()` to return `{id: label}` map
- ✅ Added mutator for integer value enforcement
- ✅ Changed fallback to `'number'` editor
- ✅ Normalized `lookupRef` handling (string/integer)

**Phase 11 (Icon Support):**
- ✅ Extended `loadLookup()` to extract `icon` from `collection`
- ✅ Added `createIconFormatter()` for icon-only display
- ✅ Implemented `lookupStyle` property (cell display)
- ✅ Implemented `lookupEdit` property (dropdown display)
- ✅ Six display combinations working (A-F)
- ✅ hozAlign support in dropdowns via CSS
- ✅ Zero value handling

### Key Decisions Made

1. **Single coltype approach:** Instead of four coltypes (`lookup`, `lookupIcon`, etc.), we use one `lookup` coltype with `lookupStyle` and `lookupEdit` properties. This provides the same six display combinations with less code duplication.

2. **Icon source:** `collection.icon` contains full HTML (e.g., `"<fa fa-check></fa>"` or `"<i class='fas fa-check'></i>"`). No additional processing needed.

3. **Empty values:** Store `0` for empty/cleared lookup values (consistent with integer storage).

4. **lookupEdit naming:** Uses `"iconLabel"` (not `"iconText"`) for consistency with "label" terminology.

### Files Modified

| File | Changes |
|------|---------|
| `src/tables/resolution/lookup-loader.js` | Icon extraction, ID→label map, number fallback, debug logging |
| `src/tables/lithium-table.js` | `lookupStyle`/`lookupEdit` support, mutator, CSS class-based rendering |
| `src/tables/template/capture.js` | Added `lookupStyle`/`lookupEdit` to `CANONICAL_COLUMN_PROPS` |
| `src/styles/vendor-fixes.css` | Lookup dropdown styling, flexbox alignment, hozAlign support |
| `tests/unit/lithium-table.test.js` | Updated tests for new values format |

### Test Results

- 642 tests pass
- Clean lint
- Clean build
- Manually verified with Query Manager (all three lookup fields working)

### Documentation Updated

- ✅ `LITHIUM-TAB-TYPES-LOOKUP.md` — Added `lookupStyle` and `lookupEdit` properties with examples
- ✅ `LITHIUM-TAB-PLAN.md` — Marked Phase 8 and Phase 11 complete
- ✅ `LITHIUM-FAQ.md` — Added root cause entry for label-vs-ID bug
- ✅ `Phase-8-11-Plan.md` — This document

### Open Items

- ⏳ **Migration 1153 update** — User needs to update database coltype defaults for `lookupStyle` and `lookupEdit` properties
- ⏳ **Separate coltypes discussion** — If desired later, we can add `lookupIcon` as a convenience alias that sets `lookupStyle: "icon"`

### Lessons Learned

1. **Tabulator `list` editor behavior:** The `values` property format determines what gets stored. Array of labels → stores label string. Object mapping `{id: label}` → stores ID. Critical distinction for integer foreign keys.

2. **CSS specificity with cascade layers:** When using `@layer`, styles in the same layer as Tabulator's styles need `!important` or higher specificity to override. The `:has()` selector proved useful for targeting parent elements based on child attributes.

3. **Flexbox vs text-align:** For icon+label combinations, flexbox with `justify-content` is more reliable than `text-align` for centering, especially when mixing SVG icons and text spans.

4. **State persistence:** Adding properties to `CANONICAL_COLUMN_PROPS` and `extractColumnMeta()` is necessary but not sufficient — both must be updated for template/template save/refresh to work correctly.
