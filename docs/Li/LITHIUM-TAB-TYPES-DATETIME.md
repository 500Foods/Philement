# DateTime Column Type

Date + time values (timestamps).

```json
"datetime": {
  // -- Core --
  "description": "Date + time values (timestamps)",

  // -- Layout --
  "hozAlign": "center",
  "vertAlign": "middle",
  "headerHozAlign": null,
  "headerVertical": false,
  "width": 150,
  "minWidth": 120,
  "maxWidth": null,
  "frozen": false,
  "resizable": true,
  "responsive": null,

  // -- Display --
  "formatter": "datetime",
  "formatterParams": {
    "inputFormat": "yyyy-MM-dd'T'HH:mm:ss",
    "outputFormat": "yyyy-MM-dd HH:mm",
    "invalidPlaceholder": ""
  },
  "cssClass": "li-col-datetime",
  "cellStyle": null,
  "headerStyle": null,
  "rowStyle": null,
  "rowHandle": false,
  "tooltip": null,
  "headerTooltip": null,
  "download": true,
  "headerWordWrap": false,

  // -- Editing --
  "editor": "datetime-local",
  "editorParams": {},
  "editable": false,
  "validator": null,

  // -- Sorting --
  "sorter": "datetime",
  "sorterParams": {
    "format": "yyyy-MM-dd'T'HH:mm:ss"
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