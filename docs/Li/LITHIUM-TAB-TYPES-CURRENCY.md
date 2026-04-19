# Currency Column Type

Monetary values with currency symbol.

```json
"currency": {
  // -- Core --
  "description": "Monetary values with currency symbol",

  // -- Layout --
  "hozAlign": "right",
  "vertAlign": "middle",
  "headerHozAlign": null,
  "headerVertical": false,
  "width": null,
  "minWidth": 90,
  "maxWidth": null,
  "frozen": false,
  "resizable": true,
  "responsive": null,

  // -- Display --
  "formatter": "money",
  "formatterParams": {
    "thousand": ",",
    "decimal": ".",
    "symbol": "$",
    "symbolAfter": false,
    "precision": 2
  },
  "cssClass": "li-col-currency",
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
    "max": null,
    "step": 0.01
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
  "zero": "$0.00",
  "accessor": null,
  "accessorParams": {},
  "accessorDownload": null,
  "mutator": null,
  "mutatorParams": {},

  // -- Aggregation --
  "bottomCalc": "sum",
  "bottomCalcFormatter": "money",
  "bottomCalcFormatterParams": {
    "thousand": ",",
    "decimal": ".",
    "symbol": "$",
    "precision": 2
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