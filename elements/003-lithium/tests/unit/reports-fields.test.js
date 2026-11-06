import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import ReportsManager from '../../src/managers/reports/reports.js';
import { addItem, createBlank } from '../../src/managers/reports/model/document.js';
import { applyProperty, propertyRows } from '../../src/managers/reports/report-properties.js';

vi.mock('../../src/tables/lithium-table-main.js', () => ({
  LithiumTable: class {
    constructor(options) {
      this.options = options;
      this.currentData = [];
      this.table = { redraw() {} };
      this.setSplitter = () => {};
      this.destroy = () => {};
    }

    async init() {}

    async loadData() {}
  },
}));

const html = readFileSync(resolve(process.cwd(), 'src/managers/reports/reports.html'), 'utf8');

const lookups = {
  objectTypes: [{ key_idx: 21, collection: { attributes: [7, 8] } }],
  attributes: [
    { key_idx: 7, value_txt: 'Field', collection: { Editor: 'Field', section: 4 } },
    { key_idx: 8, value_txt: 'Datasource', collection: { Editor: 'Datasource', section: 4 } },
  ],
};

describe('field property rows', () => {
  const sel = { bandId: 'd1', itemId: 'n1' };

  it('Field lists the sample columns, Datasource lists definition datasources', () => {
    const doc = { ...addItem(createBlank(), 'd1', { typeKey: 21, id: 'n1', field: '' }), datasources: ['main'] };
    const rows = propertyRows(doc, sel, { ...lookups, sampleColumns: ['customer', 'total'] });
    const field = rows.find((row) => row.id === '7');
    expect(field.editor).toBe('List');
    expect(field.list).toEqual(['customer', 'total']);
    expect(field.readOnly).toBe(false);
    expect(rows.find((row) => row.id === '8').list).toEqual(['main']);
    const bound = applyProperty(doc, sel, '7', 'customer');
    expect(bound.bands.find((b) => b.id === 'd1').items[0].field).toBe('customer');
  });

  it('with no sample, Field is typed, and a bound value stays listed', () => {
    const doc = addItem(createBlank(), 'd1', { typeKey: 21, id: 'n1', field: 'legacy' });
    expect(propertyRows(doc, sel, lookups).find((row) => row.id === '7').list).toEqual(['legacy']);
    const empty = addItem(createBlank(), 'd1', { typeKey: 21, id: 'n1', field: '' });
    expect(propertyRows(empty, sel, lookups).find((row) => row.id === '7').editor).toBe('Text');
  });
});

describe('Report Manager field items', () => {
  let container;
  let manager;

  beforeEach(() => {
    vi.stubGlobal('fetch', vi.fn(async (url) => (url === '/src/managers/reports/reports.html'
      ? new Response(html, { status: 200, headers: { 'content-type': 'text/html' } })
      : new Response('missing', { status: 404 }))));
    container = document.createElement('div');
    container.id = 'manager-slot-24';
    container.className = 'manager-slot slot-visible';
    document.body.appendChild(container);
    const host = document.createElement('div');
    container.appendChild(host);
    manager = new ReportsManager({}, host);
  });

  afterEach(() => {
    manager.destroy();
    container.remove();
  });

  async function open() {
    await manager.init();
    manager.setRecordEditing(true);
    const saved = [];
    manager.saveFile = (filename, json) => saved.push({ filename, json });
    return saved;
  }

  const q = (sel) => manager.container.querySelector(sel);

  it('Field category swaps the third toolbar', async () => {
    await open();
    expect(q('#reports-field-number-btn').hidden).toBe(true);
    q('#reports-field-btn').click();
    expect(q('#reports-field-btn').getAttribute('aria-pressed')).toBe('true');
    expect(q('#reports-static-btn').getAttribute('aria-pressed')).toBe('false');
    expect(q('#reports-text-btn').hidden).toBe(true);
    expect(q('#reports-shape-btn').hidden).toBe(true);
    for (const id of ['field-number', 'field-datetime', 'field-text', 'field-memo', 'field-shape', 'field-barcode', 'field-image', 'sample', 'sample-paste']) {
      expect(q(`#reports-${id}-btn`).hidden).toBe(false);
    }
    const order = [...manager.container.querySelectorAll('#reports-items-toolbar [data-reports-category="field"][data-type-key]')]
      .filter((btn) => !btn.hidden)
      .map((btn) => btn.dataset.typeKey);
    expect(order).toEqual(['21', '24', '20', '23', '26', '45', '22']);
    q('#reports-static-btn').click();
    expect(q('#reports-text-btn').hidden).toBe(false);
    expect(q('#reports-shape-btn').hidden).toBe(false);
    expect(q('#reports-sample-btn').hidden).toBe(true);
  });

  it('adds an unbound field, refuses export, then exports once bound with rows on the datasource', async () => {
    const saved = await open();
    q('#reports-field-btn').click();
    q('#reports-field-datetime-btn').click();
    const item = manager.doc.bands.find((b) => b.id === 'd1').items[0];
    expect(item).toMatchObject({ typeKey: 24, field: '', mask: '' });
    const node = q('.reports-item-field');
    expect(node.textContent).toBe('Field datetime');
    expect(node.dataset.unbound).toBe('true');

    manager.elements.exportBtn.click();
    expect(saved).toHaveLength(0);

    expect(manager.loadSample('[{"customer":"A","when":"2026-11-03"}]')).toBe(true);
    expect(manager.sampleColumns()).toEqual(['customer', 'when']);
    expect(manager.loadSample('not json')).toBe(false);
    expect(manager.sampleColumns()).toEqual(['customer', 'when']);

    manager._commitDocument(applyProperty(manager.doc, { bandId: 'd1', itemId: item.id }, '7', 'when'));
    expect(q('.reports-item-field').textContent).toBe('when');
    manager.elements.exportBtn.click();
    expect(saved).toHaveLength(1);
    expect(JSON.parse(saved[0].json).datasources[0]).toMatchObject({ name: 'data1', kind: 'json', kindKey: 1, rows: [{ customer: 'A', when: '2026-11-03' }] });
    expect(JSON.parse(saved[0].json).bands.find((b) => b.id === 'd1').items[0]).toMatchObject({ typeKey: 24, field: 'when' });
  });
});
