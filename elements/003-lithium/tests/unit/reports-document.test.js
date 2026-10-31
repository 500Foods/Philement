import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { describe, expect, it } from 'vitest';
import {
  addBand,
  addItem,
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
      { id: 'ph', typeKey: 13, height: '0.4 in', items: [] },
      { id: 'detail', typeKey: 15, height: '0.25 in', items: [] },
      { id: 'pf', typeKey: 17, height: '0.4 in', items: [] },
    ]);
    expect(Object.hasOwn(band(blank, 'detail'), 'datasource')).toBe(false);
    expect(blank.page.sizeKey).toBe(0);
    expect(blank.font.style).toBe('Normal');
    expect(blank.grid.style).toBe('dots');
  });

  it('adds, moves, resizes, and deletes a band', () => {
    const blank = createBlank();
    const added = addBand(blank, { typeKey: 14, field: 'region', index: 1 });
    expect(added.bands.map((entry) => entry.id)).toEqual(['ph', 'b1', 'detail', 'pf']);
    expect(band(added, 'b1')).toEqual({
      id: 'b1',
      typeKey: 14,
      field: 'region',
      height: '0.25 in',
      items: [],
    });
    const moved = moveBand(added, 'pf', 0);
    expect(moved.bands.map((entry) => entry.id)).toEqual(['pf', 'ph', 'b1', 'detail']);
    const resized = resizeBand(moved, 'b1', '96 px');
    expect(band(resized, 'b1').height).toBe('1 in');
    const clamped = resizeBand(resized, 'b1', '0.05 in');
    expect(band(clamped, 'b1').height).toBe('0.15 in');
    expect(band(resizeBand(clamped, 'b1', '0.15 in'), 'b1').height).toBe('0.15 in');
    const removed = deleteBand(clamped, 'b1');
    expect(removed.bands.map((entry) => entry.id)).toEqual(['pf', 'ph', 'detail']);
    expect(blank.bands.map((entry) => entry.id)).toEqual(['ph', 'detail', 'pf']);
  });

  it('adds, moves, resizes, and deletes an item', () => {
    const blank = createBlank();
    const added = addItem(blank, 'detail', { typeKey: 0 });
    expect(item(added, 'detail', 'i1')).toEqual({
      id: 'i1',
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
    expect(Object.hasOwn(item(added, 'detail', 'i1'), 'font')).toBe(false);
    expect(Object.hasOwn(item(added, 'detail', 'i1'), 'mask')).toBe(false);
    const numberItem = addItem(added, 'detail', { typeKey: 21, field: 'amount' });
    expect(item(numberItem, 'detail', 'i2').mask).toBe('');
    expect(Object.hasOwn(item(numberItem, 'detail', 'i2'), 'text')).toBe(false);
    const textItem = addItem(numberItem, 'ph', { typeKey: 20, field: 'customer', id: 'customer' });
    expect(Object.hasOwn(item(textItem, 'ph', 'customer'), 'mask')).toBe(false);
    const moved = moveItem(textItem, 'detail', 'i1', { left: '96 px' });
    expect(item(moved, 'detail', 'i1').box.left).toBe('1 in');
    expect(item(moved, 'detail', 'i1').box.width).toBe('1.5 in');
    expect(item(moved, 'detail', 'i1').id).toBe('i1');
    const resized = resizeItem(moved, 'detail', 'i1', { width: '2 in', height: '0.3 in' });
    expect(item(resized, 'detail', 'i1').box).toEqual({
      left: '1 in',
      top: '0.025 in',
      width: '2 in',
      height: '0.3 in',
    });
    const removed = deleteItem(resized, 'detail', 'i1');
    expect(band(removed, 'detail').items.map((entry) => entry.id)).toEqual(['i2']);
    expect(band(blank, 'detail').items).toEqual([]);
  });

  it('does not invent a datasource on a new detail band', () => {
    const next = addBand(createBlank(), { typeKey: 15, id: 'more' });
    expect(Object.hasOwn(band(next, 'more'), 'datasource')).toBe(false);
    expect(band(next, 'more').height).toBe('0.25 in');
  });
});
