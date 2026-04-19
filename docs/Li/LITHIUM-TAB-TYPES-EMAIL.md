# Email Column Type

Email address (clickable mailto link).

```json
"email": {
  // -- Core --
  "description": "Email address (clickable mailto link)",

  // -- Layout --
  "hozAlign": "left",
  "vertAlign": "middle",
  "headerHozAlign": null,
  "headerVertical": false,
  "width": null,
  "minWidth": 120,
  "maxWidth": null,
  "frozen": false,
  "resizable": true,
  "responsive": null,

  // -- Display --
  "formatter": "link",
  "formatterParams": {
    "target": "_blank",
    "urlPrefix": "mailto:"
  },
  "cssClass": "li-col-email",
  "cellStyle": null,
  "headerStyle": null,
  "rowStyle": null,
  "rowHandle": false,
  "tooltip": null,
  "headerTooltip": null,
  "download": true,
  "headerWordWrap": false,

  // -- Editing --
  "editor": "input",
  "editorParams": {
    "search": false
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