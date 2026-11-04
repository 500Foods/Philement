import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import ReportsManager from '../../src/managers/reports/reports.js';
import { addBandInOrder, addGroup, addItem, createBlank } from '../../src/managers/reports/model/document.js';
import { dropItem } from '../../src/managers/reports/reports-drop.js';
import { paintBandItems } from '../../src/managers/reports/items.js';
import {
  AGGREGATE_OPS,
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

  it('refuses a detail band and allows a page header and a page footer', () => {
    const message = 'An aggregate item goes on a group header, a group footer, a report header, a report footer, a page header, or a page footer.';
    expect(() => addItem(createBlank(), 'd1', dropItem(35))).toThrow(message);
    for (const bandId of ['ph1', 'pf1']) {
      const doc = addItem(createBlank(), bandId, dropItem(35));
      expect(doc.bands.find((band) => band.id === bandId).items[0]).toMatchObject({
        typeKey: 35, op: 'count', field: '', mask: '',
      });
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
    expect(aggregateFunctionAllowed('avg', 'number')).toBe(true);
    expect(aggregateFunctionAllowed('mean', 'text')).toBe(false);
    expect(aggregateFunctionAllowed('weighted', undefined)).toBe(false);
    expect(aggregateFunctionAllowed('median', 'number')).toBe(true);
    expect(aggregateFunctionAllowed('mode', 'text')).toBe(true);
    expect(aggregateFunctionAllowed('mode', 'number')).toBe(true);
    expect(aggregateFunctionAllowed('mode', 'datetime')).toBe(false);
    expect(aggregateFunctionAllowed('mode', undefined)).toBe(true);
    expect(aggregateFunctionAllowed('min', 'text')).toBe(true);
    expect(aggregateFunctionAllowed('min', 'number')).toBe(true);
    expect(aggregateFunctionAllowed('min', 'datetime')).toBe(true);
    expect(aggregateFunctionAllowed('max', 'boolean')).toBe(false);
    expect(aggregateFunctionAllowed('first', 'rows')).toBe(false);
    expect(aggregateFunctionAllowed('last', undefined)).toBe(true);
    expect(aggregateFunctionChoices(functions, 'sum')).toEqual(['total']);
    expect(aggregateFunctionChoices(functions, 'min')).toEqual(['label', 'total', 'when', 'plain']);
    expect(aggregateFunctionChoices(functions, 'avg')).toEqual(['total']);
    expect(aggregateFunctionChoices(functions, 'mode')).toEqual(['label', 'total', 'plain']);
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
    expect(() => setAggregateOp(item, 'range', functions)).toThrow(/min, max, first, last/);
    expect(() => setAggregateFunction(item, 'flag', functions)).toThrow(/not offered for sum/);
  });

  it('stores weight only while the op is weighted, and keeps it when the field changes', () => {
    const item = { op: 'sum', field: 'amount', typeKey: 35, mask: '#,##0.00' };
    setAggregateOp(item, 'weighted', functions);
    expect(item).toEqual({ op: 'weighted', field: 'amount', typeKey: 35, mask: '#,##0.00', weight: '' });
    item.weight = 'qty';
    setAggregateField(item, 'cost');
    expect(item).toEqual({ op: 'weighted', field: 'cost', typeKey: 35, mask: '#,##0.00', weight: 'qty' });
    setAggregateOp(item, 'avg', functions);
    expect(item.weight).toBeUndefined();
    expect(item.op).toBe('avg');
  });

  it('uses aggregate text for a text function on mode, and a number for a number function', () => {
    const item = { typeKey: 35, op: 'count', field: 'amount', mask: '#,##0.00' };
    setAggregateOp(item, 'mode', functions);
    setAggregateFunction(item, 'label', functions);
    expect(item).toEqual({ typeKey: 34, op: 'mode', function: 'label' });
    setAggregateFunction(item, 'total', functions);
    expect(item).toEqual({ typeKey: 35, op: 'mode', function: 'total', mask: '' });
    setAggregateField(item, 'amount');
    expect(item).toEqual({ typeKey: 35, op: 'mode', field: 'amount', mask: '' });
  });

  it('keeps the return type for min, max, first, and last', () => {
    const item = { typeKey: 35, op: 'count', field: 'amount', mask: '#,##0.00' };
    setAggregateOp(item, 'min', functions);
    setAggregateFunction(item, 'when', functions);
    expect(item).toEqual({ typeKey: 38, op: 'min', function: 'when', mask: '' });
    setAggregateFunction(item, 'label', functions);
    expect(item).toEqual({ typeKey: 34, op: 'min', function: 'label' });
    setAggregateFunction(item, 'total', functions);
    expect(item).toEqual({ typeKey: 35, op: 'min', function: 'total', mask: '' });
    setAggregateField(item, 'when');
    expect(item).toEqual({ typeKey: 35, op: 'min', field: 'when', mask: '' });
    const dated = { typeKey: 38, op: 'max', field: 'when', mask: 'yyyy-MM-dd' };
    setAggregateField(dated, 'when');
    expect(dated).toEqual({ typeKey: 38, op: 'max', field: 'when', mask: 'yyyy-MM-dd' });
    const added = addItem(withFooter(), 'rf1', { typeKey: 38, id: 'd1' });
    expect(added.bands.find((band) => band.id === 'rf1').items[0]).toMatchObject({
      typeKey: 38, op: 'min', field: '', mask: '',
    });
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
    expect(rows.find((row) => row.id === 'aggregate-op').list).toEqual([...AGGREGATE_OPS]);
    expect(rows.find((row) => row.id === 'aggregate-weight')).toBeUndefined();
    const fn = rows.find((row) => row.id === 'aggregate-function');
    expect(fn.list).toEqual(['total']);
    expect(fn.note).toMatch(/formula compiler/);
    const bound = applyProperty(applyProperty(doc, sel, 'aggregate-op', 'count'), sel, '7', 'amount');
    expect(bound.bands.find((band) => band.id === 'rf1').items[0]).toMatchObject({ op: 'count', field: 'amount' });
    expect(bound.bands.find((band) => band.id === 'rf1').items[0].function).toBeUndefined();
  });

  it('shows Weight for weighted, and a text mode drops the mask', () => {
    const base = withFooter();
    base.functions = functions;
    const doc = addItem(base, 'rf1', { typeKey: 35, id: 'a1', op: 'sum', field: 'amount', mask: '#,##0.00' });
    const sel = { bandId: 'rf1', itemId: 'a1' };
    const weighted = applyProperty(doc, sel, 'aggregate-op', 'weighted');
    const weight = propertyRows(weighted, sel, lookups).find((row) => row.id === 'aggregate-weight');
    expect(weight.value).toBe('');
    expect(weight.list).toEqual(['amount', 'name']);
    const weighed = applyProperty(weighted, sel, 'aggregate-weight', 'qty');
    expect(weighed.bands.find((band) => band.id === 'rf1').items[0].weight).toBe('qty');
    const mode = applyProperty(applyProperty(weighed, sel, 'aggregate-op', 'mode'), sel, 'aggregate-function', 'label');
    const item = mode.bands.find((band) => band.id === 'rf1').items[0];
    expect(item).toMatchObject({ typeKey: 34, op: 'mode', function: 'label' });
    expect(item.mask).toBeUndefined();
    expect(item.weight).toBeUndefined();
    const text = addItem(withFooter(), 'rf1', { typeKey: 34, id: 't1' });
    expect(text.bands.find((band) => band.id === 'rf1').items[0]).toMatchObject({ typeKey: 34, op: 'mode', field: '' });
    expect(() => addItem(createBlank(), 'd1', { typeKey: 34 })).toThrow(/aggregate item/);
  });

  it('paints a text mode on the band', () => {
    const body = document.createElement('div');
    paintBandItems(body, {
      id: 'rf1',
      items: [{
        id: 'm1', typeKey: 34, op: 'mode', function: 'label',
        box: { left: '0 in', top: '0 in', width: '1 in', height: '0.2 in' },
      }],
    }, { font: { family: 'Vanadium Sans', size: '10 pt', style: 'Normal' }, page: { units: 'in' } });
    const node = body.querySelector('.reports-item-aggregate');
    expect(node.textContent).toBe('mode label');
    expect(node.dataset.unbound).toBe('false');
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

  it('enables Number on a group, page, or report header or footer', async () => {
    await manager.init();
    manager.setRecordEditing(true);
    q('#reports-aggregate-btn').click();
    const button = q('#reports-aggregate-number-btn');
    expect(button.hidden).toBe(false);
    expect(button.disabled).toBe(true);

    const selectBand = (id) => {
      const strip = q(`[data-band-id="${id}"] .reports-band-strip`);
      const event = new CustomEvent('pointerdown', { bubbles: true, cancelable: true });
      strip.dispatchEvent(event);
    };
    selectBand('ph1');
    expect(q('#reports-aggregate-number-btn').disabled).toBe(false);
    selectBand('pf1');
    expect(q('#reports-aggregate-number-btn').disabled).toBe(false);
    selectBand('d1');
    expect(q('#reports-aggregate-number-btn').disabled).toBe(true);

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
