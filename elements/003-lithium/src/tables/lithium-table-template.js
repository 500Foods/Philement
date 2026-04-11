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
  const def = column.getDefinition();
  const field = column.getField();
  const display = def.title || field;

  const colDef = {
    display,
    field,
    coltype: def.lithiumColtype || def.coltype || 'string',
    visible: column.isVisible(),
    sort: def.lithiumSort ?? (def.headerSort !== false),
    filter: def.lithiumFilter ?? !!def.headerFilter,
    editable: def.lithiumEditable ?? (def.editable === true),
    calculated: def.lithiumCalculated ?? false,
    primaryKey: def.lithiumPrimaryKey ?? (def.primaryKey === true || field === primaryKeyField),
    description: def.lithiumDescription || def.description || `${display} column`,
  };

  if (def.lookupRef) colDef.lookupRef = def.lookupRef;
  if (def.group != null) colDef.group = def.group;
  if (def.frozen === true) colDef.frozen = true;
  if (def.rowHandle === true) colDef.rowHandle = true;
  if (def.resizable === false) colDef.resizable = false;
  if (typeof def.formatter === 'function') colDef.formatter = def.formatter;

  // Flatten Tabulator runtime properties directly onto the column definition
  // These will overlay coltype defaults in resolveColumn
  const runtimeWidth = getRuntimeColumnWidth(column, def);
  if (runtimeWidth != null) colDef.width = runtimeWidth;
  if (def.minWidth != null) colDef.minWidth = def.minWidth;
  if (def.maxWidth != null) colDef.maxWidth = def.maxWidth;
  if (def.hozAlign) colDef.hozAlign = def.hozAlign;
  if (def.vertAlign) colDef.vertAlign = def.vertAlign;
  if (hasOwn(def, 'bottomCalc')) colDef.bottomCalc = def.bottomCalc ?? null;
  if (def.bottomCalcFormatter) colDef.bottomCalcFormatter = def.bottomCalcFormatter;
  if (def.bottomCalcFormatterParams) colDef.bottomCalcFormatterParams = def.bottomCalcFormatterParams;
  if (def.formatterParams) colDef.formatterParams = def.formatterParams;
  if (def.editorParams) colDef.editorParams = def.editorParams;
  if (def.sorterParams) colDef.sorterParams = def.sorterParams;

  return colDef;
}

export function mergeTemplateColumn(baseColumn, patchColumn = {}) {
  const merged = {
    ...baseColumn,
  };

  // FlattenLithium properties (top-level metadata)
  [
    'display',
    'field',
    'coltype',
    'visible',
    'sort',
    'filter',
    'group',
    'editable',
    'calculated',
    'primaryKey',
    'description',
    'lookupRef',
    'frozen',
    'rowHandle',
    'resizable',
  ].forEach((key) => {
    if (hasOwn(patchColumn, key)) {
      merged[key] = patchColumn[key];
    }
  });

  // Flatten overrides from both base and patch into the main object
  // This ensures template properties are at top level for resolveColumn
  const baseOverrides = baseColumn.overrides || {};
  const patchOverrides = patchColumn.overrides || {};
  const mergedOverrides = { ...baseOverrides, ...patchOverrides };
  
  // Copy all override properties to top level
  Object.assign(merged, mergedOverrides);

  // Keep overrides for backward compatibility, but flatten them
  if (Object.keys(mergedOverrides).length > 0) {
    merged.overrides = mergedOverrides;
  }

  if (typeof patchColumn.formatter === 'function') {
    merged.formatter = patchColumn.formatter;
  }

  return merged;
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
