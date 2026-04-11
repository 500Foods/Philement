# Date Column Type

Date values (no time component).

```json
"date": {
  // -- Core --
  "description": "Date values (no time component)",

  // -- Layout --
  "hozAlign": "center",
  "vertAlign": "middle",
  "headerHozAlign": null,
  "headerVertical": false,
  "width": 110,
  "minWidth": 90,
  "maxWidth": null,
  "frozen": false,
  "resizable": true,
  "responsive": null,

  // -- Display --
  "formatter": "datetime",
  "formatterParams": {
    "inputFormat": "yyyy-MM-dd",
    "outputFormat": "yyyy-MM-dd",
    "invalidPlaceholder": ""
  },
  "cssClass": "li-col-date",
  "cellStyle": null,
  "headerStyle": null,
  "rowStyle": null,
  "rowHandle": false,
  "tooltip": null,
  "headerTooltip": null,
  "download": true,
  "headerWordWrap": false,

  // -- Editing --
  "editor": "date",
  "editorParams": {},
  "editable": false,
  "validator": null,

  // -- Sorting --
  "sorter": "date",
  "sorterParams": {
    "format": "yyyy-MM-dd"
  },
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

---

**Related Documentation:**
- [LITHIUM-TAB-TYPES.md](LITHIUM-TAB-TYPES.md) — Column properties reference
- [LITHIUM-TAB.md](LITHIUM-TAB.md) — LithiumTable component