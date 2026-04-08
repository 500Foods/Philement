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

function createTemplateColumnFromTableDef(fieldName, colDef = {}) {
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
  if (colDef.overrides && Object.keys(colDef.overrides).length > 0) {
    templateCol.overrides = { ...colDef.overrides };
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

  const overrides = {};
  const runtimeWidth = getRuntimeColumnWidth(column, def);

  if (runtimeWidth != null) overrides.width = runtimeWidth;
  if (def.minWidth != null) overrides.minWidth = def.minWidth;
  if (def.maxWidth != null) overrides.maxWidth = def.maxWidth;
  if (def.hozAlign) overrides.align = def.hozAlign;
  if (def.vertAlign) overrides.vertAlign = def.vertAlign;
  if (hasOwn(def, 'bottomCalc')) overrides.bottomCalc = def.bottomCalc ?? null;
  if (def.bottomCalcFormatter) overrides.bottomCalcFormatter = def.bottomCalcFormatter;
  if (def.bottomCalcFormatterParams) overrides.bottomCalcFormatterParams = def.bottomCalcFormatterParams;
  if (def.formatterParams) overrides.formatterParams = def.formatterParams;
  if (def.editorParams) overrides.editorParams = def.editorParams;
  if (def.sorterParams) overrides.sorterParams = def.sorterParams;

  if (Object.keys(overrides).length > 0) {
    colDef.overrides = overrides;
  }

  return colDef;
}

export function mergeTemplateColumn(baseColumn, patchColumn = {}) {
  const merged = {
    ...baseColumn,
    overrides: { ...(baseColumn.overrides || {}) },
  };

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

  if (typeof patchColumn.formatter === 'function') {
    merged.formatter = patchColumn.formatter;
  }

  if (patchColumn.overrides) {
    merged.overrides = mergeOverrides(merged.overrides, patchColumn.overrides);
  }

  if (Object.keys(merged.overrides).length === 0) {
    delete merged.overrides;
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
