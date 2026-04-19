# Lithium Column Types

This document describes the column type system used by LithiumTable to define how columns behave. Each column type is stored in Lookup 059 Key 0 ("column-types") and defines display formatting, alignment, editor behavior, sorting, and value-handling rules.

---

## Overview

Column types provide a reusable set of properties that can be applied to columns. The coltypes system supports:

- **Base defaults** — Common properties shared by all types
- **Type-specific overrides** — Individual types with tailored settings

The column resolution process:

1. Start with coltype defaults from the definition
2. Overlay table definition column properties
3. Result is the final column configuration

---

## Type-Specific Documentation

Each column type has its own dedicated document:

| Document | Type | Description |
|----------|------|-------------|
| [LITHIUM-TAB-TYPES-DEFAULT.md](LITHIUM-TAB-TYPES-DEFAULT.md) | Default | Base properties inherited by all types |
| [LITHIUM-TAB-TYPES-STRING.md](LITHIUM-TAB-TYPES-STRING.md) | string | General text/string values |
| [LITHIUM-TAB-TYPES-TEXT.md](LITHIUM-TAB-TYPES-TEXT.md) | text | Long text/multiline content |
| [LITHIUM-TAB-TYPES-INTEGER.md](LITHIUM-TAB-TYPES-INTEGER.md) | integer | Whole numbers (IDs, counts) |
| [LITHIUM-TAB-TYPES-INDEX.md](LITHIUM-TAB-TYPES-INDEX.md) | index | Simple whole numbers (lookup values) |
| [LITHIUM-TAB-TYPES-DECIMAL.md](LITHIUM-TAB-TYPES-DECIMAL.md) | decimal | Floating-point numbers |
| [LITHIUM-TAB-TYPES-BOOLEAN.md](LITHIUM-TAB-TYPES-BOOLEAN.md) | boolean | True/false toggle values |
| [LITHIUM-TAB-TYPES-DATE.md](LITHIUM-TAB-TYPES-DATE.md) | date | Date values (no time) |
| [LITHIUM-TAB-TYPES-DATETIME.md](LITHIUM-TAB-TYPES-DATETIME.md) | datetime | Date + time values |
| [LITHIUM-TAB-TYPES-TIME.md](LITHIUM-TAB-TYPES-TIME.md) | time | Time-only values |
| [LITHIUM-TAB-TYPES-CURRENCY.md](LITHIUM-TAB-TYPES-CURRENCY.md) | currency | Monetary values with currency symbol |
| [LITHIUM-TAB-TYPES-PERCENT.md](LITHIUM-TAB-TYPES-PERCENT.md) | percent | Percentage values |
| [LITHIUM-TAB-TYPES-PROGRESS.md](LITHIUM-TAB-TYPES-PROGRESS.md) | progress | Progress bar (0-100) |
| [LITHIUM-TAB-TYPES-EMAIL.md](LITHIUM-TAB-TYPES-EMAIL.md) | email | Email address (mailto link) |
| [LITHIUM-TAB-TYPES-URL.md](LITHIUM-TAB-TYPES-URL.md) | url | Hyperlink URL (clickable) |
| [LITHIUM-TAB-TYPES-LOOKUP.md](LITHIUM-TAB-TYPES-LOOKUP.md) | lookup | Foreign key to lookup table (text) |
| [LITHIUM-TAB-TYPES-LOOKUPICON.md](LITHIUM-TAB-TYPES-LOOKUPICON.md) | lookupIcon | Lookup with icon only in cell |
| [LITHIUM-TAB-TYPES-LOOKUPICONTEXT.md](LITHIUM-TAB-TYPES-LOOKUPICONTEXT.md) | lookupIconText | Lookup with icon + text everywhere |
| [LITHIUM-TAB-TYPES-LOOKUPICONLIST.md](LITHIUM-TAB-TYPES-LOOKUPICONLIST.md) | lookupIconList | Lookup with icon (cell), icon+text (dropdown) |
| [LITHIUM-TAB-TYPES-ENUM.md](LITHIUM-TAB-TYPES-ENUM.md) | enum | Fixed set of values |
| [LITHIUM-TAB-TYPES-HTML.md](LITHIUM-TAB-TYPES-HTML.md) | html | Rich HTML content |
| [LITHIUM-TAB-TYPES-IMAGE.md](LITHIUM-TAB-TYPES-IMAGE.md) | image | Image thumbnail |
| [LITHIUM-TAB-TYPES-COLOR.md](LITHIUM-TAB-TYPES-COLOR.md) | color | Color swatch |
| [LITHIUM-TAB-TYPES-STAR.md](LITHIUM-TAB-TYPES-STAR.md) | star | Star rating (0-5) |
| [LITHIUM-TAB-TYPES-ROWNUM.md](LITHIUM-TAB-TYPES-ROWNUM.md) | rownum | Auto-incrementing row number |
| [LITHIUM-TAB-TYPES-JSON.md](LITHIUM-TAB-TYPES-JSON.md) | json | JSON object or array |

---

## Quick Reference

A concise summary of all column types with their key properties:

| Type | File | minWidth | formatter | editor | Key Properties |
|------|------|-----------|-----------|--------|-----------------|
| **Basic Types** | | | | | |
| default | [DEFAULT](LITHIUM-TAB-TYPES-DEFAULT.md) | 20 | plaintext | input | Base for all types |
| string | [STRING](LITHIUM-TAB-TYPES-STRING.md) | 80 | plaintext | input | filter: true, groupable: true |
| text | [TEXT](LITHIUM-TAB-TYPES-TEXT.md) | 150 | plaintext | textarea | vertAlign: top |
| integer | [INTEGER](LITHIUM-TAB-TYPES-INTEGER.md) | 40 | number | number | sorter: number, bottomCalc: sum |
| index | [INDEX](LITHIUM-TAB-TYPES-INDEX.md) | 40 | number | number | blank: "0", bottomCalc: count |
| decimal | [DECIMAL](LITHIUM-TAB-TYPES-DECIMAL.md) | 80 | number | number | precision: 2 |
| boolean | [BOOLEAN](LITHIUM-TAB-TYPES-BOOLEAN.md) | 50 | tickCross | tickCross | sorter: boolean |
| **Date/Time Types** | | | | | |
| date | [DATE](LITHIUM-TAB-TYPES-DATE.md) | 90 | datetime | date | sorter: date |
| datetime | [DATETIME](LITHIUM-TAB-TYPES-DATETIME.md) | 120 | datetime | datetime-local | sorter: datetime |
| time | [TIME](LITHIUM-TAB-TYPES-TIME.md) | 60 | datetime | time | sorter: time |
| **Numeric Display** | | | | | |
| currency | [CURRENCY](LITHIUM-TAB-TYPES-CURRENCY.md) | 90 | money | number | symbol: "$", precision: 2 |
| percent | [PERCENT](LITHIUM-TAB-TYPES-PERCENT.md) | 70 | number | number | suffix: "%", bottomCalc: avg |
| progress | [PROGRESS](LITHIUM-TAB-TYPES-PROGRESS.md) | 80 | progress | number | max: 100, bottomCalc: avg |
| **Link Types** | | | | | |
| email | [EMAIL](LITHIUM-TAB-TYPES-EMAIL.md) | 120 | link | input | urlPrefix: "mailto:" |
| url | [URL](LITHIUM-TAB-TYPES-URL.md) | 120 | link | input | target: "_blank" |
| **Lookup Types** | | | | | |
| lookup | [LOOKUP](LITHIUM-TAB-TYPES-LOOKUP.md) | 80 | lookup | list | Requires `lookupRef` on column |
| lookupIcon | [LOOKUPICON](LITHIUM-TAB-TYPES-LOOKUPICON.md) | 50 | icon | list | lookupStyle: "icon" |
| lookupIconText | [LOOKUPICONTEXT](LITHIUM-TAB-TYPES-LOOKUPICONTEXT.md) | 100 | iconText | list | lookupStyle: "iconText" |
| lookupIconList | [LOOKUPICONLIST](LITHIUM-TAB-TYPES-LOOKUPICONLIST.md) | 50 | icon | list | cell: icon, dropdown: icon+text |
| **Special Types** | | | | | |
| enum | [ENUM](LITHIUM-TAB-TYPES-ENUM.md) | 80 | plaintext | list | Requires `values` in editorParams |
| html | [HTML](LITHIUM-TAB-TYPES-HTML.md) | 150 | html | textarea | vertAlign: top |
| image | [IMAGE](LITHIUM-TAB-TYPES-IMAGE.md) | 50 | image | false | Read-only, fixed width 50px |
| color | [COLOR](LITHIUM-TAB-TYPES-COLOR.md) | 40 | color | input | Fixed width 60px |
| star | [STAR](LITHIUM-TAB-TYPES-STAR.md) | 80 | star | star | stars: 5, bottomCalc: avg |
| rownum | [ROWNUM](LITHIUM-TAB-TYPES-ROWNUM.md) | 40 | rownum | false | Read-only, headerSort: false |
| json | [JSON](LITHIUM-TAB-TYPES-JSON.md) | 120 | plaintext | textarea | vertAlign: top |

---

## Base Properties

These properties are available on all column types:

### Core Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `title` | string | `"Column"` | Column header title displayed in the table |
| `field` | string \| null | `null` | Data field name (JSON key) - required for data binding |
| `description` | string | `""` | Description for documentation/Column Manager |
| `visible` | boolean | `true` | Show/hide column in table |

### Layout Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `hozAlign` | string | `"left"` | Horizontal text alignment (`"left"`, `"center"`, `"right"`) |
| `vertAlign` | string | `"middle"` | Vertical text alignment (`"top"`, `"middle"`, `"bottom"`) |
| `headerHozAlign` | string | `null` | Header title horizontal alignment |
| `headerVertical` | boolean | `false` | Rotate header text vertically |
| `width` | number \| null | `null` | Column width in pixels |
| `minWidth` | number | `40` | Minimum column width in pixels |
| `maxWidth` | number \| null | `null` | Maximum column width in pixels |
| `frozen` | boolean \| string | `false` | Freeze position (`true` = left, `"right"` = right) |
| `resizable` | boolean | `true` | Allow column resizing |
| `responsive` | number \| null | `null` | Responsive hide priority (higher = hide first) |

### Display Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `formatter` | string | `"plaintext"` | Cell content formatter name |
| `formatterParams` | object | `{}` | Parameters passed to the formatter |
| `cssClass` | string | `""` | CSS class applied to cells |
| `cellStyle` | string \| object \| function | `null` | Inline CSS for cells |
| `headerStyle` | string \| object | `null` | Inline CSS for header |
| `tooltip` | string \| null | `null` | Cell tooltip content |
| `headerTooltip` | string \| null | `null` | Header tooltip content |
| `download` | boolean | `true` | Include in download/export |
| `headerWordWrap` | boolean | `false` | Allow word wrap in header |

### Row Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `rowStyle` | string \| object \| function | `null` | Inline CSS for entire row |
| `rowHandle` | boolean | `false` | Use column as row handle |

### Editing Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `editor` | string \| boolean | `"input"` | Cell editor name (`false` = not editable) |
| `editorParams` | object | `{}` | Parameters passed to the editor |
| `editable` | boolean \| function | `null` | Conditional editing |
| `validator` | string \| array \| object | `null` | Validation rules |

### Interaction Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `contextMenu` | array \| function | `null` | Right-click menu |
| `headerClickMenu` | array \| function | `null` | Header click menu |
| `cellClick` | function | `null` | Cell click callback |
| `headerClick` | function | `null` | Header click callback |

### Sorting Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `sorter` | string | `"alphanum"` | Sort function name |
| `sorterParams` | object | `{}` | Parameters passed to the sorter |
| `headerSort` | boolean | `true` | Enable sorting on this column |
| `sortPri` | number \| null | `null` | Sort priority for multi-column sorting |

### Filtering Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `headerFilter` | boolean | `false` | Show header filter input |
| `headerFilterFunc` | string | `"like"` | Header filter function |
| `headerFilterPlaceholder` | string | `"filter..."` | Filter input placeholder |

### Grouping Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `groupable` | boolean | `false` | Enable grouping by this column |
| `groupPri` | number \| null | `null` | Group priority (lower = outer group) |
| `groupDir` | string | `"asc"` | Group sort order (`"asc"` or `"desc"`) |

### Column Ordering Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `columnPri` | number \| null | `null` | Display order priority (lower = displayed first) |

### Data Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `blank` | string | `""` | Display value when data is blank/empty |
| `zero` | any | `null` | Display value when data is zero |
| `accessor` | string \| function | `null` | Transform data on read |
| `accessorParams` | object | `{}` | Parameters for accessor |
| `accessorDownload` | string \| function | `null` | Transform data on download |
| `mutator` | string \| function | `null` | Transform data on write |
| `mutatorParams` | object | `{}` | Parameters for mutator |

### Aggregation Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `bottomCalc` | string \| null | `null` | Footer calculation (`"count"`, `"sum"`, `"avg"`, `"min"`, `"max"`) |
| `bottomCalcFormatter` | string \| null | `null` | Formatter for calculation result |
| `bottomCalcFormatterParams` | object | `{}` | Parameters for bottomCalcFormatter |
| `bottomCalcHozAlign` | string \| null | `null` | Footer cell alignment (`"left"`, `"center"`, `"right"`) |
| `bottomCalcStyle` | string \| object \| function | `null` | Inline CSS for footer cell |

---

## Table Definition Properties (Tabledef)

These properties are used in table definitions (Lookup 059, Keys 1+) to define columns. They are not part of the coltype defaults but are used when resolving the final column configuration.

| Property | Type | Description |
|----------|------|-------------|
| `primaryKey` | boolean | Marks this column as the table's primary key |
| `calculated` | boolean | Indicates the value is computed (not stored in DB) |
| `coltype` | string | **Required.** The column type to use (e.g., "string", "integer", "lookup") |

### primaryKey

Marks a column as the database primary key. Typically hidden from display but used for API operations.

```json
"primaryKey": true
```

| Default | Type |
|---------|------|
| `false` | boolean |

### calculated

Indicates the column value is computed at runtime rather than stored in the database. Common for aggregate fields or derived values.

```json
"calculated": true
```

| Default | Type |
|---------|------|
| `false` | boolean |

### coltype (Required)

The column type identifier that determines how the column behaves. This references a type defined in the coltypes system (Lookup 059, Key 0).

```json
"coltype": "string"
```

| Value | Description |
|-------|-------------|
| `"string"` | General text |
| `"integer"` | Whole numbers |
| `"index"` | Simple whole numbers (lookup keys) |
| `"lookup"` | Foreign key to lookup table |
| ... | Any other defined coltype |

**Note:** The `coltype` property is required on every column in a table definition. It references the type definitions in Lookup 059 Key 0.

---

## Property Reference

### hozAlign

Horizontal text alignment within cells.

```json
"hozAlign": "left"
```

| Value | Description |
|-------|-------------|
| `"left"` | Align text to left (default for strings) |
| `"center"` | Center text (default for booleans, dates) |
| `"right"` | Align text to right (default for numbers) |

**Tabulator equivalent:** `hozAlign`

---

### vertAlign

Vertical text alignment within cells.

```json
"vertAlign": "middle"
```

| Value | Description |
|-------|-------------|
| `"top"` | Align to top edge |
| `"middle"` | Center vertically (default) |
| `"bottom"` | Align to bottom edge |

---

### formatter

How cell data is displayed. Uses Tabulator built-in formatters or custom functions.

```json
"formatter": "plaintext"
```

| Value | Description |
|-------|-------------|
| `"plaintext"` | Plain text (default) |
| `"number"` | Number with formatting |
| `"money"` | Currency formatting |
| `"datetime"` | Date/time formatting |
| `"tickCross"` | Boolean checkmark/X |
| `"star"` | Star rating |
| `"progress"` | Progress bar |
| `"image"` | Image thumbnail |
| `"link"` | Clickable hyperlink |
| `"lookup"` | Lookup table resolution |

---

### formatterParams

Parameters passed to the formatter function.

```json
"formatterParams": {
  "precision": 2,
  "thousand": ",",
  "decimal": "."
}
```

---

### editor

How cell data is edited. Set to `false` for read-only.

```json
"editor": "input"
```

| Value | Description |
|-------|-------------|
| `"input"` | Single-line text input (default) |
| `"number"` | Number input with stepper |
| `"textarea"` | Multi-line text area |
| `"list"` | Dropdown list |
| `"select"` | Select dropdown |
| `"date"` | Date picker |
| `"datetime-local"` | Date/time picker |
| `"time"` | Time picker |
| `"tickCross"` | Checkbox toggle |
| `"star"` | Star rating input |
| `false` | Not editable |

---

### editorParams

Parameters passed to the editor.

```json
"editorParams": {
  "search": false,
  "min": 0,
  "max": 100,
  "step": 1
}
```

---

### sorter

How column data is sorted.

```json
"sorter": "alphanum"
```

| Value | Description |
|-------|-------------|
| `"alphanum"` | Alphanumeric (default for strings) |
| `"number"` | Numeric (default for numbers) |
| `"date"` | Date sorting |
| `"datetime"` | Date/time sorting |
| `"time"` | Time sorting |
| `"boolean"` | Boolean (true > false) |

---

### sorterParams

Parameters passed to the sorter.

```json
"sorterParams": {
  "format": "yyyy-MM-dd"
}
```

---

### title

Column header title displayed in the table.

```json
"title": "Status"
```

| Default | Type |
|---------|------|
| `"Column"` | string |

**Note:** This is a core property that should be in every column definition.

---

### field

JSON data field name. This is the key used to access the value from the data row object.

```json
"field": "query_status_a27"
```

| Default | Type |
|---------|------|
| `null` | string |

**Note:** This is a core property. Required for columns that display data. Not required for button/icon columns.

---

### headerSort

Enable or disable clicking the header to sort.

```json
"headerSort": true
```

| Value | Description |
|-------|-------------|
| `true` | Header click sorts (default) |
| `false` | Sorting disabled |

---

### sortPri

Sort priority for multi-column sorting. Lower numbers sort first.

```json
"sortPri": 1
```

| Default | Type |
|---------|------|
| `null` | number \| null |

**Usage:** When the table has multiple sort columns, they are applied in order of `sortPri`.

---

### headerFilter

Show a filter input in the column header.

```json
"headerFilter": true
```

| Value | Description |
|-------|-------------|
| `false` | No filter (default) |
| `true` | Show filter input |

**Note:** This is a simplified boolean. For custom filter behavior, use `headerFilterFunc`.

---

### headerFilterFunc

Filter function for header filter inputs.

```json
"headerFilterFunc": "like"
```

| Value | Description |
|-------|-------------|
| `"="` | Exact match |
| `"!="` | Not equal |
| `"like"` | Contains substring (default for strings) |
| `">="` | Greater than or equal (default for numbers) |
| `">"` | Greater than |
| `"<="` | Less than or equal |
| `"<"` | Less than |

**Custom Filter Types:**

For dropdown-style filters with custom content, use `headerFilterParams` in the column definition:

```json
"headerFilterParams": {
  "valuesLookup": true,
  "valuesLookupField": "status",
  "showRecent": true,
  "recentMax": 10,
  "displayMode": "iconText"
}
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `valuesLookup` | boolean | Load values from a lookup table |
| `valuesLookupField` | string | Field to use for dropdown values |
| `showRecent` | boolean | Show "recently selected" option |
| `recentMax` | number | Maximum recent values to track |
| `displayMode` | string | `"iconText"`, `"text"`, or `"icon"` for dropdown display |

---

### headerFilterPlaceholder

Placeholder text in header filter input.

```json
"headerFilterPlaceholder": "filter..."
```

---

### frozen

Freeze column at left or right edge.

```json
"frozen": false
```

| Value | Description |
|-------|-------------|
| `false` | Not frozen (default) |
| `true` | Freeze to left edge |

---

### resizable

Allow column to be resized by user.

```json
"resizable": true
```

| Value | Description |
|-------|-------------|
| `true` | Resizable (default) |
| `false` | Not resizable |

**Note:** Even when `false`, columns can still be resized programmatically.

---

### maxWidth

Maximum column width in pixels.

```json
"maxWidth": 300
```

| Default | Type |
|---------|------|
| `null` | number \| null |

**Note:** User can still manually resize above this limit unless `resizable` is also `false`.

---

### groupable

Enable grouping by this column. When enabled, the column can be used as a group-by field.

```json
"groupable": true
```

| Value | Description |
|-------|-------------|
| `false` | Not groupable (default) |
| `true` | Column appears in group-by options |

**Usage:** Set `groupable: true` to make a column available for grouping. Then use `groupPri` to set the grouping priority.

---

### groupPri

Group priority. Lower numbers create outer groups (first grouped).

```json
"groupPri": 1
```

| Default | Type |
|---------|------|
| `null` | number \| null |

**Usage:** When grouping by multiple columns:

- `groupPri: 1` = outermost group
- `groupPri: 2` = second level, nested inside group 1

Used with `groupable: true` to enable grouping.

---

### groupDir

Group sort direction within each group.

```json
"groupDir": "asc"
```

| Value | Description |
|-------|-------------|
| `"asc"` | Sort ascending (default) |
| `"desc"` | Sort descending |

**Usage:** Controls how rows are sorted within each group.

---

### columnPri

Column display priority. Lower numbers are displayed first (leftmost).

```json
"columnPri": 1
```

| Default | Type |
|---------|------|
| `null` | number \| null |

**Usage:** Used to control column order without manually reordering the columns array. Columns with `null` are displayed after those with priority values.

---

### visible

Show or hide the column in the table.

```json
"visible": true
```

| Value | Description |
|-------|-------------|
| `true` | Column is visible (default) |
| `false` | Column is hidden |

**Note:** This is a core property used for column visibility toggle.

---

### headerVertical

Rotate header text to display vertically.

```json
"headerVertical": true
```

| Value | Description |
|-------|-------------|
| `false` | Horizontal header (default) |
| `true` | Vertical header text |

**Usage:** Use for narrow columns with long titles.

---

### headerHozAlign

Header title horizontal alignment.

```json
"headerHozAlign": "center"
```

| Value | Description |
|-------|-------------|
| `"left"` | Align to left |
| `"center"` | Center (default) |
| `"right"` | Align to right |

**Note:** Independent of cell `hozAlign`.

---

### responsive

Responsive hide priority. Higher numbers hide first when table is narrow.

```json
"responsive": 1
```

| Default | Type |
|---------|------|
| `null` | number \| null |

**Usage:** Set on columns to control collapse order when table width is reduced.

---

### rowHandle

Use this column as the row selection/move handle.

```json
"rowHandle": true
```

| Value | Description |
|-------|-------------|
| `false` | Not a handle (default) |
| `true` | Column is row handle |

**Usage:** For movable rows or row selection checkboxes.

---

### blank

Display value when cell data is empty/blank.

```json
"blank": ""
```

| Default | Type |
|---------|------|
| `""` | string |
| `null` | any |

---

### zero

Display value when cell data is zero (for numeric types).

```json
"zero": "0.00"
```

| Default | Type |
|---------|------|
| `null` | any |
| `"0"` | string (for integers) |
| `"0.00"` | string (for decimals) |
| `false` | boolean (for boolean) |

---

### cssClass

CSS class applied to column cells.

```json
"cssClass": "li-col-string"
```

---

### cellStyle

Inline CSS style applied to cells. Can be string or object.

```json
"cellStyle": "color: red; background: #fff0f0"
```

```json
"cellStyle": { "color": "red", "background": "#fff0f0" }
```

**Usage:** Dynamic styling based on cell values. Can also reference field values using function.

---

### headerStyle

Inline CSS style applied to header cells.

```json
"headerStyle": "font-weight: bold; background: #f0f0f0"
```

---

### download

Include column in download and clipboard operations.

```json
"download": true
```

| Value | Description |
|-------|-------------|
| `true` | Include in downloads (default) |
| `false` | Exclude from downloads |

---

### headerWordWrap

Allow word wrapping in column header text.

```json
"headerWordWrap": true
```

| Value | Description |
|-------|-------------|
| `false` | Truncate with ellipsis (default) |
| `true` | Wrap header text |

**Note:** Requires CSS: `.tabulator-col-title { white-space: normal; }`

---

### rowStyle

Inline CSS applied to entire row based on cell values.

```json
"rowStyle": "background: #fff0f0"
```

```json
"rowStyle": { "background": "#fff0f0" }
```

```json
"rowStyle": function(cell, data) {
  // Dynamic styling based on row data
  if (data.status === 0) {
    return { "opacity": "0.5", "background": "#f5f5f5" };
  }
  if (data.priority === 1) {
    return { "background": "#fff0f0" };
  }
  return null;
}
```

| Default | Type |
|---------|------|
| `null` | string \| object \| function \| null |

**Usage:** Highlight rows based on status, priority, or any condition:

```json
// Example: highlight inactive rows
{
  "field": "status",
  "rowStyle": function(cell, data) {
    return data.status_a27 === 0 
      ? { "opacity": "0.6", "color": "#999" }
      : null;
  }
}
```

**CSS Classes vs rowStyle:** Use `rowStyle` for dynamic condition-based styling. For static styling, use table-level row CSS classes.

---

### editable

Conditional cell editing. Set to function for dynamic control.

```json
"editable": true
```

```json
"editable": function(cell, value, data) {
  return data.status === 'active'; // only allow editing when status is active
}
```

| Default | Type |
|---------|------|
| `null` | boolean \| function \| null |

**Note:** `null` means use `editor` property - if editor is set, cell is editable.

---

### contextMenu

Right-click context menu for cells.

```json
"contextMenu": [
  { label: "Copy", action: function() { ... } },
  { label: "Edit", action: function() { ... } }
]
```

| Default | Type |
|---------|------|
| `null` | array \| function \| null |

---

### headerClickMenu

Menu shown when clicking column header.

```json
"headerClickMenu": [
  { label: "Sort Asc", action: function() { ... } },
  { label: "Sort Desc", action: function() { ... } }
]
```

---

### cellClick

Callback when cell is clicked.

```json
"cellClick": function(e, cell) {
  console.log("Clicked:", cell.getValue());
}
```

---

### headerClick

Callback when header is clicked.

```json
"headerClick": function(e, column) {
  console.log("Header clicked:", column.getField());
}
```

---

### bottomCalc

Footer calculation type.

```json
"bottomCalc": "sum"
```

| Value | Description |
|-------|-------------|
| `null` | No calculation |
| `"count"` | Count of rows |
| `"sum"` | Sum of values |
| `"avg"` | Average of values |
| `"min"` | Minimum value |
| `"max"` | Maximum value |

---

### bottomCalcFormatter

Formatter for footer calculation result.

```json
"bottomCalcFormatter": "number"
```

---

### bottomCalcFormatterParams

Parameters for footer calculation formatter.

```json
"bottomCalcFormatterParams": {
  "precision": 2,
  "thousand": ","
}
```

---

### bottomCalcHozAlign

Footer cell horizontal alignment.

```json
"bottomCalcHozAlign": "right"
```

| Value | Description |
|-------|-------------|
| `"left"` | Align to left |
| `"center"` | Center |
| `"right"` | Align to right (default for numeric columns) |
| `null` | Use column `hozAlign` |

**Usage:** Aligns the footer calculation result independently of the column alignment.

---

### bottomCalcStyle

Inline CSS applied to footer calculation cell.

```json
"bottomCalcStyle": "font-weight: bold; color: #333"
```

```json
"bottomCalcStyle": { "font-weight": "bold", "color": "#333" }
```

```json
"bottomCalcStyle": function(value) {
  // Dynamic styling based on calculated value
  if (value > 1000) {
    return { "color": "#d00", "font-weight": "bold" };
  }
  return null;
}
```

| Default | Type |
|---------|------|
| `null` | string \| object \| function \| null |

**Usage:** Style the footer cell differently based on the calculation result.

---

### accessor

Transform data as it is read into the table.

```json
"accessor": null
```

**Usage:** Transform incoming data before display.

---

### accessorParams

Parameters for accessor function.

```json
"accessorParams": {}
```

---

### accessorDownload

Transform data as it is downloaded/copied.

```json
"accessorDownload": null
```

---

### mutator

Transform data as it is written to the table.

```json
"mutator": null
```

**Usage:** Compute derived fields or normalize input data.

---

### mutatorParams

Parameters for mutator function.

```json
"mutatorParams": {}
```

---

## Special Properties for Lookup/Enum Types

These properties are used by column types that reference lookup tables or enum lists.

### lookupRef

Lookup table reference identifier.

```json
"lookupRef": "a27"
```

| Format | Description |
|--------|-------------|
| `"a27"` | Lookup ID with prefix |
| `"27"` | Numeric lookup ID |
| `"status"` | Named lookup |

**Usage:** Identifies the lookup table for `lookup`, `lookupIcon`, `lookupIconText`, `lookupIconList` types. The lookup table contains records with `key_idx`, `value_txt`, `value_int`, and optional JSON (commonly icon data or translations).

---

### lookupStyle

How lookup values are displayed in table cells.

```json
"lookupStyle": "iconText"
```

| Value | Cell Display |
|-------|-------------|
| `"iconText"` | Icon + text label (default) |
| `"icon"` | Icon only |
| `"text"` | Text only |

**Usage:** Controls what the user sees in the table cell. For example, a status column might show just a checkmark icon for enabled, but the text "Active" would also be available.

---

### lookupEdit

How lookup values are displayed in the edit dropdown.

```json
"lookupEdit": "iconText"
```

| Value | Dropdown Display |
|-------|-------------|
| `"iconText"` | Icon + text label (default) |
| `"icon"` | Icon only |
| `"text"` | Text only |

**Usage:** Controls the edit dropdown. This can differ from `lookupStyle`. A column might display icons only in cells but show icons + text in the dropdown for clarity.

**Example - Status column:**

```json
{
  "lookupRef": "status",
  "lookupStyle": "icon",      // Cell shows: ✓ or ✗
  "lookupEdit": "iconText"    // Dropdown shows: "✓ Enabled", "✗ Disabled"
}
```

---

### itemFormatter

Function to format lookup items in dropdowns.

```json
"itemFormatter": "iconOnly"
```

| Value | Description |
|-------|-------------|
| `"iconOnly"` | Show icon only |
| `"iconWithText"` | Show icon + text |
| `"textOnly"` | Show text only |

**Note:** This is a lower-level property. Use `lookupStyle` and `lookupEdit` instead for most cases.

---

### values

Fixed list of values for enum types.

```json
"values": ["active", "inactive", "pending"]
```

| Type | Description |
|------|-------------|
| array | Array of string values |
| object | `{ value: label }` map |

**Usage:** Defines the options for `enum` coltype. For dropdown-style filtering, use `headerFilterParams`.

---

### headerFilterParams

Parameters for custom header filters.

```json
"headerFilterParams": {
  "valuesLookup": true,
  "valuesLookupField": "status",
  "showRecent": true,
  "recentMax": 10,
  "displayMode": "iconText",
  "itemFormatter": "iconWithText"
}
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `valuesLookup` | boolean | Load values from a lookup table |
| `valuesLookupField` | string | Field to populate dropdown |
| `showRecent` | boolean | Show recently selected values |
| `recentMax` | number | Max recent values to show |
| `displayMode` | string | Display mode |
| `itemFormatter` | string | Item formatter |

**Usage:** Enables dropdown filtering with custom value sources.

---

### tooltip

Tooltip displayed on cell hover.

```json
"tooltip": null
```

**Format:** String or HTML content.

---

### headerTooltip

Tooltip displayed on header cell hover.

```json
"headerTooltip": null
```

---

### validator

Cell validation rules.

```json
"validator": null
```

**Formats:**

```json
// Single validator
"validator": "required"

// With parameters
"validator": { "type": "max", "parameters": 100 }

// Multiple validators
"validator": ["required", "max:100"]
```

---

## Table-Level vs Column-Level Properties

Some properties work at the table level while others are column-level. This section clarifies the relationship.

### Grouping

Tabulator uses **table-level** `groupBy` to define grouping:

```javascript
groupBy: "status"  // Single field
groupBy: ["status", "priority"]  // Multiple fields (nested groups)
```

Lithium extends this with **column-level** properties:

| Column Property | Purpose |
|-----------------|----------|
| `groupable` | Enable this column as grouping option |
| `groupPri` | Priority for sorting multiple groups |
| `groupDir` | Sort direction within group |

**How it works:**

1. Set `groupable: true` on columns that can be grouped
2. Set `groupPri` to determine grouping order
3. At runtime, the system builds `groupBy` array from groupable columns sorted by `groupPri`
4. Table-level `groupBy` is controlled by this array

### Sorting

Similarly, `sortPri` extends table-level sorting:

| Table Property | Column Property | Purpose |
|----------------|-----------------|----------|
| `sort` (initial) | `sortPri` | Sort priority |
| `headerSort` | — | Enable clicking to sort |

**How it works:**

1. Set `sortPri` on columns that should have initial sort
2. Lower values sort first
3. System builds multi-column sort from columns with `sortPri` set

### Column Visibility and Order

| Property | Purpose |
|-----------|----------|
| `visible` | Show/hide column (handled at runtime) |
| `columnPri` | Display priority |

Columns are sorted by `columnPri` (null values last) to determine display order.

---

## Machine-Readable Schema

For programmatic access, validation, and code generation, a JSON Schema is available:

**[LITHIUM-TAB-TYPES.schema.json](LITHIUM-TAB-TYPES.schema.json)**

This schema defines all 25 column types with their properties and can be used to:

- Validate column definitions
- Generate type definitions for TypeScript
- Enable autocomplete in JSON editors

---

## Related Documentation

| Document | Description |
|----------|-------------|
| [LITHIUM-TAB.md](LITHIUM-TAB.md) | LithiumTable component |
| [LITHIUM-COL.md](LITHIUM-COL.md) | Column Manager |
| [LITHIUM-LUT.md](LITHIUM-LUT.md) | Lookup Tables |

---

Last updated: April 2026