import { describe, expect, it } from 'vitest';
import { applyProperty, CLIENT_SECTIONS, propertyRows } from '../../src/managers/reports/report-properties.js';
import { groupBySection } from '../../src/properties/sections.js';
import {
  bandDatasource, datasourceColumns, datasourceKinds, setDatasourceRows,
} from '../../src/managers/reports/reports-sample.js';

const base = () => ({
  format: 1, name: 'R', datasources: [],
  bands: [{ id: 'd', typeKey: 15, items: [{ id: 'f', typeKey: 21, field: '' }] }],
});
const report = {};

describe('Phase 19b datasources', () => {
  const titles = (count) => {
    const doc = applyProperty(base(), report, 'ds-count', String(count));
    const shaped = propertyRows(doc, report, {}).map((row, sortSeq) => ({
      ...row, sortSeq, collection: { section: row.section, Group: row.group },
    }));
    return groupBySection(shaped, CLIENT_SECTIONS)
      .filter((g) => g.section && /^Datasource/.test(g.section.label))
      .map((g) => [g.section.label, g.rows.map((r) => r.label)]);
  };

  it('gives each datasource its own "Datasource N" section', () => {
    expect(titles(0)).toEqual([['Datasources', ['Count']]]);
    expect(titles(1)).toEqual([['Datasources', ['Count']], ['Datasource 1', ['Name', 'Kind', 'Fields']]]);
    expect(titles(2)).toEqual([
      ['Datasources', ['Count']],
      ['Datasource 1', ['Name', 'Kind', 'Fields']],
      ['Datasource 2', ['Name', 'Kind', 'Fields']],
    ]);
  });

  it('adds and trims through Count, renames, and removes on blank name', () => {
    let doc = applyProperty(base(), report, 'ds-count', '2');
    expect(doc.datasources.map((d) => d.name)).toEqual(['data1', 'data2']);
    expect(doc.datasources[0]).toMatchObject({ kind: 'json', kindKey: 1, rows: [] });
    doc = { ...doc, bands: [{ ...doc.bands[0], datasource: 'data1' }] };
    doc = applyProperty(doc, report, 'ds-0-name', 'orders');
    expect(doc.bands[0].datasource).toBe('orders');
    expect(() => applyProperty(doc, report, 'ds-1-name', 'orders')).toThrow(/already used/);
    expect(() => applyProperty(doc, report, 'ds-1-name', 'two words')).toThrow(/one word/);
    doc = applyProperty(doc, report, 'ds-0-name', '');
    expect(doc.datasources.map((d) => d.name)).toEqual(['data2']);
    expect(doc.bands[0].datasource).toBeUndefined();
    doc = applyProperty(doc, report, 'ds-count', '0');
    expect(doc.datasources).toEqual([]);
  });

  it('accepts JSON and Query kinds; CSV is listed but refused', () => {
    const doc = applyProperty(base(), report, 'ds-count', '1');
    const kindRow = propertyRows(doc, report, {}).find((r) => r.id === 'ds-0-kind');
    expect(kindRow.list).toEqual(['JSON', 'Query', 'CSV']);
    expect(kindRow.value).toBe('JSON');
    expect(applyProperty(doc, report, 'ds-0-kind', 'Query').datasources[0]).toMatchObject({ kind: 'query', kindKey: 2, queryRef: null, params: {} });
    expect(() => applyProperty(doc, report, 'ds-0-kind', 'CSV')).toThrow(/not available/);
    expect(applyProperty(doc, report, 'ds-0-kind', 'JSON').datasources[0].kindKey).toBe(1);
  });

  it('reads kinds from lookup 076 rows when loaded', () => {
    expect(datasourceKinds([{ key_idx: 1, value_txt: 'JSON', code: 'json' }])).toEqual([{ key: 1, label: 'JSON', code: 'json' }]);
  });

  it('stores sample rows on the target or first json datasource, creating one if needed', () => {
    const first = setDatasourceRows(base(), '', [{ a: 1 }, { b: 2 }]);
    expect(first.name).toBe('data1');
    expect(datasourceColumns(first.doc, 'data1')).toEqual(['a', 'b']);
    expect(bandDatasource(first.doc, first.doc.bands[0])).toBe('data1');
    const again = setDatasourceRows(first.doc, 'data1', [{ c: 3 }]);
    expect(again.doc.datasources).toHaveLength(1);
    expect(again.doc.datasources[0].rows).toEqual([{ c: 3 }]);
    expect(() => setDatasourceRows(first.doc, 'nope', [])).toThrow(/does not exist/);
  });
});

describe('detail fixture carries its rows', () => {
  it('composeSources matches detail.sources.json', async () => {
    const { readFileSync } = await import('node:fs');
    const { composeSources } = await import('../../src/managers/reports/reports-compose.js');
    const read = (name) => JSON.parse(readFileSync(`${process.cwd()}/tests/fixtures/reporting/${name}`, 'utf8'));
    const def = read('detail.definition.json');
    const sources = read('detail.sources.json');
    expect(composeSources(def)).toEqual(sources);
  });
});
