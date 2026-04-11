# LookupIconList Column Type

Like `lookup` but displays only icon in cell, icon + name in dropdown.

```json
"lookupIconList": {
  // -- Core --
  "description": "Like lookup but displays only icon in cell, icon + name in dropdown",

  // -- Layout --
  "hozAlign": "center",
  "vertAlign": "middle",
  "headerHozAlign": null,
  "headerVertical": false,
  "width": null,
  "minWidth": 50,
  "maxWidth": null,
  "frozen": false,
  "resizable": true,
  "responsive": null,

  // -- Display --
  "formatter": "icon",
  "formatterParams": {},
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
    "valuesLookup": true,
    "itemFormatter": "iconWithText"
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
  "lookupStyle": "icon",
  "lookupEdit": "iconText",

  // -- Interaction --
  "contextMenu": null,
  "headerClickMenu": null,
  "cellClick": null,
  "headerClick": null,
}
```

**Characteristics:**
- Shows icon only in cell (compact)
- Shows icon + text label in dropdown
- Stores integer ID
- Good for compact status displays

---

**Related Documentation:**
- [LITHIUM-TAB-TYPES.md](LITHIUM-TAB-TYPES.md) — Column properties reference
- [LITHIUM-TAB.md](LITHIUM-TAB.md) — LithiumTable component
- [LITHIUM-LUT.md](LITHIUM-LUT.md) — Lookup Tables