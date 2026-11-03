# LithiumProperties

A two-column Tabulator property grid in
[`src/properties/lithium-properties.js`](/elements/003-lithium/src/properties/lithium-properties.js),
with section grouping in
[`src/properties/sections.js`](/elements/003-lithium/src/properties/sections.js).
The Reports manager is the first user. Its rows come from
[`report-properties.js`](/elements/003-lithium/src/managers/reports/report-properties.js),
and it is mounted in
[`reports-panel.js`](/elements/003-lithium/src/managers/reports/reports-panel.js).

## Rows

`setRows(rows)` takes plain objects:

| Field | Meaning |
| --- | --- |
| `id` | A stable row id. Preferences are keyed by it. |
| `label` | The name column. |
| `value` | The current value. |
| `omitted` | True if the value is the default and not stored. It shows in italics. |
| `note` | The value tooltip. |
| `editor` | See Editors. |
| `list` | The choices for `List`. |
| `section`, `group` | The section id (lookup 075 `key_idx`) and the fallback group caption. |
| `readOnly` | The row does not edit. Tab skips it. |

`setRows` skips the update while a cell is being edited, and it remembers
the last rows so a preference change can repaint them.

## Sections

`setSections([{ id, label, sortSeq, icon }])` sets the section order.
Sections sort by `sortSeq`. Inside a section, rows keep the order the
caller gave (lookup `sort_seq`), unless the user saved an order of their
own. Rows with no section land in **Other**. The header shows a caret,
the icon, the caption, and a count, and the whole header toggles the
section.

## Editors

| Editor | Control |
| --- | --- |
| `Text` | Text input. |
| `Number` | Text input with a decimal keyboard. Lengths take a unit. |
| `List` | Select from `list`. |
| `Boolean` | Yes / No select (`'True'` / `'False'`). |
| `PageSize` | Select from the caller's `pageSizes()` callback. |
| `Color` | Coloris, hex, no alpha. |
| `Date` | Text input plus a calendar button on `shared/flatpickr-picker.js`. It stores `YYYY-MM-DD`. |
| `None` | Read only. |

Enter commits, Escape cancels, and Tab / Shift+Tab move to the next or
previous editable value.

## The resolver

The caller turns its model into rows and applies edits. Reports uses
`panelRows(selection, ...)` to build rows from lookups 053, 054, and 075,
and `onEdit(id, value)` writes the value back through the document model
(one undo step). The component never touches the model.

## Chrome

- Top bar: Search and Outline toggles, then Expand all and Collapse all.
- Body: the search box, the caller's outline (resizable), then the grid.
- Bottom bar: the **Favourites** and **Hidden** filters, then Previous
  and Next object (`onNavigate`).

## Preferences

- **Favourite / Hidden.** Right-click a row for a menu with two
  checkable items, **Favourite** and **Hidden**. Rows always stay in
  their own section.
  - Hidden rows drop out of the grid. Turning **Hidden** on includes
    them in the regular list again, so you can uncheck them. It does
    not filter to hidden rows only.
  - The **Favourites** filter shows only favourite rows. With Hidden
    off, hidden favourites stay out. With both on, you see every
    favourite, hidden or not.
  - Hovering the Favourites button shows a star, right-justified in
    the name cell, on each favourite row. Hovering the Hidden button,
    or turning its filter on, shows an eye-slash on hidden rows. Nothing
    else marks a favourite or a hidden row.
- **Order.** Drag a row by its name to move it inside its section. The
  new order is saved for that section. A drop in another section snaps
  back. Right-click and choose **Reset order** to go back to the lookup
  `sort_seq` order. Rows that are not in a saved order follow it in
  lookup order.
- **Layout.** Open and closed sections, the name width, the outline
  height, the Search and Outline toggles, and the panel (splitter)
  width.

`getState()` returns all of it:

```json
{
  "showSearch": false, "showOutline": true,
  "nameWidth": 120, "outlineHeight": 140, "panelWidth": 280,
  "closedSections": ["Fonts"],
  "favourites": ["width"], "hidden": ["tag"],
  "order": { "Object": ["height", "width"] },
  "showHidden": false, "favouritesOnly": false
}
```

The constructor takes the same object as `state`, and `onStateChange`
gets every change (not the initial setup). The caller stores it.
`setPanelWidth(px)` adds the caller's splitter width to the state.

Reports stores it on the user profile, `window.lithiumSettings`
(GlobalSettingsService, synced to the account), at
`24.props.<panel id>`. The Reports panel id is `designer`. On first
load, the older localStorage values (`lithium_reports_props_prefs` and
`lithium_reports_props_width`) are copied onto the profile and removed.
With no profile service, prefs fall back to localStorage.

## Adopting it in another manager

1. Add a host element and, if you want one, an outline element.
2. Build rows with stable ids and give sections from your lookup.
3. Mount it:

   ```js
   const prefs = loadPrefs();              // your profile path
   const props = new LithiumProperties({
     element, outline, state: prefs,
     onEdit: (id, value) => applyEdit(id, value),
     onStateChange: (state) => savePrefs(state),
     onNavigate: (delta) => stepSelection(delta),
   });
   props.setSections(sections);
   props.setRows(rows);
   ```

4. Store prefs on the profile at `<manager id>.props.<panel id>`. Use a
   new panel id for each panel, so their preferences do not collide.
5. Call `destroy()` when the manager closes.

The panel CSS is in `reports.css` today, under `.lithium-properties`.
Move it to a shared stylesheet when a second manager adopts it.
