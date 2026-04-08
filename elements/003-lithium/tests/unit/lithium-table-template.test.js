import { describe, expect, it } from 'vitest';
import { extractTemplateColumnFromColumn } from '../../src/tables/lithium-table-template.js';

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
    lithiumColtype: 'string',
    headerSort: true,
    headerFilter: 'input',
    description: 'Mock column',
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

describe('lithium-table-template', () => {
  describe('extractTemplateColumnFromColumn', () => {
    it('captures the live Tabulator column width when available', () => {
      const column = createMockColumn({
        definition: { width: 120 },
        width: 214,
        offsetWidth: 214,
      });

      const result = extractTemplateColumnFromColumn(column);

      expect(result.overrides.width).toBe(214);
    });

    it('falls back to DOM width when the column component width is unavailable', () => {
      const column = createMockColumn({
        definition: { width: 120 },
        offsetWidth: 187,
      });

      const result = extractTemplateColumnFromColumn(column);

      expect(result.overrides.width).toBe(187);
    });

    it('falls back to the definition width when no live width can be read', () => {
      const column = createMockColumn({
        definition: { width: 143 },
      });

      const result = extractTemplateColumnFromColumn(column);

      expect(result.overrides.width).toBe(143);
    });
  });
});
