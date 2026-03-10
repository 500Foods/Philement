# Lithium Tabulator Component

This document describes the standard Tabulator component used throughout Lithium, specifically focusing on the integrated "Navigator" block.

---

## Overview

Tabulator is the primary data grid library used in Lithium. While powerful, it requires significant configuration to achieve a consistent and usable state across different managers. To address this, we are developing a standardized, reusable component that combines a Tabulator instance with a custom "Navigator" control bar.

This component will be modular, accepting parameters such as JSON data, event callbacks for updates, and configuration options.

---

## The Navigator Block

The Navigator is a control bar positioned below (or above, depending on layout) the Tabulator grid. It provides standard functions for interacting with the table data, especially for read-write tables where rows or values can be edited.

### Navigator Controls

The Navigator consists of several distinct groups of controls:

#### 1. Pagination Controls

Buttons for navigating through pages of data:

- **First Page** (`|<`)
- **Previous Page** (`<`)
- **Next Page** (`>`)
- **Last Page** (`>|`)

#### 2. Action Controls

Buttons for modifying the table data:

- **Add:** Create a new row.
- **Delete:** Remove the selected row(s).
- **Copy:** Duplicate the selected row.
- **Save:** Commit changes made to the table.
- **Cancel:** Revert unsaved changes.
- **Refresh:** Reload data from the source.

#### 3. View Controls

Buttons for adjusting the table's display size/density:

- **Small (sm):** Compact view.
- **Medium (md):** Standard view.
- **Large (lg):** Spacious view.
- **Fit:** Adjust columns to fit the container width.

#### 4. Search/Filter

- A text input field for searching table contents.
- A corresponding "Search" or "Clear" button.

---

## Component Architecture

The goal is to create a reusable class or module that encapsulates both the Tabulator initialization and the Navigator UI.

### Proposed Structure

```javascript
// Conceptual example
class LithiumTable {
  constructor(containerId, options) {
    this.container = document.getElementById(containerId);
    this.options = options; // Data, columns, callbacks, etc.
    
    this.initUI();
    this.initTabulator();
    this.bindEvents();
  }

  initUI() {
    // Create the DOM structure for the table container and the Navigator block
  }

  initTabulator() {
    // Initialize Tabulator with standard Lithium defaults + custom options
  }

  bindEvents() {
    // Wire up Navigator buttons to Tabulator API methods
    // (e.g., Add button -> table.addRow(), Save button -> emit save event)
  }
}
```

### Key Features

- **Modularity:** Easily instantiated in any manager.
- **Consistency:** Ensures all tables look and behave the same way.
- **Read/Write Support:** Handles both display-only and editable grids.
- **Event-Driven:** Emits standard events (e.g., `rowSelected`, `dataChanged`, `saveRequested`) for the parent manager to handle.

---

## Implementation Notes

- The Navigator UI should be styled using standard Lithium CSS variables and icons (Font Awesome).
- Consider how the Navigator adapts to smaller screens (responsive design).
- The component should handle loading states (spinners) during data fetch or save operations.
