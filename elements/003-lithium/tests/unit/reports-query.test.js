import { describe, expect, it, vi } from 'vitest';
import { applyProperty, propertyRows } from '../../src/managers/reports/report-properties.js';
import { addItem } from '../../src/managers/reports/model/document.js';
import { composeSources } from '../../src/managers/reports/reports-compose.js';
import { datasourceColumns, parameterNames, paramItemLabel } from '../../src/managers/reports/reports-sample.js';
import {
  pendingQueryDatasources,
  conduitParams, findReportingQuery, reportingQuery, runQueryDatasources,
} from '../../src/managers/reports/reports-query.js';

const meta = {
  reporting: {
    group: 'Demo',
    fields: [{ name: 'row_id', type: 'integer' }, { name: 'amount', type: 'float' }],
    parameters: [
      { name: 'ROW_LIMIT', type: 'integer', default: 5 },
      { name: 'TITLE', type: 'string', default: 'Reporting Demo' },
      { name: 'SCALE', type: 'float', default: 1.0 },
    ],
  },
};
const queries = [reportingQuery({ query_ref: 174, name: 'Reporting Demo', summary: '', collection: JSON.stringify(meta) })];
const report = {};
const base = () => ({
  format: 1, name: 'R', datasources: [],
  bands: [{ id: 'd', typeKey: 15, items: [] }],
});
const ctx = { reportingQueries: queries };

function queryDoc() {
  let doc = applyProperty(base(), report, 'ds-count', '1');
  doc = applyProperty(doc, report, 'ds-0-kind', 'Query', ctx);
  return applyProperty(doc, report, 'ds-0-query', '#174 Reporting Demo', ctx);
}

describe('Phase 28 query datasources', () => {
  it('normalizes QueryRef 173 rows', () => {
    expect(queries[0]).toMatchObject({ queryRef: 174, group: 'Demo' });
    expect(queries[0].parameters.map((p) => p.name)).toEqual(['ROW_LIMIT', 'TITLE', 'SCALE']);
    expect(findReportingQuery(queries, 174)).toBe(queries[0]);
  });

  it('Kind Query is live and picking a query fills params and columns', () => {
    const doc = queryDoc();
    expect(doc.datasources[0]).toMatchObject({
      kind: 'query', kindKey: 2, queryRef: 174, params: { ROW_LIMIT: 5, TITLE: 'Reporting Demo', SCALE: 1 },
    });
    expect(datasourceColumns(doc, 'data1')).toEqual(['row_id', 'amount']);
    const labels = propertyRows(doc, report, ctx).filter((r) => r.group === 'Datasource 1').map((r) => r.label);
    expect(labels).toEqual(['Name', 'Kind', 'Query', 'ROW_LIMIT', 'TITLE', 'SCALE', 'Fields']);
    const rows = propertyRows(doc, report, ctx);
    expect(rows.find((r) => r.id === 'ds-0-param-ROW_LIMIT')).toMatchObject({ labelType: 'int', editor: 'Number', value: '5' });
    expect(rows.find((r) => r.id === 'ds-0-param-TITLE').labelType).toBe('str');
    const fields = rows.find((r) => r.id === 'ds-0-fields');
    expect(fields).toMatchObject({ multiline: true, readOnly: true });
    expect(fields.value.split('\n')).toHaveLength(2);
    expect(fields.value).not.toMatch(/rows?:/);
  });

  it('edits one parameter row into params', () => {
    const doc = applyProperty(queryDoc(), report, 'ds-0-param-ROW_LIMIT', '2', ctx);
    expect(doc.datasources[0].params).toMatchObject({ ROW_LIMIT: '2', TITLE: 'Reporting Demo', SCALE: 1 });
    expect(conduitParams(doc.datasources[0].params, queries[0]).INTEGER).toEqual({ ROW_LIMIT: 2 });
  });

  it('edits params as JSON and types them for conduit', () => {
    const doc = applyProperty(queryDoc(), report, 'ds-0-params', '{"ROW_LIMIT":"2","TITLE":"X","SCALE":"2.5"}', ctx);
    expect(conduitParams(doc.datasources[0].params, queries[0])).toEqual({
      INTEGER: { ROW_LIMIT: 2 }, STRING: { TITLE: 'X' }, FLOAT: { SCALE: 2.5 },
    });
    expect(() => applyProperty(doc, report, 'ds-0-params', '[1]', ctx)).toThrow(/JSON object/);
  });

  it('run stores rows and compose sends them; failure keeps old rows', async () => {
    const run = vi.fn(async () => [{ row_id: 1, amount: 2 }, { row_id: 2, amount: 3 }]);
    const doc = queryDoc();
    expect(composeSources({ ...doc, datasources: [{ ...doc.datasources[0], rows: undefined }] })).toEqual({});
    const { doc: ran, ran: info } = await runQueryDatasources(null, doc, queries, run);
    expect(run).toHaveBeenCalledWith(null, 174, { INTEGER: { ROW_LIMIT: 5 }, STRING: { TITLE: 'Reporting Demo' }, FLOAT: { SCALE: 1 } });
    expect(info).toEqual([{ name: 'data1', rows: 2 }]);
    expect(composeSources(ran).data1).toHaveLength(2);
    const bad = await runQueryDatasources(null, ran, queries, async () => { throw new Error('nope'); });
    expect(bad.errors[0].message).toBe('nope');
    expect(bad.doc.datasources[0].rows).toHaveLength(2);
  });
});

describe('pendingQueryDatasources', () => {
  it('lists query datasources with a QueryRef and no rows', () => {
    const doc = { datasources: [
      { name: 'a', kind: 'query', queryRef: 174 },
      { name: 'b', kind: 'query', queryRef: 174, rows: [{ x: 1 }] },
      { name: 'c', kind: 'query', queryRef: 174, rows: [] },
      { name: 'd', kind: 'json', rows: [] },
      { name: 'e', kind: 'query' },
    ] };
    expect(pendingQueryDatasources(doc).map((d) => d.name)).toEqual(['a', 'c']);
  });

  it('Parameter item (63) picks a datasource parameter and keeps a mask', () => {
    let doc = addItem(queryDoc(), 'd', { typeKey: 63, param: '', box: { left: '0 in', top: '0 in', width: '1 in', height: '0.2 in' } });
    const item = doc.bands[0].items[0];
    expect(item.mask).toBe('');
    expect(parameterNames(doc)).toEqual(['ROW_LIMIT', 'TITLE', 'SCALE']);
    const sel = { bandId: 'd', itemId: item.id };
    const row = propertyRows(doc, sel, ctx).find((r) => r.id === 'param');
    expect(row).toMatchObject({ label: 'Parameter', editor: 'List', list: ['ROW_LIMIT', 'TITLE', 'SCALE'] });
    expect(propertyRows(doc, sel, ctx).some((r) => r.id === 'mask')).toBe(true);
    doc = applyProperty(doc, sel, 'param', 'TITLE', ctx);
    expect(doc.bands[0].items[0].param).toBe('TITLE');
    expect(paramItemLabel(doc.bands[0].items[0])).toBe('[TITLE]');
    expect(paramItemLabel({})).toBe('Parameter');
  });
});
