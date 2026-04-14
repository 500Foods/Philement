import { resolveColumns } from './lithium-table.js';

function hasOwn(obj, key) {
  return Object.prototype.hasOwnProperty.call(obj, key);
}

function mergeOverrides(baseOverrides = {}, patchOverrides = {}) {
  const merged = { ...baseOverrides };
  Object.entries(patchOverrides).forEach(([key, value]) => {
    merged[key] = value;
  });
  return merged;
}

function removeHiddenHeaderFilters(columns) {
  columns.forEach((column) => {
    delete column.headerFilter;
    delete column.headerFilterFunc;
    delete column.headerFilterParams;
  });
}

export function createTemplateColumnFromTableDef(fieldName, colDef = {}) {
  const display = colDef.display || colDef.title || colDef.field || fieldName;
  const templateCol = {
    display,
    field: colDef.field || fieldName,
    coltype: colDef.coltype || 'string',
    visible: colDef.visible !== false,
    sort: colDef.sort !== false,
    filter: colDef.filter !== false,
    editable: colDef.editable === true,
    calculated: colDef.calculated === true,
    primaryKey: colDef.primaryKey === true,
    description: colDef.description || `${display} column`,
  };

  if (colDef.lookupRef) templateCol.lookupRef = colDef.lookupRef;
  if (colDef.group != null) templateCol.group = colDef.group;
  if (colDef.frozen === true) templateCol.frozen = true;
  if (colDef.rowHandle === true) templateCol.rowHandle = true;
  if (colDef.resizable === false) templateCol.resizable = false;
  if (typeof colDef.formatter === 'function') templateCol.formatter = colDef.formatter;

  // Flatten Tabulator properties directly onto the column definition
  // (replacing the old overrides pattern)
  const tabulatorProps = [
    'width', 'minWidth', 'maxWidth', 'align', 'vertAlign',
    'bottomCalc', 'bottomCalcFormatter', 'bottomCalcFormatterParams',
    'formatterParams', 'editorParams', 'sorterParams',
    'headerSort', 'headerFilter', 'headerFilterFunc', 'headerFilterParams',
    'hozAlign'
  ];
  
  for (const prop of tabulatorProps) {
    if (colDef[prop] != null) {
      templateCol[prop] = colDef[prop];
    }
  }

  return templateCol;
}

function getRuntimeColumnWidth(column, def = {}) {
  const componentWidth = column.getWidth?.();
  if (Number.isFinite(componentWidth) && componentWidth > 0) {
    return Math.round(componentWidth);
  }

  const element = column.getElement?.();
  const elementWidth = element?.offsetWidth;
  if (Number.isFinite(elementWidth) && elementWidth > 0) {
    return Math.round(elementWidth);
  }

  if (Number.isFinite(def.width) && def.width > 0) {
    return Math.round(def.width);
  }

  return null;
}

export function extractTemplateColumnFromColumn(column, primaryKeyField = null) {
  // Copy ALL properties from the column definition, but filter out internal lithium properties
  const def = column.getDefinition();
  const field = column.getField();

  // Handle both single field and array of fields for compound keys
  const isPrimaryKey = def.primaryKey === true ||
    (Array.isArray(primaryKeyField) ? primaryKeyField.includes(field) : field === primaryKeyField);

  // Start with definition properties, excluding lithium-prefixed internal properties
  const colDef = {};
  for (const [key, value] of Object.entries(def)) {
    // Skip lithium-prefixed properties and other internal properties
    if (!key.startsWith('lithium') && !key.startsWith('_') && key !== 'headerFilterParams') {
      colDef[key] = value;
    }
  }

  // Override with specific values to ensure correctness
  colDef.field = field;
  colDef.coltype = def.lithiumColtype || def.coltype || 'string';
  colDef.visible = column.isVisible();
  colDef.primaryKey = isPrimaryKey;

  // Preserve title from the definition
  if (!colDef.title) {
    colDef.title = def.title || def.display || field;
  }

  // Preserve Tabulator hozAlign
  // (tableDef schema now supports hozAlign directly)

  // Preserve other Tabulator-specific properties that should be in tableDef
  // Convert formatter function back to string if possible
  if (typeof def.formatter === 'function' && def.lithiumFormatter) {
    colDef.formatter = def.lithiumFormatter;
  }

  // Capture runtime width (may differ from definition width)
  const runtimeWidth = getRuntimeColumnWidth(column, def);
  if (runtimeWidth != null) {
    colDef.width = runtimeWidth;
  }

  // Ensure we capture columnPri from the original tableDef if available
  if (def.columnPri != null) {
    colDef.columnPri = def.columnPri;
  }

  return colDef;
}

export function mergeTemplateColumn(baseColumn, patchColumn = {}) {
  // The patch column IS the captured state we want to restore
  // So we just use it entirely - no need to cherry-pick properties
  return { ...patchColumn };
}

export function buildColumnDefinitionsFromTemplateState({
  table,
  templateColumns,
  tableDefColumns,
  coltypes,
  filterEditor,
  filtersVisible,
  primaryKeyField,
  readonly,
  selectorColumn,
}) {
  const currentColumns = table.getColumns();
  const currentColumnMap = new Map();

  currentColumns.forEach((column) => {
    const field = column.getField();
    if (!field || field === '_selector') return;
    currentColumnMap.set(field, extractTemplateColumnFromColumn(column, primaryKeyField));
  });

  Object.entries(tableDefColumns || {}).forEach(([fieldName, colDef]) => {
    if (!fieldName || fieldName === '_selector') return;
    if (!currentColumnMap.has(fieldName)) {
      currentColumnMap.set(fieldName, createTemplateColumnFromTableDef(fieldName, colDef));
    }
  });

  const orderedEntries = [];

  Object.entries(templateColumns || {}).forEach(([field, patchColumn]) => {
    const baseColumn = currentColumnMap.get(field);
    if (!baseColumn) return;
    currentColumnMap.delete(field);
    orderedEntries.push([field, mergeTemplateColumn(baseColumn, patchColumn)]);
  });

  currentColumnMap.forEach((baseColumn, field) => {
    orderedEntries.push([field, baseColumn]);
  });

  const resolvedColumns = resolveColumns(
    {
      readonly,
      columns: Object.fromEntries(orderedEntries),
    },
    coltypes,
    { filterEditor },
  );

  if (!filtersVisible) {
    removeHiddenHeaderFilters(resolvedColumns);
  }

  return selectorColumn ? [selectorColumn, ...resolvedColumns] : resolvedColumns;
}
