import { describe, expect, it } from 'vitest';
import { addItem, createBlank } from '../../src/managers/reports/model/document.js';
import { imageDataUriValue } from '../../src/managers/reports/model/images.js';
import { parseLength } from '../../src/managers/reports/model/units.js';
import { applyProperty, attributeKeys, propertyRows } from '../../src/managers/reports/report-properties.js';
import { paintBandItems } from '../../src/managers/reports/items.js';
import { dropItem } from '../../src/managers/reports/reports-drop.js';
import { outlineEntries } from '../../src/managers/reports/reports-panel.js';
import {
  addDatasource, boundFieldText, setDatasourceRows, unboundFields,
} from '../../src/managers/reports/reports-sample.js';

const PNG = 'data:image/png;base64,AAAA';
const SVG = 'data:image/svg+xml;base64,PHN2Zw==';

function inches(length) {
  return parseLength(length).value;
}

function itemOf(doc, id) {
  return doc.bands.flatMap((band) => band.items || []).find((entry) => entry.id === id);
}

function withRows(rows, name = 'orders') {
  let doc = addDatasource(createBlank(), name);
  doc = setDatasourceRows(doc, name, rows).doc;
  const band = doc.bands.find((entry) => entry.id === 'd1');
  band.datasource = name;
  return doc;
}

function paint(doc, item) {
  const body = document.createElement('div');
  paintBandItems(body, { id: 'd1', items: [item] }, doc);
  return body;
}

describe('field image data URI', () => {
  it('keeps a base64 PNG or SVG and rejects anything else', () => {
    expect(imageDataUriValue(`  ${PNG}  `)).toBe(PNG);
    expect(imageDataUriValue(SVG)).toBe(SVG);
    expect(imageDataUriValue('data:image/jpeg;base64,AAAA')).toBe('');
    expect(imageDataUriValue('data:image/svg+xml,<svg></svg>')).toBe('');
    expect(imageDataUriValue('https://example.com/a.png')).toBe('');
    expect(imageDataUriValue('data:image/png;base64,')).toBe('');
    expect(imageDataUriValue('data:image/png;base64,AAAA hello')).toBe('');
    expect(imageDataUriValue(null)).toBe('');
  });
});

describe('query field items', () => {
  it('reads the first row of the named datasource', () => {
    const doc = withRows([{ note: 'Alpha' }, { note: 'Beta' }]);
    expect(boundFieldText(doc, { datasource: 'orders', field: 'note' })).toBe('Alpha');
    expect(boundFieldText(doc, { datasource: 'orders', field: 'missing' })).toBe('');
    expect(boundFieldText(doc, { datasource: '', field: 'note' })).toBe('');
  });

  it('prefills datasource and leaves the field empty', () => {
    let doc = withRows([{ picture: PNG, note: 'Hello', kind: 'ellipse' }]);
    doc = addItem(doc, 'd1', { typeKey: 22 });
    doc = addItem(doc, 'd1', { typeKey: 23 });
    doc = addItem(doc, 'd1', dropItem(26));
    doc = addItem(doc, 'd1', { typeKey: 20 });
    const items = doc.bands.find((band) => band.id === 'd1').items;
    expect(items.map((item) => item.typeKey)).toEqual([22, 23, 26, 20]);
    for (const item of items) {
      expect(item.datasource).toBe('orders');
      expect(item.field).toBe('');
    }
    const image = items[0];
    expect(image.id.startsWith('imagefield_')).toBe(true);
    expect(image.image).toBeUndefined();
    expect(image.dpi).toBe(72);
    expect(image.fit).toBe('contain');
    expect(inches(image.box.width)).toBeCloseTo(1, 5);
    expect(inches(image.box.height)).toBeCloseTo(0.75, 5);
    const memo = items[1];
    expect(memo.id.startsWith('memofield_')).toBe(true);
    expect(memo.text).toBeUndefined();
    expect(memo.wrap).toBe('pre-wrap');
    expect(memo.overflow).toBe('Grow');
    expect(memo.valign).toBe('top');
    expect(inches(memo.box.width)).toBeCloseTo(1.5, 5);
    expect(inches(memo.box.height)).toBeCloseTo(0.6, 5);
    const shape = items[2];
    expect(shape.id.startsWith('shapefield_')).toBe(true);
    expect(shape.kind).toBe('rectangle');
    expect(shape.align).toBeUndefined();
    expect(shape.strokeWidth).toBe('1 pt');
    expect(inches(shape.box.width)).toBeCloseTo(1.5, 5);
    expect(inches(shape.box.height)).toBeCloseTo(0.2, 5);
    const blank = addItem(createBlank(), 'd1', { typeKey: 24 });
    const datetime = blank.bands.find((band) => band.id === 'd1').items[0];
    expect(datetime).toMatchObject({ typeKey: 24, field: '', mask: '' });
    expect(datetime.datasource).toBeUndefined();
  });

  it('lists Datasource before Field and refuses export until both are set', () => {
    let doc = withRows([{ picture: PNG, note: 'Hello', kind: 'line' }]);
    doc = addItem(doc, 'd1', { typeKey: 22 });
    doc = addItem(doc, 'd1', { typeKey: 23 });
    doc = addItem(doc, 'd1', { typeKey: 26 });
    const items = doc.bands.find((band) => band.id === 'd1').items;
    expect(unboundFields(doc).map((entry) => entry.typeKey)).toEqual([22, 23, 26]);
    const image = items[0];
    const sel = { bandId: 'd1', itemId: image.id };
    const rows = propertyRows(doc, sel, { objectTypes: [], attributes: [] });
    expect(rows.some((row) => row.id === 'static-file')).toBe(false);
    expect(rows.find((row) => row.key === 5).list).toContain('stretch');
    const keys = attributeKeys(doc, sel, { objectTypes: [], attributes: [] });
    expect(keys.indexOf(8)).toBeLessThan(keys.indexOf(7));
    expect(keys).toEqual(expect.arrayContaining([5, 15, 16, 17]));
    doc = applyProperty(doc, sel, '7', 'picture');
    expect(unboundFields(doc).map((entry) => entry.typeKey)).toEqual([23, 26]);
    doc = applyProperty(doc, sel, '8', 'other');
    expect(itemOf(doc, image.id).field).toBe('picture');
    doc = addDatasource(doc, 'other');
    doc = setDatasourceRows(doc, 'other', [{ code: 'Z' }]).doc;
    doc = applyProperty(doc, sel, '8', 'other');
    expect(itemOf(doc, image.id).datasource).toBe('other');
    expect(itemOf(doc, image.id).field).toBeUndefined();
    const legacy = addItem(createBlank(), 'd1', { typeKey: 20, field: 'customer' });
    expect(unboundFields(legacy)).toEqual([]);
  });

  it('paints the query picture, the memo text, and a known shape kind', () => {
    let doc = withRows([{
      picture: PNG, note: 'Hello there', kind: 'ellipse', side: 'line', bad: 'data:image/jpeg;base64,AAAA',
    }]);
    doc = addItem(doc, 'd1', { typeKey: 22, field: 'picture' });
    const image = doc.bands.find((band) => band.id === 'd1').items.at(-1);
    const painted = paint(doc, image);
    expect(painted.querySelector('img').getAttribute('src')).toBe(PNG);
    expect(painted.querySelector('.reports-item').dataset.unbound).toBe('false');
    const broken = { ...image, field: 'bad' };
    expect(paint(doc, broken).querySelector('img')).toBeNull();
    expect(paint(doc, broken).textContent).toBe('Image');
    const memo = { ...image, id: 'memofield_1', typeKey: 23, field: 'note', wrap: 'pre-wrap', overflow: 'Grow' };
    expect(paint(doc, memo).textContent).toBe('Hello there');
    const empty = { ...memo, field: 'missing' };
    expect(paint(doc, empty).textContent).toBe('');
    expect(paint(doc, empty).querySelector('.reports-item').dataset.unbound).toBe('false');
    const loose = { ...memo, datasource: '', field: '' };
    expect(paint(doc, loose).textContent).toBe('Memo');
    expect(paint(doc, loose).querySelector('.reports-item').dataset.unbound).toBe('true');
    doc = addItem(doc, 'd1', dropItem(26));
    const shape = doc.bands.find((band) => band.id === 'd1').items.at(-1);
    const pathOf = (item) => paint(doc, item).querySelector('path')?.getAttribute('d') || '';
    const curved = { ...shape, field: 'kind' };
    expect(pathOf(curved)).toMatch(/C/);
    expect(curved.kind).toBe('rectangle');
    expect(pathOf({ ...shape, field: 'note' })).not.toMatch(/C/);
    const labels = outlineEntries(doc).map((entry) => entry.label);
    expect(labels.some((label) => label.includes('Rectangle'))).toBe(true);
    doc = applyProperty(doc, { bandId: 'd1', itemId: shape.id }, '7', 'side');
    const lineKeys = attributeKeys(doc, { bandId: 'd1', itemId: shape.id }, { objectTypes: [], attributes: [] });
    expect(lineKeys).toContain(42);
    expect(lineKeys).not.toContain(48);
  });
});
