# Lithium Migration Notes

Historical fixes and migration notes documenting significant changes. For current troubleshooting, see [LITHIUM-FAQ.md](LITHIUM-FAQ.md).

---

## Phase 6 Fixes (April 2026)

### Param Objects Deep Merge (Fixed in Phase 6)

**Problem:** Changing `formatterParams.precision` in Lookup 059 didn't apply to tables with saved templates. The `formatterParams` object from templates completely replaced the coltype defaults.

**Root Cause:** Shallow merge using spread operator replaced entire `formatterParams` objects.

**Fix:** Deep merge for param objects. These properties are now deep-merged:
- `formatterParams`, `editorParams`, `headerFilterParams`
- `sorterParams`, `accessorParams`, `mutatorParams`
- `bottomCalcFormatterParams`, `downloadFormatterParams`, `downloadCalcParams`, `clipboardParams`

**Implementation:** `deepMergeParams()` helper in `column-management.js`

---

### Double Column Discovery (Fixed in Phase 6)

**Problem:** `discoverColumns()` called twice per data load for auto-discover tables.

**Root Cause:** Both `loadData()` and `dataLoaded` callback triggered discovery.

**Fix:** Removed `dataLoaded` callback in `initTable()`. `loadData()` already calls `discoverColumns()` after `setData()`.

---

### TableDef Cache Not Clearing on Logout (Fixed in Phase 6)

**Problem:** Table definitions persisted across user sessions.

**Fix:** Added `AUTH_LOGOUT` event listener in `tabledef-loader.js` to clear `_tableDefCache` on logout.

---

### Row Selection Lost After Refresh (Fixed in Phase 6)

**Problem:** After clicking Refresh, first row selected instead of previously selected row.

**Root Cause:** `autoSelectRow()` was selecting first row when restored ID couldn't be matched.

**Fix:**
```javascript
const savedAutoSelectRow = table.autoSelectRow;
table.autoSelectRow = () => {};  // Disable during load
await table.loadData();
table.autoSelectRow = savedAutoSelectRow;
await new Promise(r => requestAnimationFrame(r));
table.autoSelectRow(capturedRowId);  // Explicitly restore
```

---

### Parent/Child Table Refresh Coordination (Fixed in Phase 6)

**Problem:** In Lookups Manager, child table wouldn't reload after parent refresh.

**Root Cause:** `_loadedChildLookupKey` guard was still set from before refresh.

**Fix:** Added `onRefreshComplete()` callback to clear child load state:

```javascript
onRefreshComplete: () => {
  this._loadedChildLookupKey = null;
},
```

---

### Refresh Clears All TableDefs (Fixed in Phase 6)

**Problem:** After Refresh in one manager, other managers used auto-discovered columns.

**Root Cause:** `refreshTabulatorSchemas()` used `fetchBatchQueries()` without JWT.

**Fix:** Changed to use `authQuery()` instead.

---

## Phase 7 Fixes (April 2026)

### String Editor Not Opening (Fixed in Phase 7)

**Problem:** String columns wouldn't open editor when clicked.

**Root Cause:**
1. Table definitions used `editor: "string"` instead of `editor: "input"`
2. Edit button toggle behavior was wrong

**Fix:** Changed to `editor: "input"` and modified `handleEdit()` to always exit when clicked while editing.

---

### Edit Mode Exiting on Non-Editable Cell Clicks (Fixed in Phase 7)

**Problem:** Clicking non-editable cells exited edit mode unexpectedly.

**Fix:** Modified `cellMouseDown` handler to skip row selection when in edit mode:

```javascript
if (!table.isEditing) {
  table.selectDataRow(cell.getRow());
}
```

---

### Same-Row Clicks Exiting Edit Mode (Fixed in Phase 7)

**Problem:** Clicking different cells within same row exited edit mode.

**Fix:** Added defensive check in `handleRowSelected`:

```javascript
if (table.isEditing && table.editingRowId === newRowId) {
  table.updateSelectorCell?.(row, true);
  return;
}
```

---

### CodeMirror ESC and Ctrl+Enter Shortcuts (Fixed in Phase 7)

**Problem:** ESC doesn't cancel, no Ctrl+Enter to save when focus in CodeMirror.

**Fix:** Added keymap bindings for Escape and Ctrl+Enter in `buildEditorExtensions()`.

**Shortcuts:**
- **Escape**: Cancel edit mode
- **Ctrl+Enter**: Save changes

---

### Composite Primary Key Handling (Fixed in Phase 7)

**Problem:** Tables with composite keys (like Lookups with `lookup_id` + `key_idx`) had broken edit mode.

**Root Cause:** Code assumed `primaryKeyField` was a single string, not an array.

**Fix:** Updated to use `_getCompositeRowId()` which handles both single and composite keys.

---

## Phase 8 Fixes (April 2026)

### Lookup Column Editing (Fixed in Phase 8)

**Problem:** Tabulator list editor stored label text instead of ID.

**Root Cause:** Tabulator's `list` editor with `values: ["label1", "label2"]` returns the selected string.

**Fix:** Use `values: {id1: "label1", id2: "label2"}` to return the ID while displaying the label. Added mutator safeguard:

```javascript
tabulatorCol.mutator = function(value) {
  if (value === null || value === undefined || value === '') return 0;
  if (typeof value === 'string') {
    const entry = lookupData.find(e => e.label === value);
    if (entry) return entry.id;
  }
  return parseInt(value, 10) || 0;
};
```

---

## Column Property Names (Canonical)

**The canonical property for column header text is `title`.** The legacy property `display` has been retired.

**Migration:**
- Change `display: "Name"` → `title: "Name"` in all tableDef JSON files
- The `title` property aligns with Tabulator's native naming convention

**Files affected:**
- All table definitions in Lookup 059 (Keys 1+)
- Column Manager column definitions
- Template extraction/capture code

---

## Library Migrations

### JSON Editor Evolution

1. **March 2026:** `jsoneditor` (v10.x) → `vanilla-jsoneditor` (v2.x)
2. **April 2026:** `vanilla-jsoneditor` → CodeMirror 6 with `@codemirror/lang-json`

**Changes:**
- All editor types (SQL, Markdown, JSON, CSS) consolidated onto CodeMirror
- `init/jsoneditor-init.js` remains as dead code
- JSON editing uses `buildEditorExtensions()` + `createReadOnlyCompartment()` pattern

---

## Tabulator Animation Notes (Tabulator Group Arrows)

**Problem:** Expand/collapse arrow doesn't animate — it snaps instantly.

**Cause:** Tabulator's `Group.generateElement()` rebuilds from HTML string on every toggle.

**Fix:** Use **prior-state pinning** — documented as Approach 3 in [LITHIUM-ICN.md](LITHIUM-ICN.md#approach-3-prior-state-pinning-for-host-regenerated-icons).

Key points:
1. CSS-only won't work — the new icon has no prior state
2. Inline `style.transform` is fragile — FA replaces the element
3. An in-flight guard is mandatory — Tabulator fires multiple events per toggle
4. Forced reflow between pin and release is mandatory — read `container.offsetHeight`

---

## Tour Manager Numeric ID Matching

**⚠️ CRITICAL:** The Tour Manager **ONLY uses numeric IDs** for matching tours to managers.

```javascript
// "003.Profile" matches "003.User Profile" because both are ID 3
// "029.Query Manager" matches "029.Queries" because both are ID 29
```

**Implementation:** `extractManagerId()` parses the numeric portion:

```javascript
function extractManagerId(managerStr) {
  const match = managerStr.match(/^(\d{3})\./);
  return match ? parseInt(match[1], 10) : null;
}
```

**See:** [LITHIUM-MGR-TOUR.md](LITHIUM-MGR-TOUR.md) for full documentation.