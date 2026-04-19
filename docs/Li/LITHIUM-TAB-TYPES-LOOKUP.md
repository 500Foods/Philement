# Lookup Column Type

Foreign key reference to a lookup table (displays label, stores ID).

```json
"lookup": {
  // -- Core --
  "description": "Foreign key reference to a lookup table (displays label, stores ID)",

  // -- Layout --
  "hozAlign": "left",
  "vertAlign": "middle",
  "headerHozAlign": null,
  "headerVertical": false,
  "width": null,
  "minWidth": 80,
  "maxWidth": null,
  "frozen": false,
  "resizable": true,
  "responsive": null,

  // -- Display --
  "formatter": "lookup",
  "formatterParams": {
    "lookupTable": null
  },
  "cssClass": "li-col-lookup",
  "cellStyle": null,
  "headerStyle": null,
  "rowStyle": null,
  "rowHandle": false,
  "tooltip": null,
  "headerTooltip": null,
  "download": true,
  "headerWordWrap": false,

  // -- Editing --
  "editor": "list",
  "editorParams": {
    "allowEmpty": false,
    "autocomplete": true,
    "freetext": false,
    "listOnEmpty": true,
    "sort": "asc",
    "valuesLookup": true
  },
  "editable": false,
  "validator": null,

  // -- Sorting --
  "sorter": "alphanum",
  "sorterParams": {},
  "headerSort": true,
  "sortPri": null,

  // -- Filtering --
  "headerFilter": false,
  "headerFilterFunc": "like",
  "headerFilterPlaceholder": "filter...",
  "headerWordWrap": false,

  // -- Grouping --
  "groupable": false,
  "groupPri": null,
  "groupOrd": "asc",
  "columnPri": null,

  // -- Data --
  "blank": "",
  "zero": null,
  "accessor": null,
  "accessorParams": {},
  "accessorDownload": null,
  "mutator": null,
  "mutatorParams": {},

  // -- Aggregation --
  "bottomCalc": null,
  "bottomCalcFormatter": null,
  "bottomCalcFormatterParams": {},
  "bottomCalcHozAlign": null,
  "bottomCalcStyle": null,

  // -- Lookup --
  "lookupRef": null,
  "lookupStyle": null,
  "lookupEdit": null,

  // -- Interaction --
  "contextMenu": null,
  "headerClickMenu": null,
  "cellClick": null,
  "headerClick": null,
}
```

**Additional Properties for lookup:**

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `lookupRef` | integer | `null` | **Required.** Lookup table ID (e.g., `27` for Lookup 027). The column stores integer IDs from this lookup; displays are resolved to labels via the Lookup cache. |
| `lookupStyle` | string | `"label"` | How the **cell** displays: `"label"` (text only) or `"icon"` (icon only from `collection.icon`). |
| `lookupEdit` | string | (from `lookupStyle`) | How the **dropdown** displays: `"label"`, `"icon"`, or `"iconLabel"`. Defaults to match `lookupStyle`. |

---

## Display Combinations

The six valid combinations of `lookupStyle` and `lookupEdit`:

| Code | Cell Display | Dropdown Display | Column Properties |
|------|--------------|------------------|-------------------|
| A | Icon | Icon | `lookupStyle: "icon"`, `lookupEdit: "icon"` |
| B | Icon | Icon + Label | `lookupStyle: "icon"`, `lookupEdit: "iconLabel"` |
| C | Icon | Label | `lookupStyle: "icon"`, `lookupEdit: "label"` |
| D | Label | Icon | `lookupStyle: "label"`, `lookupEdit: "icon"` |
| E | Label | Icon + Label | `lookupStyle: "label"`, `lookupEdit: "iconLabel"` |
| F | Label | Label | `lookupStyle: "label"` (or omitted) |

## How It Works

1. **Data Storage:** The column stores integer IDs (e.g., `1`, `2`, `3`) in the database
2. **Display:** The `formatter` resolves IDs to labels via the Lookup cache
3. **Editing:** The `list` editor shows a dropdown populated from the Lookup
4. **Commit:** The selected value is stored as an integer (never the label text)

## Example Column Definition

```json
{
  "field": "query_status_a27",
  "title": "Status",
  "coltype": "lookup",
  "lookupRef": 27,
  "lookupStyle": "icon",
  "lookupEdit": "iconLabel",
  "hozAlign": "center",
  "editable": true,
  "visible": true
}
```

This example:
- References Lookup 027 (Query Status)
- Shows only icons in cells
- Shows icon + label in the dropdown
- Centers both cell and dropdown content

## Dropdown Alignment Behavior

When `lookupEdit` is `"icon"`, the dropdown items respect the column's `hozAlign` setting:

| `hozAlign` | Icon-only dropdown alignment |
|------------|------------------------------|
| `"left"` | Icons align left |
| `"center"` | Icons align center |
| `"right"` | Icons align right |

When `lookupEdit` is `"iconLabel"` or `"iconText"`, dropdown items always align left (regardless of `hozAlign`) to ensure consistent icon+text readability.

## Icon+Label DOM Structure

For `lookupEdit: "iconLabel"` and `lookupEdit: "iconText"`, the dropdown uses a protected DOM structure to prevent FontAwesome mutations from interfering with the label:

```html
<div class="li-lookup-item" data-align="left">
  <span class="li-lookup-edit-icon">
    <fa fa-check></fa>  <!-- FontAwesome replaces this with SVG -->
  </span>
  <span class="li-lookup-edit-label">Active</span>  <!-- Protected from mutations -->
</div>
```

The icon and label are wrapped in separate spans to isolate FontAwesome's SVG replacement from the label text. This prevents issues where the label would appear briefly then disappear due to DOM mutations.

---

**Related Documentation:**
- [LITHIUM-TAB-TYPES.md](LITHIUM-TAB-TYPES.md) — Column properties reference
- [LITHIUM-TAB.md](LITHIUM-TAB.md) — LithiumTable component
- [LITHIUM-LUT.md](LITHIUM-LUT.md) — Lookup Tables
- [Phase-8-11-Plan.md](Phase-8-11-Plan.md) — Implementation notes from development