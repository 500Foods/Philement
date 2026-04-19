# Progress Column Type

Progress bar (0-100 numeric value displayed as bar).

```json
"progress": {
  // -- Core --
  "description": "Progress bar (0-100 numeric value displayed as bar)",

  // -- Layout --
  "hozAlign": "left",
  "vertAlign": "middle",
  "headerHozAlign": null,
  "headerVertical": false,
  "width": 120,
  "minWidth": 80,
  "maxWidth": null,
  "frozen": false,
  "resizable": true,
  "responsive": null,

  // -- Display --
  "formatter": "progress",
  "formatterParams": {
    "color": ["#ff0000", "#ffaa00", "#00cc00"],
    "legendAlign": "center",
    "legendColor": "#ffffff",
    "max": 100,
    "min": 0
  },
  "cssClass": "li-col-progress",
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
    "min": 0,
    "max": 100,
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
  "blank": "",
  "zero": 0,
  "accessor": null,
  "accessorParams": {},
  "accessorDownload": null,
  "mutator": null,
  "mutatorParams": {},

  // -- Aggregation --
  "bottomCalc": "avg",
  "bottomCalcFormatter": "progress",
  "bottomCalcFormatterParams": {
    "max": 100,
    "min": 0
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