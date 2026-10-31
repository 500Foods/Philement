import { readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { describe, expect, it } from 'vitest';
import {
  addItem,
  deleteItem,
  exportDefinition,
  moveItem,
  resizeBand,
} from '../../src/managers/reports/model/document.js';
import { createHistory } from '../../src/managers/reports/model/history.js';

function loadDetail() {
  const path = resolve(process.cwd(), 'tests/fixtures/reporting/detail.definition.json');
  return JSON.parse(readFileSync(path, 'utf8'));
}

function customer(doc) {
  return doc.bands.find((band) => band.id === 'detail').items.find((entry) => entry.id === 'customer');
}

describe('report history', () => {
  it('undoes a field move back to the detail fixture', () => {
    const fixture = loadDetail();
    const history = createHistory(fixture);
    const moved = moveItem(history.document, 'detail', 'customer', { left: '1 in' });
    expect(customer(moved).box.left).toBe('1 in');
    expect(customer(moved).id).toBe('customer');
    history.commit(moved);
    expect(customer(fixture).box.left).toBe('0 in');
    history.undo();
    expect(history.canRedo()).toBe(true);
    expect(customer(history.document).id).toBe('customer');
    expect(exportDefinition(history.document)).toEqual(fixture);
  });

  it('drops the redo tail when a new edit is committed', () => {
    const fixture = loadDetail();
    const history = createHistory(fixture);
    history.commit(moveItem(history.document, 'detail', 'customer', { left: '1 in' }));
    history.undo();
    history.commit(moveItem(history.document, 'detail', 'customer', { left: '2 in' }));
    expect(history.canRedo()).toBe(false);
    history.redo();
    expect(customer(history.document).box.left).toBe('2 in');
    history.undo();
    expect(exportDefinition(history.document)).toEqual(fixture);
  });

  it('restores the same item id across undo and redo', () => {
    const history = createHistory(loadDetail());
    const removed = deleteItem(history.document, 'detail', 'amount');
    history.commit(removed);
    expect(history.document.bands[1].items.map((entry) => entry.id)).toEqual(['customer', 'note']);
    history.undo();
    expect(history.document.bands[1].items.map((entry) => entry.id)).toEqual([
      'customer', 'amount', 'note',
    ]);
    history.redo();
    expect(history.document.bands[1].items.map((entry) => entry.id)).toEqual(['customer', 'note']);
    history.undo();
    const added = addItem(history.document, 'detail', { typeKey: 0 });
    history.commit(added);
    expect(history.document.bands[1].items.at(-1).id).toBe('i1');
    history.undo();
    history.redo();
    expect(history.document.bands[1].items.at(-1).id).toBe('i1');
  });

  it('restores a band height', () => {
    const fixture = loadDetail();
    const history = createHistory(fixture);
    history.commit(resizeBand(history.document, 'detail', '0.5 in'));
    expect(history.document.bands[1].height).toBe('0.5 in');
    history.undo();
    expect(history.document.bands[1].height).toBe('0.25 in');
    expect(exportDefinition(history.document)).toEqual(fixture);
  });

  it('does nothing when there is no snapshot to restore', () => {
    const history = createHistory(loadDetail());
    expect(history.canUndo()).toBe(false);
    expect(history.canRedo()).toBe(false);
    history.undo();
    history.redo();
    expect(history.canUndo()).toBe(false);
    expect(() => history.commit({ format: 2 })).toThrow(/Report format must be 1/);
    expect(history.canUndo()).toBe(false);
  });
});
