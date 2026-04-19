import { describe, expect, it } from 'vitest';
import {
  extractTableDefColumn,
  extractTableDef,
  generateMigrationSeed,
  CANONICAL_COLUMN_PROPS,
} from '../../src/tables/template/capture.js';

function createMockColumn({
  field = 'query_name',
  title = 'Query Name',
  definition = {},
  visible = true,
  width,
  offsetWidth,
} = {}) {
  const def = {
    title,
    field,
    headerSort: true,
    headerFilter: 'input',
    ...definition,
  };

  return {
    getDefinition: () => def,
    getField: () => field,
    isVisible: () => visible,
    getWidth: width == null ? undefined : () => width,
    getElement: () => (offsetWidth == null ? null : { offsetWidth }),
  };
}

describe('template/capture', () => {
  describe('CANONICAL_COLUMN_PROPS', () => {
    it('contains expected canonical properties', () => {
      expect(CANONICAL_COLUMN_PROPS.has('field')).toBe(true);
      expect(CANONICAL_COLUMN_PROPS.has('title')).toBe(true);
      expect(CANONICAL_COLUMN_PROPS.has('coltype')).toBe(true);
      expect(CANONICAL_COLUMN_PROPS.has('visible')).toBe(true);
      expect(CANONICAL_COLUMN_PROPS.has('width')).toBe(true);
    });

    it('does not contain internal lithium-prefixed properties', () => {
      // lithium* properties should not be in the canonical list
      // because they're stored separately in _columnMeta
      for (const prop of CANONICAL_COLUMN_PROPS) {
        expect(prop.startsWith('lithium')).toBe(false);
      }
    });
  });

  describe('extractTableDefColumn', () => {
    it('captures the live Tabulator column width when includeRuntimeOnly is true', () => {
      const column = createMockColumn({
        definition: { width: 120 },
        width: 214,
        offsetWidth: 214,
      });

      const result = extractTableDefColumn(column, {}, {}, { includeRuntimeOnly: true });

      expect(result.width).toBe(214);
    });

    it('uses definition width when includeRuntimeOnly is false', () => {
      const column = createMockColumn({
        definition: { width: 120 },
        width: 214,
      });

      const result = extractTableDefColumn(column, { width: 120 }, {}, { includeRuntimeOnly: false });

      expect(result.width).toBe(120);
    });

    it('includes Lithium metadata from _columnMeta', () => {
      const column = createMockColumn({
        field: 'test_field',
        title: 'Test Field',
      });

      const lithiumMeta = {
        coltype: 'integer',
        primaryKey: true,
        calculated: false,
        description: 'Custom description',
      };

      const result = extractTableDefColumn(column, {}, lithiumMeta);

      expect(result.coltype).toBe('integer');
      expect(result.primaryKey).toBe(true);
      expect(result.description).toBe('Custom description');
    });

    it('preserves visibility state from the column', () => {
      const visibleColumn = createMockColumn({ visible: true });
      const hiddenColumn = createMockColumn({ visible: false });

      const visibleResult = extractTableDefColumn(visibleColumn, {}, {});
      const hiddenResult = extractTableDefColumn(hiddenColumn, {}, {});

      expect(visibleResult.visible).toBe(true);
      expect(hiddenResult.visible).toBe(false);
    });

    it('does not include lithium-prefixed properties in output', () => {
      const column = createMockColumn({
        definition: {
          lithiumColtype: 'string',
          lithiumEditable: true,
        },
      });

      const result = extractTableDefColumn(column, {}, { coltype: 'string' });

      // lithium* properties should not be in the output
      expect(result.lithiumColtype).toBeUndefined();
      expect(result.lithiumEditable).toBeUndefined();
      // But coltype should be present from lithiumMeta
      expect(result.coltype).toBe('string');
    });
  });

  describe('extractTableDef', () => {
    it('returns null when table has no Tabulator instance', () => {
      const mockTable = { table: null };
      const result = extractTableDef(mockTable);
      expect(result).toBeNull();
    });

    it('extracts complete tableDef from LithiumTable', () => {
      const mockColumns = [
        createMockColumn({ field: 'id', title: 'ID', definition: { columnPri: 1 } }),
        createMockColumn({ field: 'name', title: 'Name', definition: { columnPri: 2 } }),
      ];

      const mockTable = {
        table: {
          getColumns: () => mockColumns,
          getSorters: () => [{ field: 'id', dir: 'asc' }],
          headerFilters: {},
        },
        tableDef: {
          title: 'Test Table',
          columns: {
            id: { field: 'id', coltype: 'integer', primaryKey: true },
            name: { field: 'name', coltype: 'string' },
          },
        },
        _columnMeta: new Map([
          ['id', { coltype: 'integer', primaryKey: true }],
          ['name', { coltype: 'string' }],
        ]),
        tablePath: 'test/table',
        queryRefs: {},
        readonly: false,
        filtersVisible: false,
        tableLayoutMode: 'fitColumns',
        storageKey: 'test_table',
        getManagerId: () => 'test_manager',
        tableWidthMode: 'compact',
      };

      const result = extractTableDef(mockTable);

      expect(result).not.toBeNull();
      expect(result.title).toBe('Test Table');
      expect(result.table).toBe('table');
      expect(result.columns.id).toBeDefined();
      expect(result.columns.name).toBeDefined();
      expect(result.initialSort).toEqual([{ column: 'id', dir: 'asc' }]);
      expect(result._templateMeta).toBeDefined();
    });

    it('excludes hidden columns when includeHiddenColumns is false', () => {
      const mockColumns = [
        createMockColumn({ field: 'id', title: 'ID', visible: true }),
        createMockColumn({ field: 'hidden', title: 'Hidden', visible: false }),
      ];

      const mockTable = {
        table: {
          getColumns: () => mockColumns,
          getSorters: () => [],
          headerFilters: {},
        },
        tableDef: { title: 'Test', columns: {} },
        _columnMeta: new Map(),
        tablePath: 'test',
        queryRefs: {},
        readonly: false,
        filtersVisible: false,
        tableLayoutMode: 'fitColumns',
        storageKey: 'test',
        getManagerId: () => 'test',
        tableWidthMode: 'compact',
      };

      const result = extractTableDef(mockTable, { includeHiddenColumns: false });

      expect(result.columns.id).toBeDefined();
      expect(result.columns.hidden).toBeUndefined();
    });
  });

  describe('generateMigrationSeed', () => {
    it('generates clean JSON without _-prefixed properties', () => {
      const mockColumns = [
        createMockColumn({ field: 'id', title: 'ID' }),
      ];

      const mockTable = {
        table: {
          getColumns: () => mockColumns,
          getSorters: () => [],
          headerFilters: {},
        },
        tableDef: { title: 'Test', columns: { id: { field: 'id' } } },
        _columnMeta: new Map([['id', { coltype: 'integer' }]]),
        tablePath: 'test/table',
        queryRefs: {},
        readonly: false,
        filtersVisible: false,
        tableLayoutMode: 'fitColumns',
        storageKey: 'test',
      };

      const result = generateMigrationSeed(mockTable);
      const parsed = JSON.parse(result);

      // Should not have _-prefixed properties
      expect(parsed._templateMeta).toBeUndefined();
      expect(parsed._filtersVisible).toBeUndefined();

      // Should have core properties
      expect(parsed.title).toBe('Test');
      expect(parsed.columns.id).toBeDefined();
    });

    it('returns empty string for null table', () => {
      const mockTable = { table: null };
      const result = generateMigrationSeed(mockTable);
      expect(result).toBe('');
    });
  });
});
