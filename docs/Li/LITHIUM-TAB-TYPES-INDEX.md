# Index Column Type

Simple whole numbers (lookup values).

```json
"index": {
  // -- Core --
  "description": "Simple whole numbers (lookup values)",

  // -- Layout --
  "hozAlign": "right",
  "vertAlign": "middle",
  "headerHozAlign": null,
  "headerVertical": false,
  "width": null,
  "minWidth": 40,
  "maxWidth": null,
  "frozen": false,
  "resizable": true,
  "responsive": null,

  // -- Display --
  "formatter": "number",
  "formatterParams": {
    "thousand": "",
    "precision": 0
  },
  "cssClass": "li-col-integer",
  "cellStyle": null,
  "headerStyle": null,
  "rowStyle": null,
  "rowHandle": false,
  "tooltip": null,
  "headerTooltip": null,
  "download": true,
  "headerWordWrap": false,

  // -- Editing --
  "editor": "number",
  "editorParams": {
    "min": null,
    "max": null,
    "step": 1
  },
  "editable": false,
  "validator": null,

  // -- Sorting --
  "sorter": "number",
  "sorterParams": {},
  "headerSort": true,
  "sortPri": null,

  // -- Filtering --
  "headerFilter": false,
  "headerFilterFunc": ">=",
  "headerFilterPlaceholder": "filter...",
  "headerWordWrap": false,

  // -- Grouping --
  "groupable": false,
  "groupPri": null,
  "groupDir": "asc",
  "columnPri": null,

  // -- Data --
  "blank": "0",
  "zero": "0",
  "accessor": null,
  "accessorParams": {},
  "accessorDownload": null,
  "mutator": null,
  "mutatorParams": {},

  // -- Aggregation --
  "bottomCalc": "count",
  "bottomCalcFormatter": "number",
  "bottomCalcFormatterParams": {
    "thousand": "",
    "precision": 0
  },

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

---

**Related Documentation:**
- [LITHIUM-TAB-TYPES.md](LITHIUM-TAB-TYPES.md) — Column properties reference
- [LITHIUM-TAB.md](LITHIUM-TAB.md) — LithiumTable component