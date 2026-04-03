/**
 * LithiumTable Unit Tests
 *
 * Tests the JSON-driven Tabulator column resolution engine:
 *   - Config loading (loadColtypes, loadTableDef)
 *   - Column resolution (resolveColumn, resolveColumns)
 *   - Blank/zero formatter wrapper (wrapFormatter, needsBlankZeroWrapper)
 *   - Table-level option mapping (resolveTableOptions)
 *   - Helper functions (getPrimaryKeyField, getQueryRefs)
 *   - Lookup support (loadLookup, getLookup, resolveLookupLabel, etc.)
 *   - Cache management (clearCache)
 */

import { describe, it, expect, beforeEach, vi } from 'vitest';
import {
  loadColtypes,
  loadTableDef,
  loadLookup,
  getLookup,
  resolveLookupLabel,
  createLookupFormatter,
  createLookupEditor,
  preloadLookups,
  resolveColumn,
  resolveColumns,
  resolveTableOptions,
  getPrimaryKeyField,
  getQueryRefs,
  wrapFormatter,
  needsBlankZeroWrapper,
  formatBuiltinValue,
  clearCache,
} from '../../src/core/lithium-table.js';

// Mock the lookups module
vi.mock('../../src/shared/lookups.js', () => ({
  getTabulatorSchemas: vi.fn(),
  fetchLookups: vi.fn(),
}));

import { getTabulatorSchemas, fetchLookups } from '../../src/shared/lookups.js';

// ── Test fixtures ─────────────────────────────────────────────────────────

const MOCK_COLTYPES = {
  coltypes: {
    integer: {
      align: 'right',
      vertAlign: 'middle',
      formatter: 'number',
      formatterParams: { thousand: ',', precision: 0 },
      editor: 'number',
      editorParams: { step: 1 },
      sorter: 'number',
      headerFilterFunc: '>=',
      headerFilterPlaceholder: 'filter...',
      blank: '',
      zero: '',
      cssClass: 'li-col-integer',
      bottomCalc: 'sum',
      bottomCalcFormatter: 'number',
      bottomCalcFormatterParams: { thousand: ',', precision: 0 },
      width: null,
      minWidth: 60,
    },
    string: {
      align: 'left',
      vertAlign: 'middle',
      formatter: 'plaintext',
      formatterParams: {},
      editor: 'input',
      editorParams: { search: false },
      sorter: 'alphanum',
      headerFilterFunc: 'like',
      headerFilterPlaceholder: 'filter...',
      blank: '',
      zero: null,
      cssClass: 'li-col-string',
      bottomCalc: null,
      width: null,
      minWidth: 80,
    },
    lookup: {
      align: 'left',
      vertAlign: 'middle',
      formatter: 'lookup',
      formatterParams: { lookupTable: null },
      editor: 'list',
      editorParams: { valuesLookup: true, autocomplete: true },
      sorter: 'alphanum',
      headerFilterFunc: 'like',
      headerFilterPlaceholder: 'filter...',
      blank: '',
      zero: null,
      cssClass: 'li-col-lookup',
      bottomCalc: null,
      width: null,
      minWidth: 80,
    },
    datetime: {
      align: 'center',
      vertAlign: 'middle',
      formatter: 'datetime',
      formatterParams: {
        inputFormat: "yyyy-MM-dd'T'HH:mm:ss",
        outputFormat: 'yyyy-MM-dd HH:mm',
        invalidPlaceholder: '',
      },
      editor: 'datetime-local',
      editorParams: {},
      sorter: 'datetime',
      sorterParams: { format: "yyyy-MM-dd'T'HH:mm:ss" },
      headerFilterFunc: 'like',
      headerFilterPlaceholder: 'filter...',
      blank: '',
      zero: null,
      cssClass: 'li-col-datetime',
      bottomCalc: null,
      width: 150,
      minWidth: 120,
    },
  },
};

const MOCK_TABLEDEF = {
  table: 'query-manager',
  title: 'Query Manager',
  queryRef: 25,
  searchQueryRef: 32,
  detailQueryRef: 27,
  insertQueryRef: 40,
  updateQueryRef: 41,
  deleteQueryRef: 42,
  readonly: false,
  selectableRows: 1,
  layout: 'fitColumns',
  responsiveLayout: 'collapse',
  resizableColumns: true,
  initialSort: [{ column: 'query_ref', dir: 'asc' }],
  columns: {
    query_id: {
      display: 'ID#',
      field: 'query_id',
      coltype: 'integer',
      visible: false,
      sort: true,
      filter: true,
      editable: false,
      calculated: false,
      primaryKey: true,
      overrides: { bottomCalc: null },
    },
    query_ref: {
      display: 'Ref',
      field: 'query_ref',
      coltype: 'integer',
      visible: true,
      sort: true,
      filter: true,
      editable: false,
      calculated: false,
      primaryKey: false,
      overrides: {
        width: 80,
        bottomCalc: 'count',
        bottomCalcFormatter: 'number',
        bottomCalcFormatterParams: { thousand: ',' },
      },
    },
    name: {
      display: 'Name',
      field: 'name',
      coltype: 'string',
      visible: true,
      sort: true,
      filter: true,
      editable: true,
      calculated: false,
      primaryKey: false,
    },
    created_at: {
      display: 'Created',
      field: 'created_at',
      coltype: 'datetime',
      visible: false,
      sort: true,
      filter: true,
      editable: false,
      calculated: true,
      primaryKey: false,
    },
  },
};

// ── Tests ─────────────────────────────────────────────────────────────────

describe('LithiumTable', () => {
  beforeEach(() => {
    clearCache();
    vi.clearAllMocks();
  });

  // ── loadColtypes ────────────────────────────────────────────────────────

  describe('loadColtypes', () => {
    it('should load coltypes from lookup cache on first call', async () => {
      getTabulatorSchemas.mockReturnValue([
        { key_idx: 0, collection: MOCK_COLTYPES },
      ]);

      const result = await loadColtypes();

      expect(getTabulatorSchemas).toHaveBeenCalledTimes(1);
      expect(result).toHaveProperty('integer');
      expect(result).toHaveProperty('string');
      expect(Object.keys(result)).toHaveLength(4);
    });

    it('should return cached data on subsequent calls', async () => {
      getTabulatorSchemas.mockReturnValue([
        { key_idx: 0, collection: MOCK_COLTYPES },
      ]);

      await loadColtypes();
      const second = await loadColtypes();

      // Second call should use module cache, not re-query lookups
      expect(getTabulatorSchemas).toHaveBeenCalledTimes(1);
      expect(second).toHaveProperty('integer');
    });

    it('should trigger lookup fetch if not cached', async () => {
      getTabulatorSchemas
        .mockReturnValueOnce(null)
        .mockReturnValueOnce([{ key_idx: 0, collection: MOCK_COLTYPES }]);
      fetchLookups.mockResolvedValue({});

      const result = await loadColtypes();

      expect(fetchLookups).toHaveBeenCalledTimes(1);
      expect(result).toHaveProperty('integer');
    });

    it('should return empty object when lookup data unavailable', async () => {
      getTabulatorSchemas.mockReturnValue(null);
      fetchLookups.mockResolvedValue({});

      const result = await loadColtypes();

      expect(result).toEqual({});
    });

    it('should return empty object when coltypes entry missing', async () => {
      getTabulatorSchemas.mockReturnValue([
        { key_idx: 1, collection: MOCK_TABLEDEF },
      ]);

      const result = await loadColtypes();

      expect(result).toEqual({});
    });

    it('should use provided data instead of lookup', async () => {
      const result = await loadColtypes(MOCK_COLTYPES);

      expect(getTabulatorSchemas).not.toHaveBeenCalled();
      expect(result).toHaveProperty('integer');
      expect(result).toHaveProperty('string');
      expect(Object.keys(result)).toHaveLength(4);
    });

    it('should cache provided data for subsequent calls', async () => {
      await loadColtypes(MOCK_COLTYPES);
      const second = await loadColtypes();

      expect(getTabulatorSchemas).not.toHaveBeenCalled();
      expect(second).toHaveProperty('integer');
    });

    it('should extract .coltypes key from provided data', async () => {
      const result = await loadColtypes(MOCK_COLTYPES);

      // MOCK_COLTYPES has a .coltypes key — loader should extract it
      expect(result.integer.align).toBe('right');
    });

    it('should handle provided data without .coltypes wrapper', async () => {
      // If someone passes the inner object directly (without .coltypes key)
      const result = await loadColtypes(MOCK_COLTYPES.coltypes);

      expect(result).toHaveProperty('integer');
      expect(result.integer.align).toBe('right');
    });

    it('should handle string JSON in lookup collection', async () => {
      getTabulatorSchemas.mockReturnValue([
        { key_idx: 0, collection: JSON.stringify(MOCK_COLTYPES) },
      ]);

      const result = await loadColtypes();

      expect(result).toHaveProperty('integer');
      expect(result.integer.align).toBe('right');
    });
  });

  // ── loadTableDef ────────────────────────────────────────────────────────

  describe('loadTableDef', () => {
    it('should load tabledef from lookup cache', async () => {
      getTabulatorSchemas.mockReturnValue([
        { key_idx: 0, collection: MOCK_COLTYPES },
        { key_idx: 1, collection: MOCK_TABLEDEF },
      ]);

      const result = await loadTableDef('queries/query-manager');

      expect(getTabulatorSchemas).toHaveBeenCalled();
      expect(result.title).toBe('Query Manager');
      expect(result.queryRef).toBe(25);
    });

    it('should return cached tabledef on second call', async () => {
      getTabulatorSchemas.mockReturnValue([
        { key_idx: 0, collection: MOCK_COLTYPES },
        { key_idx: 1, collection: MOCK_TABLEDEF },
      ]);

      await loadTableDef('queries/query-manager');
      const second = await loadTableDef('queries/query-manager');

      // Second call uses module cache
      expect(getTabulatorSchemas).toHaveBeenCalledTimes(1);
      expect(second.title).toBe('Query Manager');
    });

    it('should trigger lookup fetch if not cached', async () => {
      getTabulatorSchemas
        .mockReturnValueOnce(null)
        .mockReturnValueOnce([
          { key_idx: 0, collection: MOCK_COLTYPES },
          { key_idx: 1, collection: MOCK_TABLEDEF },
        ]);
      fetchLookups.mockResolvedValue({});

      const result = await loadTableDef('queries/query-manager');

      expect(fetchLookups).toHaveBeenCalledTimes(1);
      expect(result.title).toBe('Query Manager');
    });

    it('should return null when lookup data unavailable', async () => {
      getTabulatorSchemas.mockReturnValue(null);
      fetchLookups.mockResolvedValue({});

      const result = await loadTableDef('queries/query-manager');

      expect(result).toBeNull();
    });

    it('should return null when tabledef entry missing', async () => {
      getTabulatorSchemas.mockReturnValue([
        { key_idx: 0, collection: MOCK_COLTYPES },
      ]);

      const result = await loadTableDef('queries/query-manager');

      expect(result).toBeNull();
    });

    it('should strip .json extension from path', async () => {
      getTabulatorSchemas.mockReturnValue([
        { key_idx: 1, collection: MOCK_TABLEDEF },
      ]);

      await loadTableDef('queries/query-manager.json');
      const second = await loadTableDef('queries/query-manager');

      // Should hit cache, not re-query lookups
      expect(getTabulatorSchemas).toHaveBeenCalledTimes(1);
      expect(second.title).toBe('Query Manager');
    });

    it('should use provided data instead of lookup', async () => {
      const result = await loadTableDef('queries/query-manager', MOCK_TABLEDEF);

      expect(getTabulatorSchemas).not.toHaveBeenCalled();
      expect(result.title).toBe('Query Manager');
      expect(result.queryRef).toBe(25);
      expect(Object.keys(result.columns)).toHaveLength(4);
    });

    it('should cache provided data for subsequent calls', async () => {
      await loadTableDef('queries/query-manager', MOCK_TABLEDEF);
      const second = await loadTableDef('queries/query-manager');

      expect(getTabulatorSchemas).not.toHaveBeenCalled();
      expect(second.title).toBe('Query Manager');
    });

    it('should store provided data under the normalised path key', async () => {
      // Load with .json extension in the path
      await loadTableDef('queries/query-manager.json', MOCK_TABLEDEF);
      // Retrieve without .json extension — should hit the same cache entry
      const result = await loadTableDef('queries/query-manager');

      expect(getTabulatorSchemas).not.toHaveBeenCalled();
      expect(result.title).toBe('Query Manager');
    });

    it('should handle string JSON in lookup collection', async () => {
      getTabulatorSchemas.mockReturnValue([
        { key_idx: 1, collection: JSON.stringify(MOCK_TABLEDEF) },
      ]);

      const result = await loadTableDef('queries/query-manager');

      expect(result.title).toBe('Query Manager');
      expect(result.queryRef).toBe(25);
    });
  });

  // ── resolveColumn ──────────────────────────────────────────────────────

  describe('resolveColumn', () => {
    const coltypes = MOCK_COLTYPES.coltypes;

    it('should resolve a basic integer column', () => {
      const colDef = MOCK_TABLEDEF.columns.query_ref;
      const result = resolveColumn('query_ref', colDef, coltypes);

      expect(result.title).toBe('Ref');
      expect(result.field).toBe('query_ref');
      expect(result.visible).toBe(true);
      expect(result.hozAlign).toBe('right');
      expect(result.vertAlign).toBe('middle');
      expect(result.headerSort).toBe(true);
      expect(result.sorter).toBe('number');
    });

    it('should apply column overrides over coltype defaults', () => {
      const colDef = MOCK_TABLEDEF.columns.query_ref;
      const result = resolveColumn('query_ref', colDef, coltypes);

      // Override: width: 80 (coltype default is null)
      expect(result.width).toBe(80);
      // Override: bottomCalc: "count" (coltype default is "sum")
      // Now returns a function (lithiumCount) instead of string 'count'
      expect(typeof result.bottomCalc).toBe('function');
      // Override: bottomCalcFormatter: "number" — resolveColumn wraps number/money
      // formatters into a closure that calls formatBuiltinValue(), so it's a function
      expect(typeof result.bottomCalcFormatter).toBe('function');
    });

    it('should respect visible: false', () => {
      const colDef = MOCK_TABLEDEF.columns.query_id;
      const result = resolveColumn('query_id', colDef, coltypes);

      expect(result.visible).toBe(false);
    });

    it('should set headerFilter when filter: true', () => {
      const colDef = MOCK_TABLEDEF.columns.query_ref;
      const result = resolveColumn('query_ref', colDef, coltypes);

      // Without filterEditor option, defaults to 'input'
      expect(result.headerFilter).toBe('input');
    });

    it('should use custom filterEditor when provided', () => {
      const mockEditorFn = vi.fn();
      const colDef = MOCK_TABLEDEF.columns.query_ref;
      const result = resolveColumn('query_ref', colDef, coltypes, {
        filterEditor: mockEditorFn,
      });

      expect(result.headerFilter).toBe(mockEditorFn);
    });

    it('should not set headerFilter when filter is not true', () => {
      const colDef = {
        display: 'SQL',
        field: 'code',
        coltype: 'string',
        visible: false,
        sort: false,
        filter: false,
        editable: true,
        calculated: false,
      };
      const result = resolveColumn('code', colDef, coltypes);

      expect(result.headerFilter).toBeUndefined();
    });

    it('should set editor for editable, non-readonly, non-calculated columns', () => {
      const colDef = MOCK_TABLEDEF.columns.name;
      const result = resolveColumn('name', colDef, coltypes);

      expect(result.editor).toBe('input');
    });

    it('should not set editor for non-editable columns', () => {
      const colDef = MOCK_TABLEDEF.columns.query_ref;
      const result = resolveColumn('query_ref', colDef, coltypes);

      expect(result.editor).toBeUndefined();
    });

    it('should not set editor for calculated columns', () => {
      const colDef = MOCK_TABLEDEF.columns.created_at;
      const result = resolveColumn('created_at', colDef, coltypes);

      expect(result.editor).toBeUndefined();
    });

    it('should not set editor when tableReadonly is true', () => {
      const colDef = MOCK_TABLEDEF.columns.name;
      const result = resolveColumn('name', colDef, coltypes, { tableReadonly: true });

      expect(result.editor).toBeUndefined();
    });

    it('should apply cssClass from coltype', () => {
      const colDef = MOCK_TABLEDEF.columns.query_ref;
      const result = resolveColumn('query_ref', colDef, coltypes);

      expect(result.cssClass).toBe('li-col-integer');
    });

    it('should apply minWidth from coltype', () => {
      const colDef = MOCK_TABLEDEF.columns.name;
      const result = resolveColumn('name', colDef, coltypes);

      expect(result.minWidth).toBe(80);
    });

    it('should apply sorterParams when present', () => {
      const colDef = MOCK_TABLEDEF.columns.created_at;
      const result = resolveColumn('created_at', colDef, coltypes);

      expect(result.sorterParams).toEqual({ format: "yyyy-MM-dd'T'HH:mm:ss" });
    });

    it('should handle missing coltype gracefully', () => {
      const colDef = {
        display: 'Unknown',
        field: 'unknown_field',
        coltype: 'nonexistent',
        visible: true,
        sort: true,
      };
      const result = resolveColumn('unknown_field', colDef, coltypes);

      expect(result.title).toBe('Unknown');
      expect(result.field).toBe('unknown_field');
      expect(result.hozAlign).toBe('left');  // fallback
    });

    it('should handle null overrides in column definition', () => {
      const colDef = MOCK_TABLEDEF.columns.query_id;
      const result = resolveColumn('query_id', colDef, coltypes);

      // Override sets bottomCalc: null, which nullifies the coltype default "sum".
      // resolveColumn only sets bottomCalc when merged value != null, so null
      // override effectively removes it from the output (undefined).
      expect(result.bottomCalc).toBeUndefined();
    });

    it('should capture bottomCalcFormatterParams in closure for number/money formatters', () => {
      const colDef = MOCK_TABLEDEF.columns.query_ref;
      const result = resolveColumn('query_ref', colDef, coltypes);

      // For number/money bottomCalcFormatters, resolveColumn wraps them into a
      // closure that calls formatBuiltinValue() with the params captured inside.
      // So bottomCalcFormatterParams is NOT set as a separate property — the
      // params live inside the closure.  Verify the function produces correct output.
      expect(result.bottomCalcFormatterParams).toBeUndefined();
      expect(typeof result.bottomCalcFormatter).toBe('function');
      // The closure should format using the captured { thousand: ',' } params
      const mockCell = { getValue: () => 1234 };
      expect(result.bottomCalcFormatter(mockCell)).toBe('1,234');
    });

    it('should set headerSort to false when sort is false', () => {
      const colDef = {
        display: 'Code',
        field: 'code',
        coltype: 'string',
        visible: false,
        sort: false,
        filter: false,
        editable: false,
      };
      const result = resolveColumn('code', colDef, coltypes);

      expect(result.headerSort).toBe(false);
    });

    it('should use lookup formatter when lookupRef is cached', async () => {
      // Pre-populate the lookup cache
      const mockAuthQuery = vi.fn().mockResolvedValue([
        { lookup_id: 1, lookup_label: 'Active' },
        { lookup_id: 2, lookup_label: 'Inactive' },
      ]);
      const mockApi = {};
      await loadLookup('a27', mockAuthQuery, mockApi);

      const colDef = {
        display: 'Status',
        field: 'query_status_a27',
        coltype: 'lookup',
        visible: false,
        sort: true,
        filter: true,
        editable: true,
        calculated: false,
        lookupRef: 'a27',
      };
      const result = resolveColumn('query_status_a27', colDef, coltypes);

      // Formatter should be a function (the lookup formatter), not the coltype string
      expect(typeof result.formatter).toBe('function');
      // The formatter should resolve ID → label
      const mockCell = { getValue: () => 1 };
      expect(result.formatter(mockCell)).toBe('Active');
    });

    it('should use lookup editor for editable lookup columns with cached data', async () => {
      const mockAuthQuery = vi.fn().mockResolvedValue([
        { lookup_id: 1, lookup_label: 'Active' },
        { lookup_id: 2, lookup_label: 'Inactive' },
      ]);
      const mockApi = {};
      await loadLookup('a27', mockAuthQuery, mockApi);

      const colDef = {
        display: 'Status',
        field: 'query_status_a27',
        coltype: 'lookup',
        visible: false,
        sort: true,
        filter: true,
        editable: true,
        calculated: false,
        lookupRef: 'a27',
      };
      const result = resolveColumn('query_status_a27', colDef, coltypes);

      // Editor should be 'list' from lookup
      expect(result.editor).toBe('list');
      expect(result.editorParams.values).toEqual(['Active', 'Inactive']);
      expect(result.editorParams.autocomplete).toBe(true);
    });

    it('should fall back to coltype formatter when lookupRef is not cached', () => {
      // Don't pre-load lookup — cache is empty after beforeEach clearCache()
      const colDef = {
        display: 'Status',
        field: 'query_status_a27',
        coltype: 'lookup',
        visible: false,
        sort: true,
        filter: true,
        editable: true,
        calculated: false,
        lookupRef: 'a27',
      };
      const result = resolveColumn('query_status_a27', colDef, coltypes);

      // Without cached lookup, falls through to the standard formatter path
      // The lookup coltype has blank: '' so it gets wrapped
      expect(typeof result.formatter).toBe('function');
    });

    it('should fall back to coltype editor when lookupRef is not cached', () => {
      const colDef = {
        display: 'Status',
        field: 'query_status_a27',
        coltype: 'lookup',
        visible: false,
        sort: true,
        filter: true,
        editable: true,
        calculated: false,
        lookupRef: 'a27',
      };
      const result = resolveColumn('query_status_a27', colDef, coltypes);

      // Without cached lookup, falls back to coltype editor ('list')
      expect(result.editor).toBe('list');
    });
  });

  // ── resolveColumns ─────────────────────────────────────────────────────

  describe('resolveColumns', () => {
    const coltypes = MOCK_COLTYPES.coltypes;

    it('should resolve all columns from a table definition', () => {
      const result = resolveColumns(MOCK_TABLEDEF, coltypes);

      expect(result).toHaveLength(4);
      expect(result[0].field).toBe('query_id');
      expect(result[1].field).toBe('query_ref');
      expect(result[2].field).toBe('name');
      expect(result[3].field).toBe('created_at');
    });

    it('should pass filterEditor option to all columns', () => {
      const mockEditor = vi.fn();
      const result = resolveColumns(MOCK_TABLEDEF, coltypes, {
        filterEditor: mockEditor,
      });

      // Columns with filter: true should have the custom editor
      const refCol = result.find(c => c.field === 'query_ref');
      expect(refCol.headerFilter).toBe(mockEditor);
    });

    it('should set tableReadonly from tableDef.readonly', () => {
      const readonlyDef = { ...MOCK_TABLEDEF, readonly: true };
      const result = resolveColumns(readonlyDef, coltypes);

      // "name" is normally editable, but table is readonly
      const nameCol = result.find(c => c.field === 'name');
      expect(nameCol.editor).toBeUndefined();
    });

    it('should return empty array for null tableDef', () => {
      expect(resolveColumns(null, coltypes)).toEqual([]);
    });

    it('should return empty array for tableDef without columns', () => {
      expect(resolveColumns({}, coltypes)).toEqual([]);
    });
  });

  // ── resolveTableOptions ────────────────────────────────────────────────

  describe('resolveTableOptions', () => {
    it('should map all supported table-level properties', () => {
      const result = resolveTableOptions(MOCK_TABLEDEF);

      expect(result.layout).toBe('fitColumns');
      expect(result.responsiveLayout).toBe('collapse');
      expect(result.selectableRows).toBe(1);
      expect(result.resizableColumns).toBe(true);
      expect(result.initialSort).toEqual([{ column: 'query_ref', dir: 'asc' }]);
    });

    it('should omit properties not set in tableDef', () => {
      const result = resolveTableOptions({ layout: 'fitColumns' });

      expect(result.layout).toBe('fitColumns');
      expect(result.groupBy).toBeUndefined();
      expect(result.responsiveLayout).toBeUndefined();
    });

    it('should return empty object for null tableDef', () => {
      expect(resolveTableOptions(null)).toEqual({});
    });

    it('should handle groupBy when present', () => {
      const def = { ...MOCK_TABLEDEF, groupBy: 'query_type_a28' };
      const result = resolveTableOptions(def);

      expect(result.groupBy).toBe('query_type_a28');
    });

    it('should map persistSort when present', () => {
      const def = { ...MOCK_TABLEDEF, persistSort: true };
      const result = resolveTableOptions(def);

      expect(result.persistSort).toBe(true);
    });

    it('should map persistFilter when present', () => {
      const def = { ...MOCK_TABLEDEF, persistFilter: true };
      const result = resolveTableOptions(def);

      expect(result.persistFilter).toBe(true);
    });

    it('should omit persistSort/persistFilter when not set', () => {
      const result = resolveTableOptions({ layout: 'fitColumns' });

      expect(result.persistSort).toBeUndefined();
      expect(result.persistFilter).toBeUndefined();
    });

    it('should map persistSort: false explicitly', () => {
      const def = { persistSort: false };
      const result = resolveTableOptions(def);

      expect(result.persistSort).toBe(false);
    });
  });

  // ── getPrimaryKeyField ─────────────────────────────────────────────────

  describe('getPrimaryKeyField', () => {
    it('should return the field name of the primary key column', () => {
      expect(getPrimaryKeyField(MOCK_TABLEDEF)).toBe('query_id');
    });

    it('should return null if no primary key is defined', () => {
      const noPK = {
        columns: {
          name: { display: 'Name', field: 'name', primaryKey: false },
        },
      };
      expect(getPrimaryKeyField(noPK)).toBeNull();
    });

    it('should return null for null tableDef', () => {
      expect(getPrimaryKeyField(null)).toBeNull();
    });

    it('should return null for tableDef without columns', () => {
      expect(getPrimaryKeyField({})).toBeNull();
    });
  });

  // ── getQueryRefs ───────────────────────────────────────────────────────

  describe('getQueryRefs', () => {
    it('should return all query refs from tableDef', () => {
      const refs = getQueryRefs(MOCK_TABLEDEF);

      expect(refs.queryRef).toBe(25);
      expect(refs.searchQueryRef).toBe(32);
      expect(refs.detailQueryRef).toBe(27);
      expect(refs.insertQueryRef).toBe(40);
      expect(refs.updateQueryRef).toBe(41);
      expect(refs.deleteQueryRef).toBe(42);
    });

    it('should return nulls for missing refs', () => {
      const refs = getQueryRefs({ queryRef: 10 });

      expect(refs.queryRef).toBe(10);
      expect(refs.searchQueryRef).toBeNull();
      expect(refs.detailQueryRef).toBeNull();
      expect(refs.insertQueryRef).toBeNull();
      expect(refs.updateQueryRef).toBeNull();
      expect(refs.deleteQueryRef).toBeNull();
    });

    it('should return all nulls for null tableDef', () => {
      const refs = getQueryRefs(null);

      expect(refs.queryRef).toBeNull();
      expect(refs.searchQueryRef).toBeNull();
      expect(refs.detailQueryRef).toBeNull();
      expect(refs.insertQueryRef).toBeNull();
      expect(refs.updateQueryRef).toBeNull();
      expect(refs.deleteQueryRef).toBeNull();
    });

    it('should return CRUD refs when only some are defined', () => {
      const refs = getQueryRefs({ queryRef: 10, deleteQueryRef: 50 });

      expect(refs.queryRef).toBe(10);
      expect(refs.insertQueryRef).toBeNull();
      expect(refs.updateQueryRef).toBeNull();
      expect(refs.deleteQueryRef).toBe(50);
    });
  });

  // ── wrapFormatter ──────────────────────────────────────────────────────

  describe('wrapFormatter', () => {
    it('should return original formatter when no blank/zero handling needed', () => {
      const result = wrapFormatter('number', {}, null, null);

      expect(result).toBe('number');
    });

    it('should return blankValue for null cell values', () => {
      const formatter = wrapFormatter('number', {}, '—', null);
      const mockCell = { getValue: () => null };

      expect(formatter(mockCell)).toBe('—');
    });

    it('should return blankValue for undefined cell values', () => {
      const formatter = wrapFormatter('number', {}, '—', null);
      const mockCell = { getValue: () => undefined };

      expect(formatter(mockCell)).toBe('—');
    });

    it('should return blankValue for empty string cell values', () => {
      const formatter = wrapFormatter('number', {}, '—', null);
      const mockCell = { getValue: () => '' };

      expect(formatter(mockCell)).toBe('—');
    });

    it('should return empty string blankValue for blanks', () => {
      const formatter = wrapFormatter('number', {}, '', null);
      const mockCell = { getValue: () => null };

      // blank="" means show nothing — that's active
      expect(formatter(mockCell)).toBe('');
    });

    it('should return zeroValue for zero cell values', () => {
      const formatter = wrapFormatter('number', {}, null, '—');
      const mockCell = { getValue: () => 0 };

      expect(formatter(mockCell)).toBe('—');
    });

    it('should format non-blank, non-zero values using built-in number formatter', () => {
      const formatter = wrapFormatter('number', { thousand: ',', precision: 0 }, '', '');
      const mockCell = { getValue: () => 42 };

      expect(formatter(mockCell)).toBe('42');
    });

    it('should format large numbers with thousands separator', () => {
      const formatter = wrapFormatter('number', { thousand: ',', precision: 0 }, '', '');
      const mockCell = { getValue: () => 1234567 };

      expect(formatter(mockCell)).toBe('1,234,567');
    });

    it('should format money values with symbol and precision', () => {
      const formatter = wrapFormatter('money', { symbol: '$', thousand: ',', precision: 2 }, '', null);
      const mockCell = { getValue: () => 1234.5 };

      expect(formatter(mockCell)).toBe('$1,234.50');
    });

    it('should format plaintext values as strings', () => {
      const formatter = wrapFormatter('plaintext', {}, '', null);
      const mockCell = { getValue: () => 42 };

      expect(formatter(mockCell)).toBe('42');
    });

    it('should pass through unknown formatter types as raw values', () => {
      const formatter = wrapFormatter('datetime', {}, '', null);
      const mockCell = { getValue: () => '2026-01-15' };

      expect(formatter(mockCell)).toBe('2026-01-15');
    });

    it('should call custom formatter function for non-blank values', () => {
      const customFn = vi.fn().mockReturnValue('formatted');
      const formatter = wrapFormatter(customFn, { precision: 2 }, '', '');
      const mockCell = { getValue: () => 42 };

      const result = formatter(mockCell, vi.fn());

      expect(result).toBe('formatted');
      expect(customFn).toHaveBeenCalledWith(mockCell, { precision: 2 }, expect.any(Function));
    });

    it('should not call custom formatter for blank values', () => {
      const customFn = vi.fn();
      const formatter = wrapFormatter(customFn, {}, '—', null);
      const mockCell = { getValue: () => null };

      formatter(mockCell, vi.fn());

      expect(customFn).not.toHaveBeenCalled();
    });
  });

  // ── needsBlankZeroWrapper ──────────────────────────────────────────────

  describe('needsBlankZeroWrapper', () => {
    it('should return false when both blank and zero are null', () => {
      expect(needsBlankZeroWrapper(null, null)).toBe(false);
    });

    it('should return true when blank is set (even empty string)', () => {
      expect(needsBlankZeroWrapper('', null)).toBe(true);
    });

    it('should return true when zero is set', () => {
      expect(needsBlankZeroWrapper(null, '')).toBe(true);
    });

    it('should return true when both are set', () => {
      expect(needsBlankZeroWrapper('—', '—')).toBe(true);
    });

    it('should return true for blank="0"', () => {
      expect(needsBlankZeroWrapper('0', null)).toBe(true);
    });
  });

  // ── formatBuiltinValue ─────────────────────────────────────────────────

  describe('formatBuiltinValue', () => {
    it('should format number with precision 0', () => {
      expect(formatBuiltinValue(42, 'number', { precision: 0 })).toBe('42');
    });

    it('should format number with precision 2', () => {
      expect(formatBuiltinValue(42, 'number', { precision: 2 })).toBe('42.00');
    });

    it('should format number with thousand separator', () => {
      expect(formatBuiltinValue(1234567, 'number', { thousand: ',', precision: 0 })).toBe('1,234,567');
    });

    it('should format number with suffix', () => {
      expect(formatBuiltinValue(42, 'number', { precision: 0, suffix: 's' })).toBe('42s');
    });

    it('should format number with prefix', () => {
      expect(formatBuiltinValue(42, 'number', { precision: 0, prefix: '#' })).toBe('#42');
    });

    it('should format number with custom decimal character', () => {
      expect(formatBuiltinValue(3.14, 'number', { precision: 2, decimal: ',' })).toBe('3,14');
    });

    it('should return empty string for null number', () => {
      expect(formatBuiltinValue(null, 'number', { precision: 0 })).toBe('');
    });

    it('should return empty string for empty string number', () => {
      expect(formatBuiltinValue('', 'number', { precision: 0 })).toBe('');
    });

    it('should return string for NaN input', () => {
      expect(formatBuiltinValue('abc', 'number', { precision: 0 })).toBe('abc');
    });

    it('should format money with symbol before', () => {
      expect(formatBuiltinValue(1234.5, 'money', {
        symbol: '$', thousand: ',', precision: 2,
      })).toBe('$1,234.50');
    });

    it('should format money with symbol after', () => {
      expect(formatBuiltinValue(1234, 'money', {
        symbol: '€', symbolAfter: true, precision: 2,
      })).toBe('1234.00€');
    });

    it('should format money with custom decimal', () => {
      expect(formatBuiltinValue(1234.5, 'money', {
        symbol: '€', symbolAfter: true, precision: 2, thousand: '.', decimal: ',',
      })).toBe('1.234,50€');
    });

    it('should format plaintext as string', () => {
      expect(formatBuiltinValue(42, 'plaintext', {})).toBe('42');
      expect(formatBuiltinValue('hello', 'plaintext', {})).toBe('hello');
      expect(formatBuiltinValue(null, 'plaintext', {})).toBe('');
    });

    it('should return raw value for unknown formatter', () => {
      expect(formatBuiltinValue(42, 'datetime', {})).toBe(42);
      expect(formatBuiltinValue('2026-01-01', 'datetime', {})).toBe('2026-01-01');
    });

    it('should handle zero precision for money', () => {
      expect(formatBuiltinValue(99, 'money', { symbol: '¥', precision: 0 })).toBe('¥99');
    });

    it('should handle negative numbers', () => {
      expect(formatBuiltinValue(-1234, 'number', { thousand: ',', precision: 0 })).toBe('-1,234');
    });

    it('should default to precision 0 for number when not specified', () => {
      expect(formatBuiltinValue(42.789, 'number', {})).toBe('43');
    });

    it('should default to precision 2 for money when not specified', () => {
      expect(formatBuiltinValue(42, 'money', {})).toBe('42.00');
    });
  });

  // ── Lookup support ─────────────────────────────────────────────────────

  describe('loadLookup', () => {
    it('should return empty array when no API instance provided', async () => {
      const result = await loadLookup('a27', null, null);

      expect(result).toEqual([]);
    });

    it('should return empty array when no authQuery function provided', async () => {
      const result = await loadLookup('a27', null, {});

      expect(result).toEqual([]);
    });

    it('should fetch and cache lookup data using QueryRef 34', async () => {
      const mockAuthQuery = vi.fn().mockResolvedValue([
        { lookup_id: 1, lookup_label: 'Active' },
        { lookup_id: 2, lookup_label: 'Inactive' },
      ]);
      const mockApi = {};

      const result = await loadLookup('a27', mockAuthQuery, mockApi);

      expect(result).toHaveLength(2);
      expect(result[0]).toEqual({ id: 1, label: 'Active' });
      expect(result[1]).toEqual({ id: 2, label: 'Inactive' });
      // Verify correct QueryRef and parameter format
      expect(mockAuthQuery).toHaveBeenCalledWith(mockApi, 34, {
        INTEGER: { LOOKUPID: 27 },
      });
    });

    it('should parse numeric lookup ID from various formats', async () => {
      const mockAuthQuery = vi.fn().mockResolvedValue([]);
      const mockApi = {};

      // Test a27 format
      await loadLookup('a27', mockAuthQuery, mockApi);
      expect(mockAuthQuery).toHaveBeenLastCalledWith(mockApi, 34, {
        INTEGER: { LOOKUPID: 27 },
      });

      // Test g42 format
      await loadLookup('g42', mockAuthQuery, mockApi);
      expect(mockAuthQuery).toHaveBeenLastCalledWith(mockApi, 34, {
        INTEGER: { LOOKUPID: 42 },
      });

      // Test numeric-only format (just in case)
      await loadLookup('58', mockAuthQuery, mockApi);
      expect(mockAuthQuery).toHaveBeenLastCalledWith(mockApi, 34, {
        INTEGER: { LOOKUPID: 58 },
      });
    });

    it('should return empty array for invalid lookup reference format', async () => {
      const mockAuthQuery = vi.fn();
      const mockApi = {};

      const result = await loadLookup('invalid', mockAuthQuery, mockApi);

      expect(result).toEqual([]);
      expect(mockAuthQuery).not.toHaveBeenCalled();
    });

    it('should return cached data on second call', async () => {
      const mockAuthQuery = vi.fn().mockResolvedValue([
        { lookup_id: 1, lookup_label: 'Active' },
      ]);
      const mockApi = {};

      await loadLookup('a27', mockAuthQuery, mockApi);
      const second = await loadLookup('a27', mockAuthQuery, mockApi);

      expect(mockAuthQuery).toHaveBeenCalledTimes(1);
      expect(second).toHaveLength(1);
    });

    it('should return empty array on API error', async () => {
      const mockAuthQuery = vi.fn().mockRejectedValue(new Error('API error'));
      const mockApi = {};

      const result = await loadLookup('a99', mockAuthQuery, mockApi);

      expect(result).toEqual([]);
    });

    it('should handle alternative field names in API response', async () => {
      const mockAuthQuery = vi.fn().mockResolvedValue([
        { id: 10, label: 'Test Label' },
      ]);
      const mockApi = {};

      const result = await loadLookup('b55', mockAuthQuery, mockApi);

      expect(result[0]).toEqual({ id: 10, label: 'Test Label' });
    });
  });

  describe('getLookup', () => {
    it('should return undefined for non-cached lookup', () => {
      expect(getLookup('a99')).toBeUndefined();
    });

    it('should return cached lookup data', async () => {
      const mockAuthQuery = vi.fn().mockResolvedValue([
        { lookup_id: 1, lookup_label: 'Active' },
      ]);
      const mockApi = {};

      await loadLookup('a27', mockAuthQuery, mockApi);

      const data = getLookup('a27');
      expect(data).toHaveLength(1);
      expect(data[0].label).toBe('Active');
    });
  });

  describe('resolveLookupLabel', () => {
    beforeEach(async () => {
      const mockAuthQuery = vi.fn().mockResolvedValue([
        { lookup_id: 1, lookup_label: 'Active' },
        { lookup_id: 2, lookup_label: 'Inactive' },
      ]);
      const mockApi = {};
      await loadLookup('a27', mockAuthQuery, mockApi);
    });

    it('should return the label for a known ID', () => {
      expect(resolveLookupLabel(1, 'a27')).toBe('Active');
      expect(resolveLookupLabel(2, 'a27')).toBe('Inactive');
    });

    it('should return the ID as string for unknown ID', () => {
      expect(resolveLookupLabel(99, 'a27')).toBe('99');
    });

    it('should return empty string for null ID', () => {
      expect(resolveLookupLabel(null, 'a27')).toBe('');
    });

    it('should return ID as string for uncached lookup ref', () => {
      expect(resolveLookupLabel(5, 'b99')).toBe('5');
    });
  });

  describe('createLookupFormatter', () => {
    beforeEach(async () => {
      const mockAuthQuery = vi.fn().mockResolvedValue([
        { lookup_id: 1, lookup_label: 'Active' },
      ]);
      const mockApi = {};
      await loadLookup('a27', mockAuthQuery, mockApi);
    });

    it('should return a formatter function', () => {
      const fn = createLookupFormatter('a27');
      expect(typeof fn).toBe('function');
    });

    it('should format cell value to lookup label', () => {
      const fn = createLookupFormatter('a27');
      const mockCell = { getValue: () => 1 };

      expect(fn(mockCell)).toBe('Active');
    });

    it('should return empty string for null/empty values', () => {
      const fn = createLookupFormatter('a27');

      expect(fn({ getValue: () => null })).toBe('');
      expect(fn({ getValue: () => '' })).toBe('');
    });
  });

  describe('createLookupEditor', () => {
    it('should return input fallback for empty lookup data', () => {
      const result = createLookupEditor('a99', []);

      expect(result).toBe('input');
    });

    it('should return editor config with list values', () => {
      const lookupData = [
        { id: 1, label: 'Active' },
        { id: 2, label: 'Inactive' },
      ];
      const result = createLookupEditor('a27', lookupData);

      expect(result.editor).toBe('list');
      expect(result.editorParams.values).toEqual(['Active', 'Inactive']);
      expect(result.editorParams.autocomplete).toBe(true);
    });

    it('should return input fallback for null lookup data', () => {
      const result = createLookupEditor('a99', null);

      expect(result).toBe('input');
    });
  });

  describe('preloadLookups', () => {
    it('should load multiple lookups in parallel', async () => {
      const mockAuthQuery = vi.fn()
        .mockResolvedValueOnce([{ lookup_id: 1, lookup_label: 'Active' }])
        .mockResolvedValueOnce([{ lookup_id: 10, lookup_label: 'SQL' }]);
      const mockApi = {};

      const results = await preloadLookups(['a27', 'a28'], mockAuthQuery, mockApi);

      expect(results.size).toBe(2);
      expect(results.get('a27')).toHaveLength(1);
      expect(results.get('a28')).toHaveLength(1);
    });
  });

  // ── clearCache ─────────────────────────────────────────────────────────

  describe('clearCache', () => {
    it('should clear all caches so data is re-loaded', async () => {
      getTabulatorSchemas.mockReturnValue([
        { key_idx: 0, collection: MOCK_COLTYPES },
        { key_idx: 1, collection: MOCK_TABLEDEF },
      ]);

      await loadColtypes();
      await loadTableDef('queries/query-manager');

      // Called twice: once for coltypes, once for tabledef
      expect(getTabulatorSchemas).toHaveBeenCalledTimes(2);

      clearCache();
      getTabulatorSchemas.mockClear();

      await loadColtypes();
      await loadTableDef('queries/query-manager');

      // Should be called again after cache clear
      expect(getTabulatorSchemas).toHaveBeenCalledTimes(2);
    });
  });
});
