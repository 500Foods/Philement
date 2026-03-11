# Lithium Query Manager

This document outlines the design and implementation plan for the Query Manager module in Lithium.

---

## Overview

The Query Manager is a standard Menu Manager (ID: 5) that provides an interface for viewing, editing, and managing database queries. It follows the standard manager layout with headers and footers, similar to the Session Log Manager.

---

## UI Layout

The main UI is divided into two primary sections:

### Left Panel: Query List (Tabulator)

- A relatively narrow panel containing a Tabulator table.
- Displays a list of available queries.
- Selecting a row in this table populates the right panel with the query's details.
- Utilizes the standard Lithium Tabulator component with a custom navigator (see [LITHIUM-TAB.md](LITHIUM-TAB.md)).

### Right Panel: Query Details (Tabbed Interface)

A tabbed interface displaying the details of the selected query.

**Tabs:**

1. **SQL Editor:** A CodeMirror instance configured for SQL syntax highlighting to edit the query string.
2. **JSON Editor:** An editor for viewing/modifying the query's JSON configuration or parameters.
3. **Markdown Editor:** A CodeMirror instance for editing query documentation or notes in Markdown format.
4. **Rich Text Editor (SunEditor):** A WYSIWYG editor (SunEditor) for formatted text, potentially for previewing or editing rich descriptions (TBD).

---

## Implementation Plan (To-Do List)

### Phase 1: Foundation & Components

- [ ] Create the standard Tabulator + Navigator component (as defined in `LITHIUM-TAB.md`).
- [ ] Set up the basic manager structure (`src/managers/query-manager/`).
- [ ] Register the Query Manager in `app.js` (ID: 5) and add its icon to `main.js`.

### Phase 2: UI Layout & Left Panel

- [ ] Implement the split-pane layout (left table, right tabs).
- [ ] Integrate the standard Tabulator component into the left panel.
- [ ] Fetch and display mock/real query data in the table.
- [ ] Implement row selection logic to trigger updates in the right panel.

### Phase 3: Right Panel Tabs

- [ ] Implement the tabbed interface container.
- [ ] **SQL Tab:** Integrate CodeMirror with SQL mode.
- [ ] **JSON Tab:** Integrate the JSON editor.
- [ ] **Markdown Tab:** Integrate CodeMirror with Markdown mode.
- [ ] **Rich Text Tab:** Integrate SunEditor (preview or edit mode).
- [ ] Wire up data binding so selecting a row populates all tabs correctly.

### Phase 4: Polish & Integration

- [ ] Implement save/update logic for modified queries.
- [ ] Ensure responsive design (handling the split pane on smaller screens).
- [ ] Add necessary event bus communication.
- [ ] Final testing and refinement.

---

## Implementation Notes

### QueryRef 27 - Get System Query

When fetching full query details (QueryRef 27), pass `QUERYID` (the query's unique database identifier) rather than `QUERY_REF` (the display reference number):

```javascript
// CORRECT - uses query_id from the selected row
const queryDetails = await authQuery(api, 27, {
  INTEGER: { QUERYID: queryData.query_id },
});

// INCORRECT - was previously using query_ref (the display reference number)
const queryDetails = await authQuery(api, 27, {
  INTEGER: { QUERY_REF: queryData.query_ref },
});
```

The table displays `query_ref` (the human-readable reference like "001"), but the API requires `query_id` (the database primary key) to fetch the full query details.
