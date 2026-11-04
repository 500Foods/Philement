import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import ReportsManager from '../../src/managers/reports/reports.js';
import { addBandInOrder, addGroup, addItem, createBlank } from '../../src/managers/reports/model/document.js';
import { dropItem } from '../../src/managers/reports/reports-drop.js';
import {
  aggregateFunctionAllowed,
  aggregateFunctionChoices,
  aggregateItemLabel,
  setAggregateField,
  setAggregateFunction,
  setAggregateOp,
} from '../../src/managers/reports/reports-aggregate.js';
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

const functions = [
  { name: 'label', returns: 'text' },
  { name: 'total', returns: 'number' },
  { name: 'when', returns: 'datetime' },
  { name: 'flag', returns: 'boolean' },
  { name: 'lines', returns: 'rows' },
  { name: 'plain' },
];

const lookups = {
  objectTypes: [{ key_idx: 35, collection: { attributes: [7] } }],
  attributes: [{ key_idx: 7, value_txt: 'Field', collection: { Editor: 'Field', section: 4 } }],
  sampleColumns: ['amount', 'name'],
};

function withFooter() {
  return addBandInOrder(createBlank(), 18);
}

describe('aggregate item', () => {
  it('drops a count with an empty field', () => {
    expect(dropItem(35)).toEqual({ typeKey: 35, op: 'count', field: '' });
  });

  it('refuses a detail, page header, or page footer', () => {
    const message = 'An aggregate item goes on a group header, a group footer, a report header, or a report footer.';
    for (const bandId of ['d1', 'ph1', 'pf1']) {
      expect(() => addItem(createBlank(), bandId, dropItem(35))).toThrow(message);
    }
  });

  it('stores op and field on a report footer, group header, and report header', () => {
    const footer = addItem(withFooter(), 'rf1', { typeKey: 35, id: 'a1' });
    expect(footer.bands.find((band) => band.id === 'rf1').items[0]).toMatchObject({
      typeKey: 35, op: 'count', field: '', mask: '',
    });
    const grouped = addItem(addGroup(createBlank(), { field: 'region' }), 'gh1', { typeKey: 35, op: 'sum', field: 'amount' });
    expect(grouped.bands.find((band) => band.id === 'gh1').items[0]).toMatchObject({ op: 'sum', field: 'amount' });
    const header = addItem(addBandInOrder(createBlank(), 12), 'rh1', dropItem(35));
    expect(header.bands.find((band) => band.id === 'rh1').items[0].op).toBe('count');
  });

  it('offers a function only when its returns allow the op', () => {
    expect(aggregateFunctionAllowed('sum', 'number')).toBe(true);
    expect(aggregateFunctionAllowed('sum', 'text')).toBe(false);
    expect(aggregateFunctionAllowed('sum', undefined)).toBe(false);
    expect(aggregateFunctionAllowed('count', undefined)).toBe(true);
    expect(aggregateFunctionAllowed('unique', 'datetime')).toBe(true);
    expect(aggregateFunctionAllowed('filled', 'boolean')).toBe(false);
    expect(aggregateFunctionAllowed('count', 'rows')).toBe(false);
    expect(aggregateFunctionChoices(functions, 'sum')).toEqual(['total']);
    expect(aggregateFunctionChoices(functions, 'count')).toEqual(['label', 'total', 'when', 'plain']);
  });

  it('keeps exactly one of field or function', () => {
    const item = { op: 'count', function: 'label' };
    setAggregateOp(item, 'sum', functions);
    expect(item).toEqual({ op: 'sum', field: '' });
    setAggregateFunction(item, 'total', functions);
    expect(item).toEqual({ op: 'sum', function: 'total' });
    setAggregateField(item, 'amount');
    expect(item).toEqual({ op: 'sum', field: 'amount' });
    expect(() => setAggregateOp(item, 'avg', functions)).toThrow(/count, filled, unique, sum/);
    expect(() => setAggregateFunction(item, 'flag', functions)).toThrow(/not offered for sum/);
  });

  it('labels the op and the bound name', () => {
    expect(aggregateItemLabel({ op: 'sum', field: 'amount' })).toBe('sum amount');
    expect(aggregateItemLabel({ op: 'count', function: 'pack_code' })).toBe('count pack_code');
    expect(aggregateItemLabel({ op: 'count', field: '' })).toBe('Aggregate');
  });

  it('shows Op and Function, and a field clears the function', () => {
    const base = withFooter();
    base.functions = functions;
    const doc = addItem(base, 'rf1', { typeKey: 35, id: 'a1', op: 'sum', function: 'total' });
    const sel = { bandId: 'rf1', itemId: 'a1' };
    const rows = propertyRows(doc, sel, lookups);
    expect(rows.find((row) => row.id === 'aggregate-op').list).toEqual(['count', 'filled', 'unique', 'sum']);
    const fn = rows.find((row) => row.id === 'aggregate-function');
    expect(fn.list).toEqual(['total']);
    expect(fn.note).toMatch(/formula compiler/);
    const bound = applyProperty(applyProperty(doc, sel, 'aggregate-op', 'count'), sel, '7', 'amount');
    expect(bound.bands.find((band) => band.id === 'rf1').items[0]).toMatchObject({ op: 'count', field: 'amount' });
    expect(bound.bands.find((band) => band.id === 'rf1').items[0].function).toBeUndefined();
  });
});

describe('Report Manager aggregate palette', () => {
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

  const q = (sel) => manager.container.querySelector(sel);

  it('enables Number only on a group or report header or footer', async () => {
    await manager.init();
    manager.setRecordEditing(true);
    q('#reports-aggregate-btn').click();
    const button = q('#reports-aggregate-number-btn');
    expect(button.hidden).toBe(false);
    expect(button.disabled).toBe(true);

    q('#reports-bands-btn').click();
    q('#reports-band-report-footer-btn').click();
    q('#reports-aggregate-btn').click();
    expect(q('#reports-aggregate-number-btn').disabled).toBe(false);
    q('#reports-aggregate-number-btn').click();

    const band = manager.doc.bands.find((entry) => entry.typeKey === 18);
    expect(band.items[0]).toMatchObject({ typeKey: 35, op: 'count', field: '' });
    const node = q('.reports-item-aggregate');
    expect(node.textContent).toBe('Aggregate');
    expect(node.dataset.unbound).toBe('true');
  });
});
