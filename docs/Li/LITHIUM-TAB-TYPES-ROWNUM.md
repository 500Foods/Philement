# RowNum Column Type

Auto-incrementing row number (display only, not from data).

```json
"rownum": {
  // -- Core --
  "description": "Auto-incrementing row number (display only, not from data)",

  // -- Layout --
  "hozAlign": "right",
  "vertAlign": "middle",
  "headerHozAlign": null,
  "headerVertical": false,
  "width": 50,
  "minWidth": 40,
  "maxWidth": null,
  "frozen": false,
  "resizable": true,
  "responsive": null,

  // -- Display --
  "formatter": "rownum",
  "formatterParams": {},
  "cssClass": "li-col-rownum",
  "cellStyle": null,
  "headerStyle": null,
  "rowStyle": null,
  "rowHandle": false,
  "tooltip": null,
  "headerTooltip": null,
  "download": true,
  "headerWordWrap": false,

  // -- Editing --
  "editor": false,
  "editorParams": {},
  "editable": false,
  "validator": null,

  // -- Sorting --
  "sorter": "number",
  "sorterParams": {},
  "headerSort": false,
  "sortPri": null,

  // -- Filtering --
  "headerFilter": false,
  "headerFilterFunc": null,
  "headerFilterPlaceholder": "filter...",
  "headerWordWrap": false,

  // -- Grouping --
  "groupable": false,
  "groupPri": null,
  "groupDir": "asc",
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
  "bottomCalc": "count",
  "bottomCalcFormatter": "number",
  "bottomCalcFormatterParams": {
    "thousand": ","
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