# Image Column Type

Image thumbnail (URL stored in field).

```json
"image": {
  // -- Core --
  "description": "Image thumbnail (URL stored in field)",

  // -- Layout --
  "hozAlign": "center",
  "vertAlign": "middle",
  "headerHozAlign": null,
  "headerVertical": false,
  "width": 50,
  "minWidth": 50,
  "maxWidth": null,
  "frozen": false,
  "resizable": true,
  "responsive": null,

  // -- Display --
  "formatter": "image",
  "formatterParams": {
    "height": "32px",
    "urlPrefix": "",
    "urlSuffix": "",
    "width": "32px"
  },
  "cssClass": "li-col-image",
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

---

**Related Documentation:**
- [LITHIUM-TAB-TYPES.md](LITHIUM-TAB-TYPES.md) — Column properties reference
- [LITHIUM-TAB.md](LITHIUM-TAB.md) — LithiumTable component