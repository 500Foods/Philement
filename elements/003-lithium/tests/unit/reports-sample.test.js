import { describe, expect, it } from 'vitest';
import {
  datasourceNames, fieldItemLabel, isFieldType, parseSample, unboundFields,
} from '../../src/managers/reports/reports-sample.js';

describe('sample datasource', () => {
  it('columns are the union of keys in first-seen order', () => {
    expect(parseSample('[{"customer":"A","total":1},{"total":2,"date":"x"}]').columns)
      .toEqual(['customer', 'total', 'date']);
  });

  it('rejects bad JSON, non-arrays and non-object rows', () => {
    expect(() => parseSample('{')).toThrow('not valid JSON');
    expect(() => parseSample('{}')).toThrow('array of objects');
    expect(() => parseSample([{}, 3])).toThrow('row 2');
  });

  it('reads datasource names from the definition', () => {
    expect(datasourceNames({ datasources: ['main', { name: 'lines' }, '', null] })).toEqual(['main', 'lines']);
    expect(datasourceNames({})).toEqual([]);
  });
});

describe('field items', () => {
  it('knows the three field types', () => {
    expect([20, 21, 24].every(isFieldType)).toBe(true);
    expect(isFieldType(0)).toBe(false);
  });

  it('labels with the field name, or the type caption when unbound', () => {
    expect(fieldItemLabel({ typeKey: 20, field: 'customer' })).toBe('customer');
    expect(fieldItemLabel({ typeKey: 21 })).toBe('Field number');
    expect(fieldItemLabel({ typeKey: 24, field: '' })).toBe('Field datetime');
  });

  it('lists unbound field items', () => {
    const doc = { bands: [{ id: 'detail', items: [
      { id: 'i1', typeKey: 20, field: 'customer' },
      { id: 'i2', typeKey: 21, field: ' ' },
      { id: 'i3', typeKey: 0, text: 'x' },
    ] }] };
    expect(unboundFields(doc)).toEqual([{ bandId: 'detail', itemId: 'i2', typeKey: 21 }]);
  });
});
