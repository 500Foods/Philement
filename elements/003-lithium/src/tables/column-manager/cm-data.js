/**
 * Column Manager - Data Module
 *
 * Column data loading and mapping from parent table.
 *
 * @module tables/column-manager/cm-data
 */

import { log, Subsystems, Status } from '../../core/log.js';

/**
 * Load column data from the parent table
 * @param {Object} cm - ColumnManager instance
 */
export async function loadColumnData(cm) {
  if (!cm.parentTable?.table) {
    log(Subsystems.TABLE, Status.WARN, '[ColumnManager] Parent table not available');
    return;
  }

  try {
    const columns = cm.parentTable.table.getColumns();
    log(Subsystems.TABLE, Status.DEBUG, `[ColumnManager] Found ${columns.length} columns in parent table`);

    cm.originalColumns = columns.map((col) => {
      const def = col.getDefinition();
      const element = col.getElement();

      // Get actual width from the DOM element if available, fallback to definition
      let actualWidth = def.width || null;
      if (element && element.offsetWidth !== undefined) {
        try {
          const computedWidth = element.offsetWidth;
          // Only use computed width if it's reasonable (not collapsed/minimized)
          if (computedWidth && computedWidth > 20) {
            actualWidth = computedWidth;
          }
        } catch {
          log(Subsystems.TABLE, Status.DEBUG,
            `[ColumnManager] Could not get computed width for ${col.getField()}, using definition`);
        }
      }

      // Handle bottomCalc - ensure it's a string value, not a function
      let summaryValue = 'none';
      if (def.bottomCalc) {
        if (typeof def.bottomCalc === 'string') {
          summaryValue = def.bottomCalc;
        } else if (typeof def.bottomCalc === 'function') {
          summaryValue = def.bottomCalc.name || 'none';
        }
      }

      return {
        field: col.getField(),
        title: def.title || col.getField(),
        visible: col.isVisible(),
        coltype: def.lithiumColtype || def.coltype || 'string',
        bottomCalc: summaryValue,
        hozAlign: def.hozAlign || 'left',
        editable: def.lithiumEditable ?? (def.editable === true),
        editor: def.editor || null,
        sorter: def.sorter || null,
        formatter: def.formatter || null,
        width: actualWidth,
        minWidth: def.minWidth || null,
        maxWidth: def.maxWidth || null,
        frozen: def.frozen || false,
        resizable: def.resizable !== false,
        sortable: def.headerSort !== false,
        filterable: !!def.headerFilter,
        category: def.category || 'Default',
      };
    });

    // Convert to column manager data format
    cm.columnData = cm.originalColumns.map((col, index) => ({
      id: index + 1,
      order: index + 1,
      drag_handle: '', // Formatter renders the icon; data value unused
      visible: col.visible,
      editable: col.editable !== false,
      field_name: col.field,
      column_name: col.title,
      format: col.coltype,
      summary: col.bottomCalc,
      alignment: col.hozAlign || 'left',
      width: col.width,
      category: col.category || 'Default',
      field: col.field,
    }));

    log(Subsystems.TABLE, Status.DEBUG, `[ColumnManager] Loaded ${cm.columnData.length} column records`);
  } catch (err) {
    log(Subsystems.TABLE, Status.ERROR, `[ColumnManager] Error loading column data: ${err.message}`);
    cm.columnData = [];
  }
}

/**
 * Refresh column data from parent table (preserving column order)
 * @param {Object} cm - ColumnManager instance
 */
export async function refreshColumnData(cm) {
  if (!cm.parentTable?.table) return;

  // Get current column order from parent table
  const columns = cm.parentTable.table.getColumns();
  const currentOrder = columns
    .filter((col) => col.getField() !== '_selector')
    .map((col) => col.getField());

  // Load fresh column data
  await loadColumnData(cm);

  // Reorder column data to match parent table's current order
  if (currentOrder.length > 0) {
    const reorderedData = [];
    currentOrder.forEach((field, index) => {
      const colData = cm.columnData.find((c) => c.field_name === field);
      if (colData) {
        reorderedData.push({
          ...colData,
          order: index + 1,
        });
      }
    });

    // Add any columns not in currentOrder at the end
    cm.columnData.forEach((colData) => {
      if (!currentOrder.includes(colData.field_name)) {
        reorderedData.push({
          ...colData,
          order: reorderedData.length + 1,
        });
      }
    });

    cm.columnData = reorderedData;
  }

  // Update the column manager table if it exists
  if (cm.columnTable?.table) {
    await cm.columnTable.table.replaceData(cm.columnData);
  }
}

/**
 * Get column group for grouping support
 * @param {string} field - Field name
 * @returns {string} Group name
 */
export function getColumnGroup(field) {
  if (field.includes('id') || field.includes('ID')) return 'Identifiers';
  if (field.includes('name') || field.includes('Name')) return 'Names';
  if (field.includes('date') || field.includes('time')) return 'Dates';
  if (field.includes('status') || field.includes('state')) return 'Status';
  return 'General';
}

/**
 * Capture parent table state as the new baseline
 * @param {Object} cm - ColumnManager instance
 */
export function captureParentStateAsOriginal(cm) {
  if (!cm.parentTable?.table) return;
  cm.originalColumns = cm.parentTable.table.getColumns().map((col) => {
    const def = col.getDefinition();
    const element = col.getElement();
    let actualWidth = def.width || null;
    if (element && element.offsetWidth !== undefined) {
      const computedWidth = element.offsetWidth;
      if (computedWidth && computedWidth > 20) {
        actualWidth = computedWidth;
      }
    }

    let summaryValue = 'none';
    if (def.bottomCalc) {
      if (typeof def.bottomCalc === 'string') {
        summaryValue = def.bottomCalc;
      } else if (typeof def.bottomCalc === 'function') {
        summaryValue = def.bottomCalc.name || 'none';
      }
    }

    return {
      field: col.getField(),
      title: def.title || col.getField(),
      visible: col.isVisible(),
      coltype: def.lithiumColtype || def.coltype || 'string',
      bottomCalc: summaryValue,
      hozAlign: def.hozAlign || 'left',
      width: actualWidth,
      category: def.category || 'Default',
      editable: def.lithiumEditable ?? (def.editable === true),
    };
  });
}

/**
 * Get available format options from coltypes
 * @returns {Array} Format options
 */
export function getFormatOptions() {
  return [
    { value: 'string', label: 'Text' },
    { value: 'integer', label: 'Integer' },
    { value: 'decimal', label: 'Decimal' },
    { value: 'index', label: 'Index' },
    { value: 'date', label: 'Date' },
    { value: 'datetime', label: 'DateTime' },
    { value: 'boolean', label: 'Boolean' },
    { value: 'lookup', label: 'Lookup' },
  ];
}

/**
 * Get available summary options
 * @returns {Array} Summary options
 */
export function getSummaryOptions() {
  return [
    { value: 'none', label: 'None' },
    { value: 'sum', label: 'Sum' },
    { value: 'count', label: 'Count' },
    { value: 'avg', label: 'Average' },
    { value: 'min', label: 'Minimum' },
    { value: 'max', label: 'Maximum' },
  ];
}

/**
 * Get available alignment options
 * @returns {Array} Alignment options
 */
export function getAlignmentOptions() {
  return [
    { value: 'left', label: 'Left' },
    { value: 'center', label: 'Center' },
    { value: 'right', label: 'Right' },
  ];
}
