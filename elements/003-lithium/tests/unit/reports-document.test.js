import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { describe, expect, it } from 'vitest';
import {
  addBand,
  addItem,
  cloneItem,
  createBlank,
  deleteBand,
  deleteItem,
  exportDefinition,
  importDefinition,
  moveBand,
  moveItem,
  resizeBand,
  resizeItem,
} from '../../src/managers/reports/model/document.js';

const FIXTURES = [
  'static.definition.json',
  'detail.definition.json',
  'grouped.definition.json',
];

function loadDefinition(name) {
  const path = resolve(process.cwd(), 'tests/fixtures/reporting', name);
  return JSON.parse(readFileSync(path, 'utf8'));
}

function band(doc, id) {
  return doc.bands.find((entry) => entry.id === id);
}

function item(doc, bandId, itemId) {
  return band(doc, bandId).items.find((entry) => entry.id === itemId);
}

describe('report document', () => {
  it('round-trips the Phase 1 fixtures', () => {
    for (const name of FIXTURES) {
      const fixture = loadDefinition(name);
      const original = JSON.parse(JSON.stringify(fixture));
      const imported = importDefinition(fixture);
      expect(fixture).toEqual(original);
      expect(exportDefinition(imported)).toEqual(original);
      expect(exportDefinition(importDefinition(JSON.stringify(original)))).toEqual(original);
    }
  });

  it('leaves omitted item keys omitted', () => {
    const fixture = loadDefinition('static.definition.json');
    const title = item(importDefinition(fixture), 'detail', 'title');
    expect(Object.hasOwn(title, 'align')).toBe(false);
    expect(Object.hasOwn(title, 'font')).toBe(false);
    expect(Object.hasOwn(title, 'mask')).toBe(false);
    expect(title.text).toBe('Static');
  });

  it('keeps a file that is only format 1', () => {
    expect(exportDefinition(importDefinition({ format: 1 }))).toEqual({ format: 1 });
  });

  it('rejects a format other than 1', () => {
    expect(() => importDefinition({ format: 2 })).toThrow(/Report format must be 1/);
    expect(() => importDefinition({ format: '1' })).toThrow(/Report format must be 1/);
    expect(() => importDefinition({})).toThrow(/no format/);
    expect(() => importDefinition(null)).toThrow(/JSON object/);
    expect(() => importDefinition([])).toThrow(/JSON object/);
    expect(() => importDefinition('{')).toThrow(/not JSON/);
  });

  it('rejects a duplicate band id or item id', () => {
    const fixture = loadDefinition('detail.definition.json');
    const duplicateBand = JSON.parse(JSON.stringify(fixture));
    duplicateBand.bands[2].id = 'detail';
    expect(() => importDefinition(duplicateBand)).toThrow(/Band id "detail" is already used/);
    const duplicateItem = JSON.parse(JSON.stringify(fixture));
    duplicateItem.bands[0].items = [{ id: 'customer', typeKey: 0 }];
    expect(() => importDefinition(duplicateItem)).toThrow(/Item id "customer" is already used/);
  });

  it('writes the Defaults table for a new report', () => {
    const blank = createBlank();
    const staticPage = loadDefinition('static.definition.json');
    expect(blank.format).toBe(1);
    expect(blank.name).toBe('Report');
    expect(blank.title).toBe('Report');
    expect(blank.page).toEqual(staticPage.page);
    expect(blank.grid).toEqual(staticPage.grid);
    expect(blank.font).toEqual(staticPage.font);
    expect(blank.datasources).toEqual([]);
    expect(blank.functions).toEqual([]);
    expect(blank.bands).toEqual([
      { id: 'ph1', typeKey: 13, height: '0.4 in', items: [] },
      { id: 'd1', typeKey: 15, height: '0.25 in', items: [] },
      { id: 'pf1', typeKey: 17, height: '0.4 in', items: [] },
    ]);
    expect(Object.hasOwn(band(blank, 'd1'), 'datasource')).toBe(false);
    expect(blank.page.sizeKey).toBe(0);
    expect(blank.font.style).toBe('Normal');
    expect(blank.grid.style).toBe('dots');
  });

  it('adds, moves, resizes, and deletes a band', () => {
    const blank = createBlank();
    const added = addBand(blank, { typeKey: 14, field: 'region', index: 1 });
    expect(added.bands.map((entry) => entry.id)).toEqual(['ph1', 'gh1', 'd1', 'pf1']);
    expect(band(added, 'gh1')).toEqual({
      id: 'gh1',
      typeKey: 14,
      field: 'region',
      height: '0.25 in',
      items: [],
    });
    const moved = moveBand(added, 'pf1', 0);
    expect(moved.bands.map((entry) => entry.id)).toEqual(['pf1', 'ph1', 'gh1', 'd1']);
    const resized = resizeBand(moved, 'gh1', '96 px');
    expect(band(resized, 'gh1').height).toBe('1 in');
    const clamped = resizeBand(resized, 'gh1', '0.05 in');
    expect(band(clamped, 'gh1').height).toBe('0.15 in');
    expect(band(resizeBand(clamped, 'gh1', '0.15 in'), 'gh1').height).toBe('0.15 in');
    const removed = deleteBand(clamped, 'gh1');
    expect(removed.bands.map((entry) => entry.id)).toEqual(['pf1', 'ph1', 'd1']);
    expect(blank.bands.map((entry) => entry.id)).toEqual(['ph1', 'd1', 'pf1']);
  });

  it('adds, moves, resizes, and deletes an item', () => {
    const blank = createBlank();
    const added = addItem(blank, 'd1', { typeKey: 0 });
    expect(item(added, 'd1', 'text_1')).toEqual({
      id: 'text_1',
      typeKey: 0,
      align: 'Left',
      text: 'Text',
      box: {
        left: '3.25 in',
        top: '0.025 in',
        width: '1.5 in',
        height: '0.2 in',
      },
    });
    expect(Object.hasOwn(item(added, 'd1', 'text_1'), 'font')).toBe(false);
    expect(Object.hasOwn(item(added, 'd1', 'text_1'), 'mask')).toBe(false);
    const numberItem = addItem(added, 'd1', { typeKey: 21, field: 'amount' });
    expect(item(numberItem, 'd1', 'number_1').mask).toBe('');
    expect(Object.hasOwn(item(numberItem, 'd1', 'number_1'), 'text')).toBe(false);
    const textItem = addItem(numberItem, 'ph1', { typeKey: 20, field: 'customer', id: 'customer' });
    expect(Object.hasOwn(item(textItem, 'ph1', 'customer'), 'mask')).toBe(false);
    const moved = moveItem(textItem, 'd1', 'text_1', { left: '96 px' });
    expect(item(moved, 'd1', 'text_1').box.left).toBe('1 in');
    expect(item(moved, 'd1', 'text_1').box.width).toBe('1.5 in');
    expect(item(moved, 'd1', 'text_1').id).toBe('text_1');
    const resized = resizeItem(moved, 'd1', 'text_1', { width: '2 in', height: '0.3 in' });
    expect(item(resized, 'd1', 'text_1').box).toEqual({
      left: '1 in',
      top: '0.025 in',
      width: '2 in',
      height: '0.225 in',
    });
    const removed = deleteItem(resized, 'd1', 'text_1');
    expect(band(removed, 'd1').items.map((entry) => entry.id)).toEqual(['number_1']);
    expect(band(blank, 'd1').items).toEqual([]);
  });

  it('clamps a move and a resize to the band', () => {
    const blank = createBlank();
    const added = addItem(blank, 'd1', {
      typeKey: 0,
      box: { left: '0 in', top: '0 in', width: '1 in', height: '1 in' },
    });
    expect(item(added, 'd1', 'text_1').box.height).toBe('1 in');
    const slid = moveItem(added, 'd1', 'text_1', { left: '-2 in', top: '-1 in' });
    expect(item(slid, 'd1', 'text_1').box).toEqual({
      left: '0 in',
      top: '0 in',
      width: '1 in',
      height: '0.25 in',
    });
    const parked = moveItem(added, 'd1', 'text_1', { left: '10 in' });
    expect(item(parked, 'd1', 'text_1').box.left).toBe('7 in');
    expect(item(parked, 'd1', 'text_1').box.width).toBe('1 in');
    const wide = resizeItem(added, 'd1', 'text_1', { width: '20 in' });
    expect(item(wide, 'd1', 'text_1').box.left).toBe('0 in');
    expect(item(wide, 'd1', 'text_1').box.width).toBe('8 in');
    const flat = resizeItem(added, 'd1', 'text_1', { width: '0 in', height: '0 in' });
    expect(item(flat, 'd1', 'text_1').box.width).toBe('0.010417 in');
    expect(item(flat, 'd1', 'text_1').box.height).toBe('0.010417 in');
    const metric = {
      format: 1,
      page: {
        units: 'mm',
        width: '210 mm',
        height: '297 mm',
        margin: { left: '0 mm', right: '0 mm', top: '0 mm', bottom: '0 mm' },
      },
      bands: [{ id: 'd1', typeKey: 15, height: '20 mm', items: [] }],
    };
    const millimetre = moveItem(addItem(metric, 'd1', { typeKey: 0 }), 'd1', 'text_1', { left: '0 px' });
    expect(item(millimetre, 'd1', 'text_1').box.left.endsWith(' mm')).toBe(true);
    expect(item(millimetre, 'd1', 'text_1').box.left).toBe('0 mm');
  });

  it('does not invent a datasource on a new detail band', () => {
    const next = addBand(createBlank(), { typeKey: 15, id: 'more' });
    expect(Object.hasOwn(band(next, 'more'), 'datasource')).toBe(false);
    expect(band(next, 'more').height).toBe('0.25 in');
  });

  it('numbers each item kind on its own and keeps an id that was passed in', () => {
    const blank = createBlank();
    const text = addItem(blank, 'd1', { typeKey: 0 });
    const shape = addItem(text, 'd1', { typeKey: 25 });
    const again = addItem(shape, 'd1', { typeKey: 0, id: 's1' });
    const next = addItem(again, 'd1', { typeKey: 0 });
    expect(band(next, 'd1').items.map((entry) => entry.id)).toEqual(['text_1', 'shape_1', 's1', 'text_2']);
  });

  it('copies an item onto the same band with a new id and the same box', () => {
    const blank = createBlank();
    const added = addItem(blank, 'd1', {
      typeKey: 0,
      text: 'Hello',
      box: { left: '1 in', top: '0 in', width: '1.5 in', height: '0.2 in' },
    });
    const copy = cloneItem(added, 'd1', 'text_1');
    const items = band(copy, 'd1').items;
    expect(items.map((entry) => entry.id)).toEqual(['text_1', 'text_2']);
    expect(item(copy, 'd1', 'text_2').text).toBe('Hello');
    expect(item(copy, 'd1', 'text_2').box).toEqual(item(copy, 'd1', 'text_1').box);
    expect(band(added, 'd1').items).toHaveLength(1);
    expect(band(blank, 'd1').items).toEqual([]);
  });
});
